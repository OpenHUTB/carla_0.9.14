// Copyright 2021 The MathWorks, Inc.

#include "RoadRunnerDatasmithBPLibrary.h"
#include "RoadRunnerDatasmith.h"
#include "RoadRunnerTrafficController.h"
#include "RoadRunnerTrafficUtil.h"

#include "DatasmithAssetUserData.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/MetaData.h"
#include <Runtime/XmlParser/Public/XmlFile.h>

URoadRunnerTrafficController* URoadRunnerDatasmithBPLibrary::CurrController;

////////////////////////////////////////////////////////////////////////////////

URoadRunnerDatasmithBPLibrary::URoadRunnerDatasmithBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

////////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void URoadRunnerDatasmithBPLibrary::SetLodThresholds(UStaticMesh* mesh)
{
	mesh->bAutoComputeLODScreenSize = false;

	auto datasmithUserData = mesh->GetAssetUserData<UDatasmithAssetUserData>();

	if (!datasmithUserData)
		return;

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
	if (!mesh->RenderData.IsValid())
#else // 4.27+
	if (!mesh->GetRenderData()->IsInitialized())
#endif
		return;

	int lodIdx = 0;
	// LOD0 starts at 1.0
	if (mesh->IsSourceModelValid(lodIdx))
	{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
		mesh->RenderData->ScreenSize[lodIdx].Default = 1.0f;
#else // 4.27+
		mesh->GetRenderData()->ScreenSize[lodIdx].Default = 1.0f;
#endif
		mesh->GetSourceModel(lodIdx).ScreenSize.Default = 1.0f;
	}

	lodIdx++;
	FString lodString = FString::Printf(TEXT("LOD%d"), lodIdx);
	FName lodName = FName(*lodString);
	while (FString* value = datasmithUserData->MetaData.Find(lodName))
	{
		float threshold = FCString::Atof(**value);

		if (mesh->IsSourceModelValid(lodIdx))
		{

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
			mesh->RenderData->ScreenSize[lodIdx].Default = 1.0f;
#else // 4.27+
			mesh->GetRenderData()->ScreenSize[lodIdx].Default = threshold;
#endif
			mesh->GetSourceModel(lodIdx).ScreenSize.Default = threshold;
		}

		lodIdx++;
		lodString = FString::Printf(TEXT("LOD%d"), lodIdx);
		lodName = FName(*lodString);
	}
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerDatasmithBPLibrary::LoadSignalConfigurations(AActor* actor)
{
	auto sceneComp = actor->GetRootComponent();
	if (!sceneComp)
		return;

	auto datasmithUserData = sceneComp->GetAssetUserData<UDatasmithAssetUserData>();
	if (!datasmithUserData)
		return;

	// Get signal config metadata
	FString* value = datasmithUserData->MetaData.Find("SignalConfigurations");
	if (!value)
		return;

	// Set traffic controller to be used by the rest of the import steps
	CurrController = NewObject<URoadRunnerTrafficController>(actor);
	CurrController->SetupAttachment(sceneComp);

	// XML parsing
	FXmlFile* rrXml = new FXmlFile(*value, EConstructMethod::ConstructFromBuffer);
	if (!rrXml->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse xml."));
		return;
	}
	FXmlNode* xmlSignalConfigs = rrXml->GetRootNode();

	// Create map of signal assets from metadata
	TMap<FString, FSignalAsset> uuidToSignalAssetMap;
	FRoadRunnerTrafficUtil::LoadSignalAssets(xmlSignalConfigs, uuidToSignalAssetMap);

	// These configs will be accessed later when creating signals.
	CurrController->Init(uuidToSignalAssetMap);
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerDatasmithBPLibrary::SetUpSignal(AActor* actor)
{
	auto sceneComp = actor->GetRootComponent();
	if (!sceneComp)
		return;

	auto datasmithUserData = sceneComp->GetAssetUserData<UDatasmithAssetUserData>();
	if (!datasmithUserData)
		return;

	FString* signalId = datasmithUserData->MetaData.Find("SignalId");
	FString* signalAssetId = datasmithUserData->MetaData.Find("SignalAssetId");
	if (!signalId || !signalAssetId)
		return;

	URoadRunnerTrafficSignal* signalComp = NewObject<URoadRunnerTrafficSignal>(actor);
	signalComp->SetupAttachment(sceneComp);
	signalComp->Init(*signalId, CurrController, *signalAssetId);

	// Register with controller
	CurrController->AddSignal(*signalId, signalComp);

	// Check if OpenDrive data is included
	FString* openDriveId = datasmithUserData->MetaData.Find("OpenDriveId");
	if (!openDriveId)
		return;

	int odId = FCString::Atoi(**openDriveId);
	CurrController->AddOpenDriveIdMapping(odId, *signalId);
}

////////////////////////////////////////////////////////////////////////////////

void URoadRunnerDatasmithBPLibrary::SetUpJunctions(AActor* actor)
{
	auto sceneComp = actor->GetRootComponent();
	if (!sceneComp)
		return;

	auto datasmithUserData = sceneComp->GetAssetUserData<UDatasmithAssetUserData>();
	if (!datasmithUserData)
		return;

	FString* value = datasmithUserData->MetaData.Find("Signalization");
	if (!value)
		return;

	FXmlFile* rrXml = new FXmlFile(*value, EConstructMethod::ConstructFromBuffer);
	if (!rrXml->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse xml"));
		return;
	}
	FXmlNode* xmlSignalization = rrXml->GetRootNode();

	// Dev note: passing in empty maps since we don't need them
	TMap<FString, USCS_Node*> uuidToComponentMap;
	TMap<FString, FSignalAsset> uuidToSignalAssetMap;
	TArray<FJunction> junctions = FRoadRunnerTrafficUtil::LoadSignalJunctions(xmlSignalization, uuidToComponentMap, uuidToSignalAssetMap);

	for (const auto& junction : junctions)
	{
		auto junctionComp = NewObject<URoadRunnerTrafficJunction>(actor);
		junctionComp->SetupAttachment(sceneComp);
		junctionComp->SetController(CurrController);

		// Check that junctions actually has phase/interval data
		if (junction.SignalPhases.Num() == 0 || junction.SignalPhases[0].Intervals.Num() == 0)
			continue;

		junctionComp->SetPhases(junction);

		// Get all signals from the first interval
		auto interval = junction.SignalPhases[0].Intervals[0];
		for (const auto& signalState : interval.SignalStates)
		{
			CurrController->SignalToJunctionMap.Add(signalState.Id, junctionComp);
		}
	}
}

void URoadRunnerDatasmithBPLibrary::SetCollisions(UStaticMesh * mesh)
{
#if ENGINE_MAJOR_VERSION <= 4
	if (UBodySetup* bodySetup = mesh->BodySetup)
#else // 5+
	if (UBodySetup* bodySetup = mesh->GetBodySetup())
#endif 
	{
		bodySetup->CollisionTraceFlag = ECollisionTraceFlag(CTF_UseComplexAsSimple);
	}
}
#endif