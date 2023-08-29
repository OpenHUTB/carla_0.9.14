// Copyright 2020 The MathWorks, Inc.

#pragma once

#include "CoreMinimal.h"
#include "UnrealEd.h"
#include "Factories.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"
#include "Factories/FbxFactory.h"
#include "RoadRunnerFbxSceneImportFactory.h"
#include "RoadRunnerReimportFactory.generated.h"

////////////////////////////////////////////////////////////////////////////////
// Factory to redirect fbx scene re-import to go through the RoadRunnerFbxSceneImport
// again.
UCLASS(hidecategories = Object)
class ROADRUNNERIMPORTER_API URoadRunnerReimportFactory : public URoadRunnerFbxSceneImportFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()

	//~ Begin FReimportHandler Interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;
	//~ End FReimportHandler Interface

	//~ Begin UFactory Interface
	virtual bool FactoryCanImport(const FString& Filename) override;
	//~ End UFactory Interface

	// Asset processing delegates
	static void PostReimport(UObject* inCreateObject);
};
