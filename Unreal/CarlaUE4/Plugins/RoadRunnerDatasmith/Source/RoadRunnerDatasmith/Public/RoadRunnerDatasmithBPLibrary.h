// Copyright 2021 The MathWorks, Inc.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "RoadRunnerDatasmithBPLibrary.generated.h"

class UStaticMesh;
class URoadRunnerTrafficController;


UCLASS()
class URoadRunnerDatasmithBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set LOD Thresholds", Keywords = "RoadRunnerDatasmith LOD threshold"), Category = "RoadRunnerDatasmith")
	static void SetLodThresholds(UStaticMesh* mesh);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Load Signal Configurations", Keywords = "RoadRunnerDatasmith Signal Configuration"), Category = "RoadRunnerDatasmith")
	static void LoadSignalConfigurations(AActor* actor);

	// NOTE: LoadSignalConfigurations must be called before SetUpSignal
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Up Signal", Keywords = "RoadRunnerDatasmith Signal Configuration"), Category = "RoadRunnerDatasmith")
	static void SetUpSignal(AActor* actor);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Up Junctions", Keywords = "RoadRunnerDatasmith Signal Configuration"), Category = "RoadRunnerDatasmith")
	static void SetUpJunctions(AActor* actor);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Collisions", Keywords = "RoadRunnerDatasmith Collision Complexity"), Category = "RoadRunnerDatasmith")
	static void SetCollisions(UStaticMesh* mesh);
#endif

	static URoadRunnerTrafficController* CurrController;
};
