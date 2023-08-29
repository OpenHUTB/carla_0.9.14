// Copyright 2020 The MathWorks, Inc.

#pragma once

#include "CoreMinimal.h"
#include "UnrealEd.h"
#include "Factories.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"
#include "Factories/FbxSceneImportFactory.h"
#include "RoadRunnerFbxSceneImportFactory.generated.h"

////////////////////////////////////////////////////////////////////////////////
// Import with UFbxSceneImportFactory using the post asset import delegates
// defined in RoadRunnerImporter.
UCLASS(hidecategories = Object)
class ROADRUNNERIMPORTER_API URoadRunnerFbxSceneImportFactory : public UFbxSceneImportFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Transient)
	class URoadRunnerImporterOptions* RoadRunnerImportOptions;

	// UFactory Interface
	virtual UObject* FactoryCreateFile(UClass* inClass, UObject* inParent, FName inName, EObjectFlags flags, const FString& filename, const TCHAR* parms, FFeedbackContext* warn, bool& outCancelled) override;
	virtual bool FactoryCanImport(const FString& filename) override;

public:
	bool OuterCall = true;
	TArray<UStaticMesh*> ImportedMeshes;
};
