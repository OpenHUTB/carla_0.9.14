// Copyright 2020 The MathWorks, Inc.

#include "RoadRunnerRuntime.h"

#include <Modules/ModuleInterface.h>
#include <Modules/ModuleManager.h>

#define LOCTEXT_NAMESPACE "FRoadRunnerRuntimeModule"

////////////////////////////////////////////////////////////////////////////////

void FRoadRunnerRuntimeModule::StartupModule()
{
}

////////////////////////////////////////////////////////////////////////////////

void FRoadRunnerRuntimeModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRoadRunnerRuntimeModule, RoadRunnerRuntime)
