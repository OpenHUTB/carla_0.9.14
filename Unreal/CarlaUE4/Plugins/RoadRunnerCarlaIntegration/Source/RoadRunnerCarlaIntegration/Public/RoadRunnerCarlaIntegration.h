// Copyright 2020 The MathWorks, Inc.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include <UnrealEd.h>

////////////////////////////////////////////////////////////////////////////////
// Plugin to streamline the RoadRunner to CARLA process
//	- Creates new map from BaseMap to help with lighting
//	- Moves static meshes into specific folders for segmentation
//	- Creates materials instanced from CARLA's to work with weather
//	- Generates routes from OpenDRIVE
class FRoadRunnerCarlaIntegrationModule : public IModuleInterface
{
public:
	// Constants
	static const FString BaseMapPath;
	static const FString MapDestRelPath;
	static const FString OpenDriveDestRelPath;

	// CARLA segmentation path list
	static TArray<FString> PathList;
	// Segmentation enum
	enum SegmentationType
	{
		eRoad,
		eMarking,
		eTerrain,
		eProp,
		eSidewalk,
		eSign,
		eFoliage,
		eFence,
		ePole,
		eVehicle,
		eWall,
		eBuilding
	};

	// Import delegates
	static void RRCarlaPreImport(UFactory* inFactory, UClass* inClass, UObject* inParent, const FName& name, const TCHAR* type);
	static void RRCarlaPostImport(UFactory* inFactory, UObject* inCreateObject);

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
