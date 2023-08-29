// Copyright 2020 The MathWorks, Inc.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/MeshComponent.h"
#include "RoadRunnerTrafficSignal.h"
#include "RoadRunnerTrafficJunction.h"
#include "Containers/Map.h"
#include "RoadRunnerTrafficController.generated.h"

////////////////////////////////////////////////////////////////////////////////
// Centralized place to set signal states from simulink
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ROADRUNNERRUNTIME_API URoadRunnerTrafficController : public USceneComponent
{
	GENERATED_BODY()

public:
	void Init(const TMap<FString, FSignalAsset>& assetMap);
	const FSignalAsset& GetSignalAsset(FString assetId);

	// Can't directly connect to component
	UPROPERTY(EditAnywhere, Category = "Traffic")
	TMap<FString, AActor*> IdToActor;

	// Holds a list of phases to loop through (i.e. Traffic going east/west -> Traffic going north/south -> Left Turns)
	UPROPERTY(EditAnywhere, Category = "Traffic")
	TMap<FString, URoadRunnerTrafficSignal*> Signals;

	UPROPERTY(EditAnywhere, Category = "Traffic")
	TMap<FString, FSignalAsset> SignalAssetMap;

	// Map so you can disable a whole junction
	UPROPERTY(EditAnywhere, Category = "Traffic")
	TMap<FString, URoadRunnerTrafficJunction*> SignalToJunctionMap;

	// Backup for legacy import
	UPROPERTY(EditAnywhere, Category = "Traffic")
	TMap<FString, FString> SignalToJunctionIdMap;

	// Sets default values for this component's properties
	URoadRunnerTrafficController();

	void AddSignal(FString id, URoadRunnerTrafficSignal* signalComp);

	// Map for OpenDRIVE ids to uuids
	UPROPERTY(EditAnywhere, Category = "Traffic")
	TMap<int, FString> OpenDriveIdMap;

	void AddOpenDriveIdMapping(int odId, FString uuid);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Signal State", Keywords = "RoadRunner Traffic Signal"), Category = "RoadRunnerRuntime")
	void SetSignalState(FString id, int config, bool manualControl = false);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Signal State OpenDRIVE", Keywords = "RoadRunner Traffic Signal"), Category = "RoadRunnerRuntime")
	void SetSignalStateOpenDrive(int odId, int config, bool manualControl = false);

private: 

	// Store mapping of junction IDs to components for legacy import
	UPROPERTY(EditAnywhere, Category = "Traffic")
	TMap<FString, URoadRunnerTrafficJunction*> Junctions;
	float Timer = 0.0f;
};
