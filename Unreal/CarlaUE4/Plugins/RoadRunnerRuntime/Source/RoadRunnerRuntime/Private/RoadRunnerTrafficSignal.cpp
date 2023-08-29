// Copyright 2020 The MathWorks, Inc.

#include "RoadRunnerTrafficSignal.h"
#include "RoadRunnerTrafficController.h"
#include "RoadRunnerTrafficUtil.h"
#include "Components/MeshComponent.h"
#include "RoadRunnerRuntimeLog.h"

////////////////////////////////////////////////////////////////////////////////

URoadRunnerTrafficSignal::URoadRunnerTrafficSignal()
{
	PrimaryComponentTick.bCanEverTick = true;
}

////////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void URoadRunnerTrafficSignal::LegacyInit(FString signalId, URoadRunnerTrafficController* controller, FString assetId, const TMap<FString, USCS_Node*>& uuidToComponentMap)
{
	Id = signalId;
	AssetId = assetId;

	const auto& signalAsset = controller->GetSignalAsset(AssetId);
	// Convert signal asset to signal state
	int index = 0;
	for (const auto& config : signalAsset.SignalConfigurations)
	{
		FSignalState signalState;
		signalState.Id = Id;
		signalState.SignalAssetId = AssetId;
		signalState.Configuration = index;

		// Loop over light bulb states and set up the light instance states
		for (const auto& lightBulbState : config.LightBulbStates)
		{
			if (!uuidToComponentMap.Contains(signalState.Id))
			{
				UE_LOG(RoadRunnerTraffic, Warning, TEXT("Signal %s not found inside this blueprint."), *signalState.Id);
				continue;
			}

			FLightInstanceState lightInstanceState;
			lightInstanceState.ComponentName = FRoadRunnerTrafficUtil::FindByNamePrefix(uuidToComponentMap[signalState.Id], lightBulbState.Name);
			lightInstanceState.State = lightBulbState.State;
			lightInstanceState.Ref.ComponentProperty = FName(*lightInstanceState.ComponentName);
			lightInstanceState.Ref.OtherActor = GetOwner();

			signalState.LightInstanceStates.Add(lightInstanceState);

		}
		AddSignalState(signalState);
		index++;
	}
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerTrafficSignal::Init(FString signalId, URoadRunnerTrafficController* controller, FString assetId)
{
	Id = signalId;
	AssetId = assetId;

	const auto& signalAsset = controller->GetSignalAsset(AssetId);
	// Convert signal asset to signal state
	int index = 0;
	for (const auto& config : signalAsset.SignalConfigurations)
	{
		FSignalState signalState;
		signalState.Id = Id;
		signalState.SignalAssetId = AssetId;
		signalState.Configuration = index;

		// Loop over light bulb states and set up the light instance states
		for (const auto& lightBulbState : config.LightBulbStates)
		{
			// Search here since we need to find the actor
			auto rootComp = GetOwner()->GetRootComponent();
			TArray< USceneComponent* > descendents;
			rootComp->GetChildrenComponents(true, descendents);

			for (const auto& descendent : descendents)
			{
				auto meshComp = Cast<UStaticMeshComponent>(descendent);

				if (!meshComp)
					continue; 

				// Check static mesh's owner name
				FString actorName = meshComp->GetOwner()->GetActorLabel();

				FString regex = FString(TEXT("^"));
				regex += lightBulbState.Name;
				regex += FString(TEXT(".*"));
				FRegexPattern pattern(regex);
				FRegexMatcher matcher(pattern, actorName);

				// Check if has match
				if (matcher.FindNext())
				{
					FLightInstanceState lightInstanceState;
					lightInstanceState.State = lightBulbState.State;
					lightInstanceState.Ref.ComponentProperty = FName("StaticMeshComponent");
					lightInstanceState.Ref.OtherActor = meshComp->GetOwner();

					signalState.LightInstanceStates.Add(lightInstanceState);
				}
			}

		}
		AddSignalState(signalState);
		index++;
	}


}
#endif

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerTrafficSignal::AddSignalState(const FSignalState & signalState)
{
	SignalStates.Add(signalState);
}

////////////////////////////////////////////////////////////////////////////////
// Called when the game starts
void URoadRunnerTrafficSignal::BeginPlay()
{
	Super::BeginPlay();
}

////////////////////////////////////////////////////////////////////////////////
// Called every frame
void URoadRunnerTrafficSignal::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerTrafficSignal::SetConfiguration(int index)
{
	if (index >= SignalStates.Num())
	{
		UE_LOG(RoadRunnerTraffic, Warning, TEXT("Invalid configuration %d for signal %s."), index, *Id);
		return;
	}

	// Set lightbulb state
	for (auto& lightInstanceState : SignalStates[index].LightInstanceStates)
	{
		// Find mesh component
		if (!lightInstanceState.MeshComponent)
		{
			// Legacy backup
			if (lightInstanceState.Ref.OtherActor == nullptr)
			{
				lightInstanceState.Ref.OtherActor = GetOwner();
			}
			lightInstanceState.MeshComponent = Cast<UMeshComponent>(lightInstanceState.Ref.GetComponent(nullptr));
		}

		if (lightInstanceState.MeshComponent)
		{
			lightInstanceState.MeshComponent->SetVisibility(lightInstanceState.State);
		}
		else
		{
			// Only print error once per instance
			if (!ErrorFlag)
			{
				UE_LOG(RoadRunnerTraffic, Error, TEXT("Light State not set up properly in %s."), *GetName());
				ErrorFlag = true;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
