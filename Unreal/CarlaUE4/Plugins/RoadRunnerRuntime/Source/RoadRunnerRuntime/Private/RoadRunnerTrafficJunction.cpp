// Copyright 2020 The MathWorks, Inc.

#include "RoadRunnerTrafficJunction.h"
#include "Components/MeshComponent.h"

#include "RoadRunnerTrafficController.h"

DEFINE_LOG_CATEGORY(RoadRunnerTraffic);

////////////////////////////////////////////////////////////////////////////////
// Sets default values for this component's properties
URoadRunnerTrafficJunction::URoadRunnerTrafficJunction()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

////////////////////////////////////////////////////////////////////////////////
// Called when the game starts
void URoadRunnerTrafficJunction::BeginPlay()
{
	Super::BeginPlay();

	// Backup for legacy mode
	if (!Controller)
	{
		FComponentReference controllerRef;
		controllerRef.OtherActor = GetOwner();
		controllerRef.ComponentProperty = FName("RoadRunnerTrafficController");
		Controller = Cast<URoadRunnerTrafficController>(controllerRef.GetComponent(nullptr));
	}
}

////////////////////////////////////////////////////////////////////////////////
// Called every frame
void URoadRunnerTrafficJunction::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!AutoMode)
		return;

	Timer += DeltaTime;

	// Sanity check that phases have been set
	if (Phases.Num() == 0)
		return;
	FSignalPhase& signalPhase = GetCurrentPhase();

	if (signalPhase.Intervals.Num() == 0)
		return;

	FLightInterval& interval = GetCurrentInterval();

	if (Dirty)
	{
		// Set lightbulb state
		for (auto& signalState : interval.SignalStates)
		{
			Controller->SetSignalState(signalState.Id, signalState.Configuration);
		}

		Dirty = false;
	}

	// Update current state
	if (Timer > interval.Time)
	{
		Dirty = true;
		Timer = 0;
		CurrentInterval++;
		if (CurrentInterval >= signalPhase.Intervals.Num())
		{
			CurrentInterval = 0;
			CurrentPhase++;
			if (CurrentPhase >= Phases.Num())
			{
				CurrentPhase = 0;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

FSignalPhase& URoadRunnerTrafficJunction::GetCurrentPhase()
{
	return Phases[CurrentPhase];
}

////////////////////////////////////////////////////////////////////////////////

FLightInterval& URoadRunnerTrafficJunction::GetCurrentInterval()
{
	return GetCurrentPhase().Intervals[CurrentInterval];
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerTrafficJunction::SetController(URoadRunnerTrafficController* controller)
{
	Controller = controller;
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerTrafficJunction::SetPhases(FJunction importedData)
{
	// Early out if junction doesn't have phase/interval data
	if (importedData.SignalPhases.Num() == 0 || importedData.SignalPhases[0].Intervals.Num() == 0)
		return;

	Phases = importedData.SignalPhases;

	// Fill out list of all signal ids attached to this junction
	for (const auto& signalState : Phases[0].Intervals[0].SignalStates)
	{
		SignalIds.Add(signalState.Id);
	}
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerTrafficJunction::SetMode(bool manual)
{
	if (AutoMode == manual)
	{
		AutoMode = !manual;

		if (AutoMode)
		{
			// Reset phase
			CurrentPhase = 0;
			CurrentInterval = 0;
			Timer = 0;
		}
		else
		{
			Dirty = true;
		}
	}
}