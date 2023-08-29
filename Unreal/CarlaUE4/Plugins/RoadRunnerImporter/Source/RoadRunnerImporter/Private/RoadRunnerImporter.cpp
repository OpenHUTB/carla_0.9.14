// Copyright 2020 The MathWorks, Inc.

#include "RoadRunnerImporter.h"
#include "RoadRunnerFbxSceneImportFactory.h"
#include "RoadRunnerImporterLog.h"
#include "RoadRunnerImporterOptions.h"
#include "RoadRunnerImportUtil.h"
#include "RoadRunnerReimportFactory.h"
#include "RoadRunnerStyle.h"
#include "RoadRunnerTrafficJunction.h"
#include "RoadRunnerTrafficSignal.h"
#include "RoadRunnerTrafficController.h"
#include "RoadRunnerTrafficUtil.h"

#include <AssetRegistryModule.h>
#include <Developer/AssetTools/Public/AssetToolsModule.h>
#include <Developer/AssetTools/Public/IAssetTools.h>
#include <Editor/MaterialEditor/Public/IMaterialEditor.h>
#include <Editor/UnrealEd/Public/Editor.h>
#include <Editor/UnrealEd/Public/Kismet2/KismetEditorUtilities.h>
#include <Factories/Factory.h>
#include <Factories/FbxFactory.h>
#include <LevelEditor.h>
#include <ObjectTools.h>
#include <PackageTools.h>
#include <Runtime/AssetRegistry/Public/ARFilter.h>
#include <Runtime/Core/Public/Internationalization/Regex.h>
#include <Runtime/Launch/Resources/Version.h>
#include <Runtime/XmlParser/Public/XmlFile.h>
#include <UnrealEd.h>
#include <UObject/GCObjectScopeGuard.h>

#include <unordered_map>



#if ENGINE_MAJOR_VERSION >= 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 21)
#include <Subsystems/ImportSubsystem.h>
#endif

#if ENGINE_MAJOR_VERSION >= 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 24)
#include "Layers/LayersSubsystem.h"
#else
#include <Editor/UnrealEd/Public/Layers/ILayers.h>
#endif

#define LOCTEXT_NAMESPACE "FRoadRunnerImporterModule"

namespace
{
	static const FString RoadRunnerExtension = ".rrdata.xml";


	////////////////////////////////////////////////////////////////////////////////
	// Get the modifed light bulb name by its original name under a given scene
	// component node.
	FString FindByNamePrefix(USCS_Node* parent, const FString& prefix)
	{
		for (const auto& child : parent->GetChildNodes())
		{
			FString componentName = child->GetVariableName().ToString();

			FString regex = FString(TEXT("^"));
			regex += prefix;
			regex += FString(TEXT("(_[0-9]+)?Node.*"));
			FRegexPattern pattern(regex);

			FRegexMatcher matcher(pattern, componentName);

			// Check if has match
			if (matcher.FindNext())
			{
				return componentName;
			}

			FString childCheck = FindByNamePrefix(child, prefix);

			if (!childCheck.IsEmpty())
			{
				return childCheck;
			}
		}
		return TEXT("");
	}

	////////////////////////////////////////////////////////////////////////////////
	// Replaces the created/existing Unreal material with a material instance from the
	// RoadRunnerMaterials plugin
	void CreateRoadRunnerMaterial(FString sourceFilePath, FString packagePath,
		FRoadRunnerImportUtil::MaterialInfo materialInfo, int metadataVersion, TMap<FString, int32>& materialToLayerMap)
	{
		FString materialFullName = materialInfo.Name;
		FString basePackageName = FPackageName::GetLongPackagePath(packagePath) / materialFullName;
		basePackageName = PackageTools::SanitizePackageName(basePackageName);

		// Create new package
		const FString Suffix(TEXT(""));

		// Re-create package for new material instance
#if ENGINE_MAJOR_VERSION <= 4
		UPackage* package = CreatePackage(NULL, *basePackageName);
#else // 5.0+
		UPackage* package = CreatePackage(*basePackageName);
#endif

		// Find our base material to instance from
		FText materialSearchError;
		UMaterialInterface* baseMaterial = nullptr;
		float alphaVal = 1.0f; // Default to opaque
		float transparency = 0.0f;
		if (!materialInfo.TransparencyFactor.IsEmpty())
		{
			transparency = FCString::Atof(*(materialInfo.TransparencyFactor));
		}
		bool isTransparent = !materialInfo.TransparencyMap.IsEmpty() || transparency > 0;


		FString materialName = "BaseMaterial";
		alphaVal = 1.0f - transparency;
		// Version 1 adds DrawQueue and ShadowCaster fields
		if (metadataVersion >= 1)
		{
			// Decals and markings always use decal material
			if (materialInfo.IsDecal.Equals("true") || !materialInfo.DrawQueue.Equals("0"))
			{
				materialName = "BaseDecalMaterial";
			}
			else if (isTransparent) // Choose base material based off transparency
			{
				if (materialInfo.TwoSided.Equals("true"))
				{
					if (materialInfo.ShadowCaster.Equals("false"))
					{
						// Translucent blend mode
						materialName = "BaseTransparentMaterialTwoSided";
					}
					else
					{
						// Masked blend mode
						materialName = "BaseCutoutMaterialTwoSided";
					}
				}
				else
				{
					if (materialInfo.ShadowCaster.Equals("false"))
					{
						// Translucent blend mode
						materialName = "BaseTransparentMaterial";
					}
					else
					{
						// Masked blend mode
						materialName = "BaseCutoutMaterial";
					}
				}
			}
		}
		else // Backwards compatibility
		{
			// Choose base material based off transparency
			if (isTransparent)
			{
				if (materialInfo.TwoSided.Equals("true"))
				{
					if (transparency > 0.0f)
					{
						// Translucent blend mode
						materialName = "BaseTransparentMaterialTwoSided";
					}
					else
					{
						// Masked blend mode
						materialName = "BaseCutoutMaterialTwoSided";
					}
				}
				else
				{
					if (transparency > 0.0f)
					{
						// Translucent blend mode
						materialName = "BaseTransparentMaterial";
					}
					else
					{
						// Masked blend mode
						materialName = "BaseCutoutMaterial";
					}
				}
			}
		}

		baseMaterial = UMaterialImportHelpers::FindExistingMaterialFromSearchLocation(materialName, "/RoadRunnerMaterials/", EMaterialSearchLocation::UnderParent, materialSearchError);

		if (!materialSearchError.IsEmpty() || baseMaterial == nullptr)
		{
			UE_LOG(RoadRunnerImporter, Error, TEXT("Base material not found: %s"), *(materialSearchError.ToString()));
			return;
		}

		// Create material instance from our base material
		// Modified: always create a material instance from our base material
		auto materialInstanceFactory = NewObject<UMaterialInstanceConstantFactoryNew>();
		materialInstanceFactory->InitialParent = baseMaterial;
		UMaterialInstanceConstant* unrealMaterial = (UMaterialInstanceConstant*)materialInstanceFactory->FactoryCreateNew(UMaterialInstanceConstant::StaticClass(), package, *materialFullName, RF_Standalone | RF_Public, NULL, GWarn);
		if (unrealMaterial == NULL)
		{
			UE_LOG(RoadRunnerImporter, Error, TEXT("Material %s could not be created."), *materialInfo.Name);
			return;
		}

		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(unrealMaterial);
		// Set the dirty flag so this package will get saved later
		package->SetDirtyFlag(true);

		// Set parameters
		// Modified: set parameters based off imported material info
		// Note: RoadRunner is in the middle of converting to a roughness-metallic setup, so some material may not look right
		FRoadRunnerImportUtil::SetTextureParameter(unrealMaterial, FName(TEXT("DiffuseMap")), sourceFilePath, materialInfo.DiffuseMap, packagePath);
		FRoadRunnerImportUtil::SetTextureParameter(unrealMaterial, FName(TEXT("NormalMap")), sourceFilePath, materialInfo.NormalMap, packagePath);
		FRoadRunnerImportUtil::SetTextureParameter(unrealMaterial, FName(TEXT("RoughnessMap")), sourceFilePath, materialInfo.SpecularMap, packagePath);

		FRoadRunnerImportUtil::SetColorParameter(unrealMaterial, FName(TEXT("DiffuseColor")), materialInfo.DiffuseColor, alphaVal);
		FRoadRunnerImportUtil::SetColorParameter(unrealMaterial, FName(TEXT("EmissionColor")), materialInfo.EmissionColor, 1);

		// Note: The default RoadRunner materials have not been tuned to the new setup yet
		// FRoadRunnerImportUtil::SetScalarParameter(unrealMaterial, FName(TEXT("RoughnessFactor")), materialInfo.Roughness); 
		FRoadRunnerImportUtil::SetScalarParameter(unrealMaterial, FName(TEXT("Emission")), materialInfo.Emission);
		FRoadRunnerImportUtil::SetScalarParameter(unrealMaterial, FName(TEXT("ScalingU")), materialInfo.TextureScaleU);
		FRoadRunnerImportUtil::SetScalarParameter(unrealMaterial, FName(TEXT("ScalingV")), materialInfo.TextureScaleV);

		// let the material update itself if necessary
		unrealMaterial->PreEditChange(nullptr);
		unrealMaterial->PostEditChange();

		int32 adjustedDrawQueue = FCString::Atoi(*materialInfo.DrawQueue);
		if (isTransparent && materialInfo.IsDecal.Equals("false"))
		{
			// Render other transparent objects after decals
			adjustedDrawQueue = FRoadRunnerImporterModule::TransparentRenderQueue;
		}
		materialToLayerMap.Add(materialFullName, adjustedDrawQueue);
	}

	void AssignSortPriority(UStaticMeshComponent* staticMeshComponent, TMap<FString, int32>& materialToLayerMap)
	{
		auto slotNames = staticMeshComponent->GetMaterialSlotNames();
		// check mesh has materials
		if (slotNames.Num() <= 0)
		{
			return;
		}

		// just get the first material to find the layer
		FName matName = slotNames[0];
		FString materialFullName = matName.ToString();
		// Follow Unreal's naming scheme
		materialFullName = FRoadRunnerImportUtil::ConvertMaterialName(materialFullName);

		int32* drawQueue = materialToLayerMap.Find(materialFullName);
		if (drawQueue != nullptr)
		{
			staticMeshComponent->SetTranslucentSortPriority(*drawQueue);
		}
	}

	void ImportOpenDriveMapping(URoadRunnerTrafficController* trafficController, const FString& openDriveFile)
	{
		if (!FPaths::FileExists(openDriveFile))
		{
			return;
		}

		FXmlFile* odXml = new FXmlFile(openDriveFile);
		if (!odXml->IsValid())
		{
			UE_LOG(RoadRunnerImporter, Error, TEXT("OpenDRIVE XML is invalid in: %s"), *(openDriveFile));
			return;
		}
		FXmlNode* odRoot = odXml->GetRootNode();

		auto openDriveMapping = FRoadRunnerImportUtil::ParseOpenDriveSignals(odRoot);
		for (const auto& kvp : openDriveMapping)
		{
			trafficController->AddOpenDriveIdMapping(kvp.Key, kvp.Value);
		}
	}

	void OverwriteMaterials(URoadRunnerFbxSceneImportFactory* roadrunnerFactory, UObject* inCreateObject, FXmlNode* xmlRoot)
	{
		if (!roadrunnerFactory->RoadRunnerImportOptions->OverwriteMaterials)
			return;

		FString srcPath = FPaths::GetPath(*roadrunnerFactory->GetCurrentFilename());
		FString packagePath = FPaths::GetPath(inCreateObject->GetPathName()) + "/";
		int metadataVersion = FCString::Atoi(*(xmlRoot->GetAttribute("Version")));

		FXmlNode* xmlMatList = xmlRoot->FindChildNode("MaterialList");
		if (!xmlMatList)
		{
			UE_LOG(RoadRunnerImporter, Error, TEXT("Material List not found in metadata for file: %s"), *(srcPath));
			return;
		}

		TMap<FString, int32> materialToLayerMap;

		auto matList = FRoadRunnerImportUtil::ParseMaterialXml(xmlMatList);

		// Delete old materials first
		TArray<UObject*> objectsToForceDelete;
		for (const auto& matInfo : matList)
		{
			FString materialFullName = matInfo.Name;
			FString basePackageName = FPackageName::GetLongPackagePath(packagePath) / materialFullName;
			basePackageName = PackageTools::SanitizePackageName(basePackageName);
			// Get the original material package and delete it
			UMaterial* oldMat = LoadObject<UMaterial>(nullptr, *basePackageName);
			if (oldMat != nullptr)
			{
				objectsToForceDelete.Add(oldMat);
			}
		}
		ObjectTools::ForceDeleteObjects(objectsToForceDelete, false);

		// Create RoadRunner materials
		for (const auto& matInfo : matList)
		{
			CreateRoadRunnerMaterial(srcPath, packagePath, matInfo, metadataVersion, materialToLayerMap);
		}

		// Check which import type was used
		UBlueprint* blueprint = Cast<UBlueprint>(inCreateObject);
		AActor* importedActor = GEditor->GetSelectedActors()->GetTop<AActor>();
		TArray<AActor*> childActors;
		if (importedActor) // Null when importing as actor with components
			importedActor->GetAttachedActors(childActors);

		if (blueprint)
		{
			blueprint->AddToRoot(); // Protect from garbage collection

			// Set sort priority in blueprint
			for (const auto& uscsnode : blueprint->SimpleConstructionScript->GetAllNodes())
			{
				if (!uscsnode->ComponentTemplate->IsA<UStaticMeshComponent>())
				{
					continue;
				}
				UStaticMeshComponent* staticMeshComponent = Cast<UStaticMeshComponent>(uscsnode->ComponentTemplate);
				AssignSortPriority(staticMeshComponent, materialToLayerMap);
			}
			blueprint->RemoveFromRoot();
		}
		else if (childActors.Num() != 0)
		{
			// Imported as level actors
			TArray<AActor*> traversalStack;
			traversalStack.Push(importedActor);
			while (traversalStack.Num() != 0)
			{
				AActor* currActor = traversalStack.Pop();

				TArray<AActor*> children;
				currActor->GetAttachedActors(children);
				for (AActor* child : children)
				{
					traversalStack.Push(child);
				}

				TArray<UStaticMeshComponent*> components;
				currActor->GetComponents<UStaticMeshComponent>(components);
				for (int32 i = 0; i < components.Num(); i++)
				{
					UStaticMeshComponent* staticMeshComponent = components[i];
					AssignSortPriority(staticMeshComponent, materialToLayerMap);
				}
			}
		}
	}

	void ImportSignals(URoadRunnerFbxSceneImportFactory* roadrunnerFactory, UObject* inCreateObject, FXmlNode* xmlRoot)
	{
		if (!roadrunnerFactory->RoadRunnerImportOptions->ImportSignals)
			return;

		int metadataVersion = FCString::Atoi(*(xmlRoot->GetAttribute("Version")));
		FXmlNode* xmlSignalConfigs = nullptr;
		FXmlNode* xmlJunctionData = nullptr;

		// Prior to version 3, both signal configurations and junction data were stored in the same element
		if (metadataVersion < 3)
		{
			if (FXmlNode* xmlSignalData = xmlRoot->FindChildNode("SignalData"))
			{
				xmlSignalConfigs = xmlSignalData->FindChildNode(TEXT("SignalAssets"));
				xmlJunctionData = xmlSignalData;
			}
		}
		else
		{
			xmlSignalConfigs = xmlRoot->FindChildNode("SignalConfigurations");
			xmlJunctionData = xmlRoot->FindChildNode("Signalization");
		}

		if (xmlSignalConfigs && xmlJunctionData)
		{
			TMap<FString, USCS_Node*> uuidToComponentMap;
			const FRegexPattern uuidPattern(TEXT("^[{(]?[0-9A-Fa-f]{8}[-]?([0-9A-Fa-f]{4}[-]?){3}[0-9A-Fa-f]{12}[)}]?"));

			// Check which import type was used
			UBlueprint* blueprint = Cast<UBlueprint>(inCreateObject);
			AActor* importedActor = GEditor->GetSelectedActors()->GetTop<AActor>();
			TArray<AActor*> childActors;
			if (importedActor) // Null when importing as actor with components
				importedActor->GetAttachedActors(childActors);


			if (blueprint)
			{
				blueprint->AddToRoot(); // Protect from garbage collection
				// Create uuid to component map
				for (const auto& uscsnode : blueprint->SimpleConstructionScript->GetAllNodes())
				{
					FString nodeName = uscsnode->GetVariableName().ToString();

					FRegexMatcher matcher(uuidPattern, nodeName);

					// Need to check name for uuid, then add uscs_node to a map
					if (matcher.FindNext())
					{
						// Only check first match
						FString match = matcher.GetCaptureGroup(0);

						if (uscsnode->GetChildNodes().Num() != 0)
						{
							uuidToComponentMap.Emplace(match, (uscsnode->GetChildNodes())[0]);
						}
					}
				}

				TMap<FString, FSignalAsset> uuidToSignalAssetMap;

				// Create map of signal assets from metadata
				FRoadRunnerTrafficUtil::LoadSignalAssets(xmlSignalConfigs, uuidToSignalAssetMap);


				// Add to blueprint
				AActor* dummyActor = NewObject<AActor>();
				dummyActor->AddToRoot();

				TArray<UActorComponent*> newComponents;
				URoadRunnerTrafficController* trafficController = NewObject<URoadRunnerTrafficController>(dummyActor);
				trafficController->AddToRoot();
				trafficController->Init(uuidToSignalAssetMap);
				newComponents.Add(trafficController);


				// Parse junction data from xml
				TArray<FJunction> junctions = FRoadRunnerTrafficUtil::LoadSignalJunctions(xmlJunctionData, uuidToComponentMap, uuidToSignalAssetMap);
				TMap<FString, FString> signalToSignalAssetMap;
				// Dev note: since these components are later converted to SCSNodes with AddComponentsToBlueprint,
				// component pointers set here will not be saved.
				for (const auto& junction : junctions)
				{
					// Early out if no signalization data
					if (junction.SignalPhases.Num() == 0 || junction.SignalPhases[0].Intervals.Num() == 0)
						continue;

					URoadRunnerTrafficJunction* component = NewObject<URoadRunnerTrafficJunction>(dummyActor);
					component->AddToRoot();
					component->SetPhases(junction);
					// Dev note: Not setting controller here, will be set during runtime
					newComponents.Add(component);

					// Set ID
					component->Id = junction.Id;
					// Get all signals from the first interval
					auto interval = junction.SignalPhases[0].Intervals[0];
					for (const auto& signalState : interval.SignalStates)
					{
						trafficController->SignalToJunctionIdMap.Add(signalState.Id, junction.Id);
						signalToSignalAssetMap.Emplace(signalState.Id, signalState.SignalAssetId);
					}
				}

				// Create traffic signal for each
				for (const auto& kvp : signalToSignalAssetMap)
				{
					URoadRunnerTrafficSignal* component = NewObject<URoadRunnerTrafficSignal>(dummyActor);
					component->LegacyInit(kvp.Key, trafficController, kvp.Value, uuidToComponentMap);
					// Dev note: Not registering with controller here, will be set during runtime
					newComponents.Add(component);
				}

				// Parse OpenDRIVE file for traffic signal id mapping
				FString openDriveFile = FPaths::ChangeExtension(roadrunnerFactory->GetCurrentFilename(), ".xodr");
				ImportOpenDriveMapping(trafficController, openDriveFile);


#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 24
				FKismetEditorUtilities::AddComponentsToBlueprint(blueprint, newComponents, false, (USCS_Node*)nullptr, true);
#elif ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 24
				FKismetEditorUtilities::AddComponentsToBlueprint(blueprint, newComponents, FKismetEditorUtilities::EAddComponentToBPHarvestMode::None, (USCS_Node*)nullptr, true);
#elif ENGINE_MAJOR_VERSION >= 5
				FKismetEditorUtilities::FAddComponentsToBlueprintParams params;
				params.bKeepMobility = true;
				FKismetEditorUtilities::AddComponentsToBlueprint(blueprint, newComponents, params);
#endif

				FKismetEditorUtilities::CompileBlueprint(blueprint);
				blueprint->MarkPackageDirty();
				blueprint->PreEditChange(nullptr);
				blueprint->PostEditChange();
				dummyActor->RemoveFromRoot();
				for (const auto& comp : newComponents)
				{
					comp->RemoveFromRoot();
				}

				blueprint->RemoveFromRoot();
			}
			else if (childActors.Num() != 0)
			{
				// Importing as Level actors

				// Load signals configs
				// Set traffic controller to be used by the rest of the import steps
				auto trafficController = NewObject<URoadRunnerTrafficController>(importedActor->GetRootComponent());
				// Add to SerializedComponents array so it gets saved
				importedActor->AddInstanceComponent(trafficController);
				trafficController->OnComponentCreated();
				trafficController->RegisterComponent();
				importedActor->Tags.Add(FName("TrafficController"));

				// Parse OpenDRIVE file for traffic signal id mapping
				FString openDriveFile = FPaths::ChangeExtension(roadrunnerFactory->GetCurrentFilename(), ".xodr");
				ImportOpenDriveMapping(trafficController, openDriveFile);

				// Create map of signal assets from metadata
				TMap<FString, FSignalAsset> uuidToSignalAssetMap;
				FRoadRunnerTrafficUtil::LoadSignalAssets(xmlSignalConfigs, uuidToSignalAssetMap);
				// These configs will be accessed later when creating signals.
				trafficController->Init(uuidToSignalAssetMap);

				// Loop through junctions to get signal to signal asset mappings and setup junctions
				TMap<FString, FString> signalToSignalAssetMap;
				TArray<FJunction> junctions = FRoadRunnerTrafficUtil::LoadSignalJunctions(xmlJunctionData, uuidToComponentMap, uuidToSignalAssetMap);
				for (const auto& junction : junctions)
				{
					// Early out if no signalization data
					if (junction.SignalPhases.Num() == 0 || junction.SignalPhases[0].Intervals.Num() == 0)
						continue;

					URoadRunnerTrafficJunction* junctionComp = NewObject<URoadRunnerTrafficJunction>(importedActor);
					// Add to SerializedComponents array so it gets saved
					importedActor->AddInstanceComponent(junctionComp);
					junctionComp->OnComponentCreated();
					junctionComp->RegisterComponent();

					junctionComp->SetController(trafficController);
					junctionComp->SetPhases(junction);
					junctionComp->Id = junction.Id;

					// Get all signals from the first interval
					auto interval = junction.SignalPhases[0].Intervals[0];
					for (const auto& signalState : interval.SignalStates)
					{
						trafficController->SignalToJunctionIdMap.Add(signalState.Id, junction.Id);
						signalToSignalAssetMap.Emplace(signalState.Id, signalState.SignalAssetId);
					}
				}

				// Run setup signal on all signals, search based on uuid
				// Loop over all children to find signs
				TArray<AActor*> traversalStack;
				traversalStack.Push(importedActor);
				while (traversalStack.Num() != 0)
				{
					AActor* currActor = traversalStack.Pop();

					TArray<AActor*> children;
					currActor->GetAttachedActors(children);
					for (AActor* child : children)
					{
						traversalStack.Push(child);
					}

					FString nodeName = UKismetSystemLibrary::GetDisplayName(currActor);

					FRegexMatcher matcher(uuidPattern, nodeName);

					// Need to check name for uuid, then add uscs_node to a map
					if (matcher.FindNext())
					{
						// Only check first match
						FString match = matcher.GetCaptureGroup(0);
						auto signalId = match;
						if (!signalId.StartsWith("{")) // capture group inconsistency with  5.0
							signalId = "{" + signalId + "}";

						// Check that id is a signal and not a sign
						if (!signalToSignalAssetMap.Contains(signalId))
							continue;

						URoadRunnerTrafficSignal* signalComp = NewObject<URoadRunnerTrafficSignal>(currActor);
						signalComp->SetupAttachment(currActor->GetRootComponent());
						// Add to SerializedComponents array so it gets saved
						currActor->AddInstanceComponent(signalComp);
						signalComp->OnComponentCreated();
						signalComp->RegisterComponent();

						signalComp->Init(signalId, trafficController, signalToSignalAssetMap[signalId]);

						// Register with controller
						trafficController->AddSignal(signalId, signalComp);
					}
				}

#if WITH_EDITOR
				if (importedActor->IsSelectedInEditor())
				{
					FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
					TArray<UObject*> List;
					List.Add(importedActor);

					LevelEditor.BroadcastActorSelectionChanged(List, true);
				}
#endif
			}
		}

	}

	void FinalizeScene(URoadRunnerFbxSceneImportFactory* roadrunnerFactory, UObject* inCreateObject)
	{
		if (!(roadrunnerFactory->RoadRunnerImportOptions->OverwriteMaterials
			|| roadrunnerFactory->RoadRunnerImportOptions->ImportSignals))
			return;

		// Blueprint specific setup
		UBlueprint* blueprint = Cast<UBlueprint>(inCreateObject);
		if (!blueprint)
		{
			return;
		}

		// Replace the original actor in the world with the updated blueprint
		AActor* origActor = GEditor->GetSelectedActors()->GetTop<AActor>();
		if (!origActor)
			return;

		UWorld* world = origActor->GetWorld();
		if (!world)
			return;

		// Deselect the original actor and Destroy it
		GEditor->SelectActor(origActor, false, false);

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 24
		GEditor->Layers->DisassociateActorFromLayers(origActor);
#else
		ULayersSubsystem* Layers = GEditor->GetEditorSubsystem<ULayersSubsystem>();
		Layers->DisassociateActorFromLayers(origActor);
#endif
		world->EditorDestroyActor(origActor, false);

		// Replace the actor
		AActor* newActor = world->SpawnActor(blueprint->GeneratedClass);
		newActor->Tags.Add("TrafficController");

		// Update selection to new actor
		GEditor->SelectActor(newActor, /*bSelected=*/ true, /*bNotify=*/ true);
	}
}

////////////////////////////////////////////////////////////////////////////////
// If fbx was imported through our scene importer, re-import the materials using
// the metadata file, then parse the signal metadata and attach traffic
// junctions components to the newly created blueprint.
void FRoadRunnerImporterModule::RoadRunnerPostProcessing(UFactory* inFactory, UObject* inCreateObject)
{
	if (!inCreateObject)
	{
		return;
	}
	if (!inFactory->IsA<URoadRunnerFbxSceneImportFactory>())
	{
		return;
	}
	auto roadrunnerFactory = Cast<URoadRunnerFbxSceneImportFactory>(inFactory);
	
	FString srcPath = FPaths::GetPath(*inFactory->GetCurrentFilename());
	FString packagePath = FPaths::GetPath(inCreateObject->GetPathName()) + "/";

	FString rrMetadataFile = FPaths::ChangeExtension(inFactory->GetCurrentFilename(), RoadRunnerExtension);
	if (!FPaths::FileExists(rrMetadataFile))
	{
		return;
	}

	FXmlFile* rrXml = new FXmlFile(rrMetadataFile);
	if (!rrXml->IsValid())
	{
		UE_LOG(RoadRunnerImporter, Error, TEXT("Metadata XML is invalid in: %s"), *(rrMetadataFile));
		return;
	}
	FXmlNode* xmlRoot = rrXml->GetRootNode();

	int metadataVersion = FCString::Atoi(*(xmlRoot->GetAttribute("Version")));

	OverwriteMaterials(roadrunnerFactory, inCreateObject, xmlRoot);

	ImportSignals(roadrunnerFactory, inCreateObject, xmlRoot);

	// Apply modifications to scene for blueprint import
	FinalizeScene(roadrunnerFactory, inCreateObject);

}

////////////////////////////////////////////////////////////////////////////////

void FRoadRunnerImporterModule::StartupModule()
{
	// Set up post-reimport delegate
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 21
	FEditorDelegates::OnAssetReimport.AddStatic(&URoadRunnerReimportFactory::PostReimport);
#else
	GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.AddStatic(&URoadRunnerReimportFactory::PostReimport);
#endif

	FRoadRunnerStyle::Initialize();
	FRoadRunnerStyle::ReloadTextures();

	if (FRoadRunnerImportUtil::TempFolderExists())
		UE_LOG(RoadRunnerImporter, Warning, TEXT("RoadRunnerImporter temporary folder was not cleaned up last time, check the RoadRunnerImporter content folder if you need to recover assets."));
}

////////////////////////////////////////////////////////////////////////////////

void FRoadRunnerImporterModule::ShutdownModule()
{
	FRoadRunnerImportUtil::CleanUpTempFolder();

	FRoadRunnerStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRoadRunnerImporterModule, RoadRunnerImporter)