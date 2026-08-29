#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "B4DMapKitTypes.h"
#include "B4DExporter.generated.h"

class AB4DCampaign;
class UWorld;

/**
 * Turns a level into campaign JSON the game loads directly, and checks it first
 * against the same rules the game applies when it loads a map.
 *
 * Everything here is callable from Blueprint, so a project can wire the export
 * into its own editor utility widget instead of using the kit's menu.
 */
UCLASS()
class B4DMAPKIT_API UB4DExporter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Finds the campaign root in the given world, or the first one in the editor world. */
	UFUNCTION(BlueprintCallable, Category = "Blox 4 Dead", meta = (WorldContext = "WorldContext"))
	static AB4DCampaign* FindCampaign(UObject* WorldContext);

	/**
	 * Checks the map. An error means it will not build or cannot be played; a
	 * warning means it builds but something is off, most often a hazard placed
	 * where no player can reach it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Blox 4 Dead")
	static TArray<FB4DProblem> Validate(AB4DCampaign* Campaign);

	/** Builds the campaign JSON. Does not check the map first. */
	UFUNCTION(BlueprintCallable, Category = "Blox 4 Dead")
	static FString BuildJson(AB4DCampaign* Campaign);

	/**
	 * Checks the map and, if it has no errors, writes the JSON to disk.
	 * Returns false and fills OutProblems when it refuses.
	 */
	UFUNCTION(BlueprintCallable, Category = "Blox 4 Dead")
	static bool ExportToFile(AB4DCampaign* Campaign, const FString& FilePath, TArray<FB4DProblem>& OutProblems);

	/** The one line to paste into the game's console to preview this map without a rebuild. */
	UFUNCTION(BlueprintCallable, Category = "Blox 4 Dead")
	static FString BuildLivePreviewSnippet(AB4DCampaign* Campaign);
};
