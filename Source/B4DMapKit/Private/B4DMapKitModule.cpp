#include "B4DMapKitModule.h"
#include "B4DActors.h"
#include "B4DExporter.h"

#if WITH_EDITOR
#include "ToolMenus.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/MessageDialog.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationItem.h"
#endif

#define LOCTEXT_NAMESPACE "FB4DMapKitModule"

DEFINE_LOG_CATEGORY_STATIC(LogB4DMapKit, Log, All);

void FB4DMapKitModule::StartupModule()
{
#if WITH_EDITOR
	// ToolMenus is not ready the instant a module loads, so wait for its callback.
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FB4DMapKitModule::RegisterMenus));
#endif
}

void FB4DMapKitModule::ShutdownModule()
{
#if WITH_EDITOR
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
#endif
}

#if WITH_EDITOR

void FB4DMapKitModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
	if (!ToolsMenu)
	{
		return;
	}

	FToolMenuSection& Section = ToolsMenu->AddSection(
		TEXT("B4DMapKit"), LOCTEXT("SectionLabel", "Blox 4 Dead"));

	Section.AddMenuEntry(
		TEXT("B4DCheck"),
		LOCTEXT("CheckLabel", "Check Campaign Map"),
		LOCTEXT("CheckTooltip", "Run the game's own map rules over this level and report anything wrong."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FB4DMapKitModule::CheckCampaign)));

	Section.AddMenuEntry(
		TEXT("B4DExport"),
		LOCTEXT("ExportLabel", "Export Campaign JSON..."),
		LOCTEXT("ExportTooltip", "Check the level and write it out as campaign JSON the game loads directly."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FB4DMapKitModule::ExportCampaign)));

	Section.AddMenuEntry(
		TEXT("B4DLivePreview"),
		LOCTEXT("PreviewLabel", "Copy Live Preview Snippet"),
		LOCTEXT("PreviewTooltip", "Copy one line to paste into the game's console to play this map without a rebuild."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FB4DMapKitModule::CopyLivePreview)));
}

namespace
{
	AB4DCampaign* FindEditorCampaign()
	{
		if (!GEditor)
		{
			return nullptr;
		}
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (!World)
		{
			return nullptr;
		}
		for (TActorIterator<AB4DCampaign> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	/** Writes the findings to the log and returns a one-line summary. */
	FString ReportProblems(const TArray<FB4DProblem>& Problems)
	{
		int32 Errors = 0;
		for (const FB4DProblem& Problem : Problems)
		{
			if (Problem.Level == EB4DProblemLevel::Error)
			{
				Errors++;
				UE_LOG(LogB4DMapKit, Error, TEXT("%s"), *Problem.Message);
			}
			else
			{
				UE_LOG(LogB4DMapKit, Warning, TEXT("%s"), *Problem.Message);
			}
		}
		const int32 Warnings = Problems.Num() - Errors;
		if (Errors == 0 && Warnings == 0)
		{
			return TEXT("The map checks out with nothing to report.");
		}
		return FString::Printf(TEXT("%d error(s) and %d warning(s). See the Output Log for each one."), Errors, Warnings);
	}

	void Notify(const FString& Message)
	{
		FNotificationInfo Info(FText::FromString(Message));
		Info.ExpireDuration = 6.f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void FB4DMapKitModule::CheckCampaign()
{
	AB4DCampaign* Campaign = FindEditorCampaign();
	const TArray<FB4DProblem> Problems = UB4DExporter::Validate(Campaign);
	const FString Summary = ReportProblems(Problems);
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Summary));
}

void FB4DMapKitModule::ExportCampaign()
{
	AB4DCampaign* Campaign = FindEditorCampaign();
	if (!Campaign)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoCampaign", "This level has no B4D Campaign actor. Drop one in to start a map."));
		return;
	}

	IDesktopPlatform* Platform = FDesktopPlatformModule::Get();
	if (!Platform)
	{
		return;
	}

	TArray<FString> Chosen;
	const bool bPicked = Platform->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Export campaign JSON"),
		FPaths::ProjectSavedDir(),
		Campaign->Id + TEXT(".json"),
		TEXT("Campaign JSON|*.json"),
		EFileDialogFlags::None,
		Chosen);

	if (!bPicked || Chosen.Num() == 0)
	{
		return;
	}

	TArray<FB4DProblem> Problems;
	const bool bWritten = UB4DExporter::ExportToFile(Campaign, Chosen[0], Problems);
	const FString Summary = ReportProblems(Problems);

	if (!bWritten)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(
			FString::Printf(TEXT("Nothing was written. %s"), *Summary)));
		return;
	}

	Notify(FString::Printf(TEXT("Exported %s. %s"), *Campaign->Id, *Summary));
}

void FB4DMapKitModule::CopyLivePreview()
{
	AB4DCampaign* Campaign = FindEditorCampaign();
	if (!Campaign)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoCampaignPreview", "This level has no B4D Campaign actor."));
		return;
	}
	FPlatformApplicationMisc::ClipboardCopy(*UB4DExporter::BuildLivePreviewSnippet(Campaign));
	Notify(TEXT("Copied. Paste it into the game's console, then restart the campaign."));
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FB4DMapKitModule, B4DMapKit)
