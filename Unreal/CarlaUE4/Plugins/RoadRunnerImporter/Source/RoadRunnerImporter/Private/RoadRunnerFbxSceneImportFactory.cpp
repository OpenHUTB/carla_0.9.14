// Copyright 2020 The MathWorks, Inc.

#include "RoadRunnerFbxSceneImportFactory.h"
#include "RoadRunnerImporter.h"
#include "RoadRunnerImporterLog.h"
#include "RoadRunnerImporterOptions.h"
#include "RoadRunnerImporterOptionWindow.h"
#include "RoadRunnerImportUtil.h"

#include <Factories/Factory.h>
#include <Editor/MainFrame/Public/Interfaces/IMainFrameModule.h>
#include <Editor/UnrealEd/Public/Editor.h>
#include <Editor/UnrealEd/Public/EditorBuildUtils.h>
#include <ObjectTools.h>
#include <PackageTools.h>
#include <Runtime/Engine/Classes/PhysicsEngine/BodySetup.h>
#include <Runtime/Launch/Resources/Version.h>
#include <UnrealEd.h>

#if ENGINE_MAJOR_VERSION >= 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 21)
#include <Subsystems/ImportSubsystem.h>
#endif

#define LOCTEXT_NAMESPACE "RoadRunnerFBXSceneImportFactory"

////////////////////////////////////////////////////////////////////////////////
// Note: The "Import Into Level" button does not currently check for priority,
// so this factory will only be called through RoadRunnerFbxFactory.
URoadRunnerFbxSceneImportFactory::URoadRunnerFbxSceneImportFactory(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
	SupportedClass = UBlueprint::StaticClass();
	Formats.Add(TEXT("fbx;FBX meshes and animations"));
	ImportPriority = DefaultImportPriority + 1;

	RoadRunnerImportOptions = CreateDefaultSubobject<URoadRunnerImporterOptions>(TEXT("RoadRunnerImportOptions"), true);
}

////////////////////////////////////////////////////////////////////////////////
// Checks for metadata file, and runs our post asset import functions
// if it exists.
UObject * URoadRunnerFbxSceneImportFactory::FactoryCreateFile(UClass * inClass, UObject * inParent, FName inName, EObjectFlags flags, const FString & filename, const TCHAR * parms, FFeedbackContext * warn, bool & outCancelled)
{
	FString rrMetadataFile = FPaths::ChangeExtension(filename, ".rrdata.xml");

	// Only use our delegates if metadata file exists, and is valid.
	if (!FPaths::FileExists(rrMetadataFile))
	{
		return UFbxSceneImportFactory::FactoryCreateFile(inClass, inParent, inName, flags, filename, parms, warn, outCancelled);
	}

	FXmlFile* rrXml = new FXmlFile(rrMetadataFile);
	if (!rrXml->IsValid())
	{
		UE_LOG(RoadRunnerImporter, Warning, TEXT("RoadRunner metadata file not valid. Reverting to default scene import factory."));
		return UFbxSceneImportFactory::FactoryCreateFile(inClass, inParent, inName, flags, filename, parms, warn, outCancelled);
	}

	// Check if plugin version is out of date
	bool isOutOfDate = false;
	FXmlNode* xmlRoot = rrXml->GetRootNode();
	int metadataVersion = FCString::Atoi(*(xmlRoot->GetAttribute("Version")));
	if (metadataVersion > FRoadRunnerImporterModule::PluginVersion)
	{
		UE_LOG(RoadRunnerImporter, Warning, TEXT("%s has a version newer than the current plugin. Update the plugin if there are unexpected results."), *(rrMetadataFile));
		isOutOfDate = true;
	}

	// Ensure parent is valid through Map load
	inParent->AddToRoot();

	// Create RoadRunner Import Options window
	TSharedPtr<SWindow> ParentWindow;
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}
	TSharedRef<SWindow> Window = SNew(SWindow)
		.ClientSize(FVector2D(550.0f, 250.0f))
		.FocusWhenFirstShown(true)
		.SizingRule(ESizingRule::FixedSize)
		.Title(NSLOCTEXT("UnrealEd", "RRSceneImportOpionsTitle", "MathWorks RoadRunner Import Options"));
	TSharedPtr<SRoadRunnerImporterOptionWindow> roadRunnerImporterOptionWindow;

	Window->SetContent
	(
		SAssignNew(roadRunnerImporterOptionWindow, SRoadRunnerImporterOptionWindow)
		.OwnerWindow(Window)
		.FullPath(filename)
		.ImportOptions(RoadRunnerImportOptions)
		.VersionOutOfDate(isOutOfDate)
	);

	FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

	if (!roadRunnerImporterOptionWindow->ShouldImport())
	{
		outCancelled = true;
		return nullptr;
	}

	RoadRunnerImportOptions->SaveConfig();

	FString FileExtension = FPaths::GetExtension(filename);
	ImportedMeshes.Empty();

	// Call pre-import delegate (For RoadRunner Carla integration plugin)
	// This variable is to prevent duplicate calls from UFbxSceneImportFactory::FactoryCreateFile
	OuterCall = true;
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 21
	FEditorDelegates::OnAssetPreImport.Broadcast(this, inClass, inParent, inName, *FPaths::GetExtension(filename));
#else
	GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetPreImport.Broadcast(this, inClass, inParent, inName, *FPaths::GetExtension(filename));
#endif
	OuterCall = false;

	// Disable LogFbx since it uses up too much memory
	GEngine->Exec(NULL, TEXT("Log LogFbx off"));

	// Import materials and model
	UObject* newObj = UFbxSceneImportFactory::FactoryCreateFile(inClass, inParent, inName, flags, filename, parms, warn, outCancelled);

	// Reset log levels
	GEngine->Exec(NULL, TEXT("Log reset"));

	// Set up materials and signals
	// Note: No longer goes through PostImport delegate
	FRoadRunnerImporterModule::RoadRunnerPostProcessing(this, newObj);

	FString packagePath = FPaths::GetPath(inParent->GetName()) + "/";

	// Use complex collisions and fix material references
	for (auto ItAsset = AllNewAssets.CreateIterator(); ItAsset; ++ItAsset)
	{
		UObject *AssetObject = ItAsset.Value();
		if (AssetObject)
		{
			if (AssetObject->IsA(UStaticMesh::StaticClass()))
			{
				//change to complex collisions
				UStaticMesh* staticMesh = Cast<UStaticMesh>(AssetObject);
				ImportedMeshes.Add(staticMesh);


#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
				staticMesh->BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
#else // 4.27+
				staticMesh->GetBodySetup()->CollisionTraceFlag = CTF_UseComplexAsSimple;
#endif

				if (RoadRunnerImportOptions->OverwriteMaterials)
				{

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
					for (auto& matref : staticMesh->StaticMaterials) {
#else // 4.27+
					for (auto& matref : staticMesh->GetStaticMaterials()) {
#endif
						// Re-create material package name from old material reference
						FString materialFullName = matref.ImportedMaterialSlotName.ToString();
						// Follow Unreal's naming scheme
						materialFullName = FRoadRunnerImportUtil::ConvertMaterialName(materialFullName);

						FString basePackageName = FPackageName::GetLongPackagePath(packagePath) / materialFullName;
						basePackageName = PackageTools::SanitizePackageName(basePackageName);

						matref.MaterialInterface = LoadObject<UMaterialInterface>(nullptr, *basePackageName);
					}
				}

				// Use full precision UVs for meshes that typically have world space UVs
				FString meshName = staticMesh->GetName();
				if (meshName.StartsWith("Terrain") || meshName.StartsWith("Road"))
				{

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 22
					for (auto& srcModel : staticMesh->SourceModels) {
#elif ENGINE_MAJOR_VERSION == 4
					for (auto& srcModel : staticMesh->GetSourceModels()) {
#else //ENGINE_MAJOR_VERSION == 5
					for (int i = 0; i < staticMesh->GetNumSourceModels(); i++) {
						auto& srcModel = staticMesh->GetSourceModel(i);
#endif
						srcModel.BuildSettings.bUseFullPrecisionUVs = true;
					}
				}

				// Add collisions to all sections for LOD 0

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 22
				auto& sectionInfoMap = staticMesh->SectionInfoMap;
#else
				auto& sectionInfoMap = staticMesh->GetSectionInfoMap();
#endif
				for (int i = 0; i < staticMesh->GetNumSections(0); ++i)
				{
					FMeshSectionInfo info = sectionInfoMap.Get(0, i);
					info.bEnableCollision = true;
					sectionInfoMap.Set(0, i, info);
				}

				// Set LOD thresholds
				TMap<FName, FString>* tagValues = UMetaData::GetMapForObject(AssetObject);
				if (tagValues)
				{
					if (tagValues->Contains("FBX.LodThresholds"))
					{
						auto thresholds = (*tagValues)["FBX.LodThresholds"];
						TArray<FString> thresholdArray;
						thresholds.ParseIntoArray(thresholdArray, TEXT(";"), true);

						staticMesh->bAutoComputeLODScreenSize = false;


#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
						if (staticMesh->RenderData.IsValid()) {
#else
						if (staticMesh->GetRenderData()->IsInitialized()) {
#endif

							// UE4 defines start render distance
							for (int i = 0; i < staticMesh->GetNumLODs(); i++)
							{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 22
								if (i == 0)
								{
									staticMesh->RenderData->ScreenSize[i] = 1.0f;
									staticMesh->SourceModels[i].ScreenSize = 1.0f;
									continue;
								}

								int thresholdIndex = i - 1;

								if (thresholdIndex < thresholdArray.Num())
								{
									auto threshold = FCString::Atof(*thresholdArray[thresholdIndex]) / 100.0f; // Convert from percentage
									staticMesh->RenderData->ScreenSize[i] = threshold;
									staticMesh->SourceModels[i].ScreenSize = threshold;
								}
#elif ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
								if (!staticMesh->IsSourceModelValid(i))
									continue;

								if (i == 0)
								{
									staticMesh->RenderData->ScreenSize[i].Default = 1.0f;
									staticMesh->GetSourceModel(i).ScreenSize.Default = 1.0f;
									continue;
								}

								int thresholdIndex = i - 1;

								if (thresholdIndex < thresholdArray.Num())
								{
									auto threshold = FCString::Atof(*thresholdArray[thresholdIndex]) / 100.0f; // Convert from percentage
									staticMesh->RenderData->ScreenSize[i].Default = threshold;
									staticMesh->GetSourceModel(i).ScreenSize.Default = threshold;
								}
#else //4.27+
								if (!staticMesh->IsSourceModelValid(i))
									continue;

								if (i == 0)
								{
									staticMesh->GetRenderData()->ScreenSize[i].Default = 1.0f;
									staticMesh->GetSourceModel(i).ScreenSize.Default = 1.0f;
									continue;
								}

								int thresholdIndex = i - 1;

								if (thresholdIndex < thresholdArray.Num())
								{
									auto threshold = FCString::Atof(*thresholdArray[thresholdIndex]) / 100.0f; // Convert from percentage
									staticMesh->GetRenderData()->ScreenSize[i].Default = threshold;
									staticMesh->GetSourceModel(i).ScreenSize.Default = threshold;
								}
#endif
							}
						}
					}
				}

				staticMesh->PreEditChange(NULL);
				staticMesh->PostEditChange();
			}
		}
	}

	// Rebuild world to force update materials
	FEditorBuildUtils::EditorBuild(GEditor->GetEditorWorldContext().World(), FBuildOptions::BuildGeometry);

	// Post-Import delegate for other plugins (RoadRunner Carla Integration plugin)
	OuterCall = true;

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 21
	FEditorDelegates::OnAssetPostImport.Broadcast(this, newObj);
#else
	GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetPostImport.Broadcast(this, newObj);
#endif

	OuterCall = false;

	inParent->RemoveFromRoot();

	return newObj;
}

////////////////////////////////////////////////////////////////////////////////

bool URoadRunnerFbxSceneImportFactory::FactoryCanImport(const FString& filename)
{
	const FString extension = FPaths::GetExtension(filename);

	if (extension == TEXT("fbx"))
	{
		return true;
	}
	return false;
}

#undef LOCTEXT_NAMESPACE