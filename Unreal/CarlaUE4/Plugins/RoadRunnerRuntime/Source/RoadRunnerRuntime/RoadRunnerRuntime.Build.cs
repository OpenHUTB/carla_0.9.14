// Copyright 2020 The MathWorks, Inc.

using System.IO;
using UnrealBuildTool;

////////////////////////////////////////////////////////////////////////////////
// Build rules for the runtime components of the RoadRunnerImporter
public class RoadRunnerRuntime : ModuleRules
{
#if WITH_FORWARDED_MODULE_RULES_CTOR
	public RoadRunnerRuntime(ReadOnlyTargetRules Target) : base(Target) // 4.16 or later
#else
	public RoadRunnerRuntime(TargetInfo Target) // 4.15 or before
#endif
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"XmlParser",
			}
			);
	}
}
