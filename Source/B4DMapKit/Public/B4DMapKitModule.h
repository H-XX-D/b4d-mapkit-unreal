#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FB4DMapKitModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
#if WITH_EDITOR
	/** Adds the Blox 4 Dead entries to the level editor's Tools menu. */
	void RegisterMenus();

	/** Checks the level and reports what it found, without writing anything. */
	static void CheckCampaign();

	/** Checks the level, then asks where to save and writes the campaign JSON. */
	static void ExportCampaign();

	/** Puts the live-preview snippet on the clipboard, to paste into the game console. */
	static void CopyLivePreview();
#endif
};
