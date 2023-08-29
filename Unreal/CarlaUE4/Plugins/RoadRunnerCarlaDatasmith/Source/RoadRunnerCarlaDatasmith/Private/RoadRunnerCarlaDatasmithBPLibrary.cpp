// Copyright 2022 The MathWorks, Inc.

#include "RoadRunnerCarlaDatasmithBPLibrary.h"
#include "RoadRunnerCarlaDatasmith.h"

#include "DatasmithAssetUserData.h"
#include <OpenDrive/OpenDriveActor.h>

#include "Editor.h"
#include "Misc/FileHelper.h"

// CARLA folder names for segmentation
TMap<FString, FString> SegmentationMapCarla = {
	{"Uncategorized","None"}
	,{"Road","Road"}
	,{"Sidewalk","Sidewalk"}
	,{"Curb","Sidewalk"}
	,{"Gutter","Road"}
	,{"Marking","RoadLine"}
	,{"Ground","Ground"}
	,{"Building","Building"}
	,{"Vehicle","Vehicles"}
	,{"Bike","Vehicles"}
	,{"Pedestrian","Pedestrian"}
	,{"Sign","TrafficSign"}
	,{"Signal","TrafficLight"}
	,{"Foliage","Vegetation"}
	,{"Prop","Other"}
	,{"Bridge","Bridge"}
	,{"UNDEFINED","None"}
};

////////////////////////////////////////////////////////////////////////////////

URoadRunnerCarlaDatasmithBPLibrary::URoadRunnerCarlaDatasmithBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerCarlaDatasmithBPLibrary::WriteOpenDrive(AActor* actor)
{
	auto sceneComp = actor->GetRootComponent();
	if (!sceneComp)
		return;

	auto datasmithUserData = sceneComp->GetAssetUserData<UDatasmithAssetUserData>();

	if (!datasmithUserData)
	{
		UE_LOG(LogTemp, Log, TEXT("No datasmith data"));
		return;
	}

	FString* value = datasmithUserData->MetaData.Find("OpenDRIVE");
	if (!value)
	{
		UE_LOG(LogTemp, Log, TEXT("No OpenDRIVE data"));
		return;
	}

	// Get current level name
	FString mapName = GEditor->GetEditorWorldContext().World()->GetMapName();

	// File write path
	FString projectOpenDrivePath = FPaths::ProjectContentDir() + "Carla/Maps/OpenDrive";
	FString filesystemOpenDrivePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*projectOpenDrivePath) + "/" + mapName + ".xodr";

	FFileHelper::SaveStringToFile(*value, *filesystemOpenDrivePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerCarlaDatasmithBPLibrary::SetupCarlaScene(AActor* actor)
{
	AOpenDriveActor* openDriveActor = Cast<AOpenDriveActor>(actor);
	if (!openDriveActor)
	{
		UE_LOG(LogTemp, Log, TEXT("Could not get OpenDRIVE actor"));
		return;
	}

	FString mapName = GEditor->GetEditorWorldContext().World()->GetMapName();
	openDriveActor->BuildRoutes(FPaths::GetBaseFilename(mapName));
	openDriveActor->AddSpawners();
}


////////////////////////////////////////////////////////////////////////////////

FString URoadRunnerCarlaDatasmithBPLibrary::CategorizeMesh(UStaticMesh* mesh)
{
	auto datasmithUserData = mesh->GetAssetUserData<UDatasmithAssetUserData>();

	if (!datasmithUserData)
		return "";

	FString* value = datasmithUserData->MetaData.Find("MeshSegmentation");
	if (!value)
		return "";


	FString* namePtr = SegmentationMapCarla.Find(*value);
	FString folderName = namePtr ? *namePtr : "UNDEFINED";

	// Workaround for specific sign types
	if (value->EndsWith("Sign"))
	{
		folderName = "TrafficSign";
	}

	FString meshPackageName = mesh->GetOutermost()->GetName();

	// Create path from the original location, adding in an
	// extra folder at the end for the segmentation type.

	// Remove mesh name
	int idx;
	if (!meshPackageName.FindLastChar('/', idx))
		return "";
	FString meshPath = meshPackageName.RightChop(idx+1);
	meshPackageName = meshPackageName.Left(idx);

	// Get geometry folder name
	if (!meshPackageName.FindLastChar('/', idx))
		return "";
	FString geometryFolder = meshPackageName.RightChop(idx + 1);
	meshPackageName = meshPackageName.Left(idx);

	// Get map folder name
	if (!meshPackageName.FindLastChar('/', idx))
		return "";
	FString mapName = meshPackageName.RightChop(idx + 1);
	meshPackageName = meshPackageName.Left(idx);

	// Combine to get the new asset location
	return mapName / geometryFolder / folderName;
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerCarlaDatasmithBPLibrary::CategorizeActor(AActor* actor)
{
	auto sceneComp = actor->GetRootComponent();
	if (!sceneComp)
		return;

	auto datasmithUserData = sceneComp->GetAssetUserData<UDatasmithAssetUserData>();

	if (!datasmithUserData)
	{
		UE_LOG(LogTemp, Log, TEXT("No datasmith data"));
		return;
	}

	FString* value = datasmithUserData->MetaData.Find("ActorSegmentation");
	if (!value)
	{
		UE_LOG(LogTemp, Log, TEXT("No segmentation data"));
		return;
	}

	FString* namePtr = SegmentationMapCarla.Find(*value);
	FString folderName = namePtr ? *namePtr : "None";
	if (folderName.Equals("None") && value->EndsWith("Sign"))
		folderName = "TrafficSign";

	// Requires updating CARLA code to allow manually tagging actors
	//auto label = ATagger::GetLabelByFolderName(folderName);
	//ATagger::TagActor(*actor, true, label);
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerCarlaDatasmithBPLibrary::InitializeRoads(AOpenDriveActor* actor, const FString& filePath)
{
	auto odPath = FPaths::ChangeExtension(filePath, "xodr");

	if (!FPaths::FileExists(odPath))
		return;

	auto filename = FPaths::GetCleanFilename(odPath);

	FString projectOpenDrivePath = FPaths::ProjectContentDir() + "Carla/Maps/OpenDrive";

	FString filesystemOpenDrivePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*projectOpenDrivePath) + "/" + filename;

	if (!FPlatformFileManager::Get().GetPlatformFile().CopyFile(*filesystemOpenDrivePath, *odPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to copy OpenDrive to %s"), *filesystemOpenDrivePath);
	}

	actor->BuildRoutes(FPaths::GetBaseFilename(filename));
	actor->AddSpawners();
}

////////////////////////////////////////////////////////////////////////////////

TArray<AActor*> URoadRunnerCarlaDatasmithBPLibrary::GetSegmentedActors(AActor* actor)
{
	TArray<AActor*> ret;

	TArray<AActor*> traversalStack;
	traversalStack.Push(actor);
	while (traversalStack.Num() != 0)
	{
		AActor* currActor = traversalStack.Pop();

		TArray<AActor*> children;
		currActor->GetAttachedActors(children);
		for (AActor* child : children)
		{
			traversalStack.Push(child);
		}

		auto sceneComp = currActor->GetRootComponent();
		if (!sceneComp)
			continue;

		auto datasmithUserData = sceneComp->GetAssetUserData<UDatasmithAssetUserData>();
		if (!datasmithUserData)
			continue;

		FString* dataType = datasmithUserData->MetaData.Find("ActorSegmentation");
		if (dataType)
			ret.Add(currActor);
	}

	return ret;
}
