// Copyright 2020 The MathWorks, Inc.

#include "RoadRunnerTrafficController.h"

////////////////////////////////////////////////////////////////////////////////

URoadRunnerTrafficController::URoadRunnerTrafficController()
{
	PrimaryComponentTick.bCanEverTick = true;
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerTrafficController::Init(const TMap<FString, FSignalAsset>& assetMap)
{
	SignalAssetMap = assetMap;
}

////////////////////////////////////////////////////////////////////////////////

const FSignalAsset& URoadRunnerTrafficController::GetSignalAsset(FString assetId)
{
	if (!SignalAssetMap.Contains(assetId))
	{
		UE_LOG(RoadRunnerTraffic, Error, TEXT("Could not find signal asset %s."), *assetId);
		FSignalAsset emptyAsset;
		SignalAssetMap.Emplace("", emptyAsset);
		return SignalAssetMap[""]; // Return empty asset to avoid crash
	}
	return SignalAssetMap[assetId];
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerTrafficController::AddSignal(FString id, URoadRunnerTrafficSignal * signal)
{
	Signals.Emplace(id, signal);
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerTrafficController::AddOpenDriveIdMapping(int odId, FString uuid)
{
	OpenDriveIdMap.Emplace(odId, uuid);
}

////////////////////////////////////////////////////////////////////////////////
// Called when the game starts
void URoadRunnerTrafficController::BeginPlay()
{
	Super::BeginPlay();

	// Backup for legacy import
	if (Signals.Num() == 0)
	{
		Signals.Empty();
		for (const auto& comp : GetOwner()->GetComponentsByClass(URoadRunnerTrafficSignal::StaticClass()))
		{
			auto signalComp = Cast<URoadRunnerTrafficSignal>(comp);
			Signals.Emplace(signalComp->Id, signalComp);
		}
	}
	if (SignalToJunctionMap.Num() == 0)
	{
		for (const auto& comp : GetOwner()->GetComponentsByClass(URoadRunnerTrafficJunction::StaticClass()))
		{
			auto juncComp = Cast<URoadRunnerTrafficJunction>(comp);
			Junctions.Emplace(juncComp->Id, juncComp);
		}
		for (const TPair<FString, FString>& kvp : SignalToJunctionIdMap)
		{
			SignalToJunctionMap.Emplace(kvp.Key, Junctions[kvp.Value]);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// Called every frame
void URoadRunnerTrafficController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Timer += DeltaTime;
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerTrafficController::SetSignalState(FString id, int config, bool manualControl)
{
	if (!SignalToJunctionMap.Contains(id))
	{
		UE_LOG(RoadRunnerTraffic, Warning, TEXT("Could not find junction for signal %s."), *id);
	}
	SignalToJunctionMap[id]->SetMode(manualControl);

	if (!Signals.Contains(id) || !Signals[id])
	{
		UE_LOG(RoadRunnerTraffic, Warning, TEXT("Failed to retrieve signal %s."), *id);
		return;
	}
	Signals[id]->SetConfiguration(config);
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerTrafficController::SetSignalStateOpenDrive(int odId, int config, bool manualControl)
{
	// TODO: handle case when doesn't exist
	if (!OpenDriveIdMap.Contains(odId))
	{
		UE_LOG(RoadRunnerTraffic, Warning, TEXT("Failed to find signal for id %d."), odId);
	}
	FString id = OpenDriveIdMap[odId];
	SetSignalState(id, config, manualControl);
}