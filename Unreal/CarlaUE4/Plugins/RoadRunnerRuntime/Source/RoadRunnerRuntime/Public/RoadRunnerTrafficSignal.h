// Copyright 2020 The MathWorks, Inc.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"
#include "Components/MeshComponent.h"
#include "RoadRunnerTrafficJunction.h"
#include "RoadRunnerTrafficSignal.generated.h"

class URoadRunnerTrafficController;
class USCS_Node;

////////////////////////////////////////////////////////////////////////////////
// Handles single signal to be controlled separately
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ROADRUNNERRUNTIME_API URoadRunnerTrafficSignal : public USceneComponent
{
	GENERATED_BODY()

public:

	// Holds list of Signal states
	UPROPERTY(EditAnywhere, Category = "Traffic")
	TArray<FSignalState> SignalStates;

	// Sets default values for this component's properties
	URoadRunnerTrafficSignal();

	UPROPERTY(EditAnywhere, Category = "Traffic")
	FString Id;

	UPROPERTY(EditAnywhere, Category = "Traffic")
	FString AssetId;


#if WITH_EDITOR
	void LegacyInit(FString signalId, URoadRunnerTrafficController* controller, FString assetId, const TMap<FString, USCS_Node*>& blueprintMap);
	void Init(FString signalId, URoadRunnerTrafficController* controller, FString assetId);
#endif

	void AddSignalState(const FSignalState& signalState);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetConfiguration(int index);

	bool ErrorFlag;
};
