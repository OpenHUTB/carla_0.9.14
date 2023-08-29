// Copyright 2020 The MathWorks, Inc.

using System.IO;
using UnrealBuildTool;

////////////////////////////////////////////////////////////////////////////////
// Build rules for the RoadRunner importer plugin.
public class RoadRunnerImporter : ModuleRules
{
#if WITH_FORWARDED_MODULE_RULES_CTOR
	public RoadRunnerImporter(ReadOnlyTargetRules Target) : base(Target) // 4.16 or later
#else
	public RoadRunnerImporter(TargetInfo Target) // 4.15 or before
#endif
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"InputCore",
				"RoadRunnerRuntime",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"InputCore",
				"Slate",
				"SlateCore",
				"XmlParser",
				"MaterialEditor",
				"EditorStyle",
				"Projects"
			}
			);
	}
}
