// Copyright 2022 The MathWorks, Inc.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "RoadRunnerCarlaDatasmithBPLibrary.generated.h"

class AOpenDriveActor;
class UStaticMesh;

/* 
*	CARLA Datasmith importer helpers.
*/
UCLASS()
class URoadRunnerCarlaDatasmithBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	// Write the OpenDRIVE contained in the metadata into a file
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Write OpenDRIVE", Keywords = "Datasmith CARLA Init"), Category = "RoadRunnerCarla")
	static void WriteOpenDrive(AActor* actor);

	// Generate routes from the OpenDRIVE actor passed in
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Setup Carla Scene", Keywords = "Datasmith CARLA Init"), Category = "RoadRunnerCarla")
	static void SetupCarlaScene(AActor* actor);

	// Moves meshes into the appropriate folder for segmentation
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Categorize Mesh", Keywords = "Datasmith CARLA Init"), Category = "RoadRunnerCarla")
	static FString CategorizeMesh(UStaticMesh* mesh);

	// The below functions are for in-development features

	// Set segmentation category during runtime based on metadata
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Categorize Actor", Keywords = "Datasmith CARLA Init"), Category = "RoadRunnerCarla")
	static void CategorizeActor(AActor* actor);

	// Try to find the opendrive path based on original datasmith path copy to maps folder
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Initialize Roads", Keywords = "Datasmith CARLA Init"), Category = "RoadRunnerCarla")
	static void InitializeRoads(AOpenDriveActor* actor, const FString& filePath);

	// Get all descendant actors that have segmentation data
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Segmented Actors", Keywords = "Datasmith CARLA Init"), Category = "RoadRunnerCarla")
	static TArray<AActor*>  GetSegmentedActors(AActor* actor);
};
