// Copyright 2020 The MathWorks, Inc.

using UnrealBuildTool;


////////////////////////////////////////////////////////////////////////////////
// Build rules for the RoadRunnerCarlaIntegration plugin.
public class RoadRunnerCarlaIntegration : ModuleRules
{
#if WITH_FORWARDED_MODULE_RULES_CTOR
	public RoadRunnerCarlaIntegration(ReadOnlyTargetRules Target) : base(Target) // 4.16 or later
#else
	public RoadRunnerCarlaIntegration(TargetInfo Target) // 4.15 or before
#endif
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Carla",
				"RoadRunnerImporter"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"XmlParser",
				"Carla",
				"RoadRunnerImporter"
			}
			);

#if WITH_FORWARDED_MODULE_RULES_CTOR
		PublicDefinitions.Add("ASIO_NO_EXCEPTIONS");
		PublicDefinitions.Add("BOOST_NO_EXCEPTIONS");
		PublicDefinitions.Add("LIBCARLA_NO_EXCEPTIONS");
		PublicDefinitions.Add("PUGIXML_NO_EXCEPTIONS");
#else
		Definitions.Add("ASIO_NO_EXCEPTIONS");
		Definitions.Add("BOOST_NO_EXCEPTIONS");
		Definitions.Add("LIBCARLA_NO_EXCEPTIONS");
		Definitions.Add("PUGIXML_NO_EXCEPTIONS");
#endif
	}
}
