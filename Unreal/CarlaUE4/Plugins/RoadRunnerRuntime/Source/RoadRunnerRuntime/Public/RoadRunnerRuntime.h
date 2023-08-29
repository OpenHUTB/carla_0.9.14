// Copyright 2020 The MathWorks, Inc.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Engine.h"

////////////////////////////////////////////////////////////////////////////////
// Plugin module for runtime components of the RoadRunnerImporter.
//   - Contains support for runtime traffic signals
class FRoadRunnerRuntimeModule : public IModuleInterface
{
public:
	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
