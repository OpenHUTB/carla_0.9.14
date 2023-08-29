// Copyright 2020 The MathWorks, Inc.

#include "RoadRunnerImporterOptions.h"

////////////////////////////////////////////////////////////////////////////////
// Default to generating RoadRuner materials and importing signal data.
URoadRunnerImporterOptions::URoadRunnerImporterOptions(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	OverwriteMaterials = true;
	ImportSignals = true;
}
