// Copyright 2020 The MathWorks, Inc.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "RoadRunnerImporterOptions.generated.h"

////////////////////////////////////////////////////////////////////////////////
// Options for the RoadRunner importer plugin.
UCLASS(config=EditorPerProjectUserSettings, HideCategories=Object, MinimalAPI)
class URoadRunnerImporterOptions : public UObject
{
	GENERATED_UCLASS_BODY()

	// Determines if the plugin will create new materials using the rrdata
	// or use the options from subsequent FBX scene import option window
	UPROPERTY(EditAnywhere, config, category = ImportOptions)
	uint32 OverwriteMaterials : 1;

	// Determines if the plugin should import traffic signal visuals.
	// Currently only compatible with the import as Blueprint option.
	UPROPERTY(EditAnywhere, config, category=ImportOptions)
	uint32 ImportSignals : 1;
};
