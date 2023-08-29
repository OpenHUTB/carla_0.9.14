// Copyright 2022 The MathWorks, Inc.

using UnrealBuildTool;

public class RoadRunnerCarlaDatasmith : ModuleRules
{
	public RoadRunnerCarlaDatasmith(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
			);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
			});


		PrivateIncludePaths.AddRange(
			new string[] {
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"DatasmithContent",
				"Carla",
				"UnrealEd",
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
			});
	}
}
