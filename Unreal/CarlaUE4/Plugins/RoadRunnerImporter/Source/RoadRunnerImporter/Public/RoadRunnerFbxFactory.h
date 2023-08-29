// Copyright 2020 The MathWorks, Inc.

#pragma once

#include "CoreMinimal.h"
#include "UnrealEd.h"
#include "Factories.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"
#include "Factories/FbxFactory.h"
#include "RoadRunnerFbxFactory.generated.h"

////////////////////////////////////////////////////////////////////////////////
// Attempts to import using the custom RoadRunner scene import factory.
// If it fails, go back to the default fbx importer.
// Note: This factory is needed to overwrite the normal fbx import with
// our scene importer since "File > Import Into Level..." does not account for
// import priority.
UCLASS(hidecategories = Object)
class ROADRUNNERIMPORTER_API URoadRunnerFbxFactory : public UFbxFactory
{
	GENERATED_UCLASS_BODY()

	// UFactory Interface
	virtual UObject* FactoryCreateFile(UClass* inClass, UObject* inParent, FName inName, EObjectFlags flags, const FString& filename, const TCHAR* parms, FFeedbackContext* warn, bool& outCancelled) override;
	virtual bool FactoryCanImport(const FString& filename) override;
};
