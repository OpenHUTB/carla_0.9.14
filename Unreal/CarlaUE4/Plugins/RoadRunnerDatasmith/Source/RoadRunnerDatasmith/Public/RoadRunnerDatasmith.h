// Copyright 2021 The MathWorks, Inc.

#pragma once

#include "Modules/ModuleManager.h"

class FRoadRunnerDatasmithModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
