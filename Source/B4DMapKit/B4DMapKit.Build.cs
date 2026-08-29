using UnrealBuildTool;

public class B4DMapKit : ModuleRules
{
	public B4DMapKit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "Json", "JsonUtilities"
		});

		// The exporter and its menu entry only exist in the editor. The actors
		// themselves stay in a runtime module so a level that uses them still
		// opens in a packaged build, even though maps are only ever authored.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd", "Slate", "SlateCore", "LevelEditor", "ToolMenus", "EditorStyle", "DesktopPlatform"
			});
		}
	}
}
