// Copyright 2022 The MathWorks, Inc.

#pragma once

#include "Modules/ModuleManager.h"

class FRoadRunnerCarlaDatasmithModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
