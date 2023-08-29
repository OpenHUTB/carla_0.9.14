// Copyright 2020 The MathWorks, Inc.

#include "RoadRunnerImportUtil.h"
#include "RoadRunnerImporterLog.h"

#include <AssetRegistryModule.h>
#include <Developer/AssetTools/Public/AssetToolsModule.h>
#include <Developer/AssetTools/Public/IAssetTools.h>
#include <ObjectTools.h>
#include <PackageTools.h>
#include "HAL/PlatformFilemanager.h"
#include <Runtime/XmlParser/Public/XmlFile.h>

namespace
{
	// Get absolute path of temp folder
	FString GetTempFolderAbsolute()
	{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 17
		FString pluginsFolder = FPaths::GamePluginsDir();
#else //4.18+
		FString pluginsFolder = FPaths::ProjectPluginsDir();
#endif
		FString filesystemTempPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*(pluginsFolder + "RoadRunnerImporter/Content/TEMP"));

		return filesystemTempPath;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Helper function to parse the material info from the *.rrdata.xml file
TArray<FRoadRunnerImportUtil::MaterialInfo> FRoadRunnerImportUtil::ParseMaterialXml(FXmlNode * matList)
{
	TArray<MaterialInfo> retList;
	const auto& xmlMats = matList->GetChildrenNodes();

	for (const auto& mat : xmlMats)
	{
		const auto& matProperties = mat->GetChildrenNodes();

		// Fill out material info struct based off xml
		FRoadRunnerImportUtil::MaterialInfo matInfo;
		for (const auto& matProperty : matProperties)
		{
			const FString& tag = matProperty->GetTag();
			if (tag.Equals(TEXT("Name"), ESearchCase::CaseSensitive))
			{
				matInfo.Name = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("DiffuseMap"), ESearchCase::CaseSensitive))
			{
				matInfo.DiffuseMap = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("NormalMap"), ESearchCase::CaseSensitive))
			{
				matInfo.NormalMap = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("SpecularMap"), ESearchCase::CaseSensitive))
			{
				matInfo.SpecularMap = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("DiffuseColor"), ESearchCase::CaseSensitive))
			{
				matInfo.DiffuseColor = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("TransparentColor"), ESearchCase::CaseSensitive))
			{
				matInfo.TransparencyMap = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("TransparencyFactor"), ESearchCase::CaseSensitive))
			{
				matInfo.TransparencyFactor = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("SpecularColor"), ESearchCase::CaseSensitive))
			{
				matInfo.SpecularColor = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("SpecularFactor"), ESearchCase::CaseSensitive))
			{
				matInfo.SpecularFactor = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("Roughness"), ESearchCase::CaseSensitive))
			{
				matInfo.Roughness = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("Emission"), ESearchCase::CaseSensitive))
			{
				matInfo.Emission = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("EmissionColor"), ESearchCase::CaseSensitive))
			{
				matInfo.EmissionColor = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("TextureScaleU"), ESearchCase::CaseSensitive))
			{
				matInfo.TextureScaleU = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("TextureScaleV"), ESearchCase::CaseSensitive))
			{
				matInfo.TextureScaleV = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("TwoSided"), ESearchCase::CaseSensitive))
			{
				matInfo.TwoSided = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("DrawQueue"), ESearchCase::CaseSensitive))
			{
				matInfo.DrawQueue = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("ShadowCaster"), ESearchCase::CaseSensitive))
			{
				matInfo.ShadowCaster = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("IsDecal"), ESearchCase::CaseSensitive))
			{
				matInfo.IsDecal = matProperty->GetContent();
			}
			else if (tag.Equals(TEXT("AmbientColor")))
			{
				// Unused
			}
			else if (tag.Equals(TEXT("SegmentationType")))
			{
				matInfo.SegmentationType = matProperty->GetContent();
			}
			else
			{
				UE_LOG(RoadRunnerImporter, Warning, TEXT("Unrecognized element '%s' found in material property"), *tag);
			}
		}

		// Validation
		if (matInfo.Name.IsEmpty())
		{
			UE_LOG(RoadRunnerImporter, Warning, TEXT("Material is missing a name"));
			continue;
		}

		// Follow Unreal's naming scheme
		matInfo.Name = FRoadRunnerImportUtil::ConvertMaterialName(matInfo.Name);

		retList.Add(matInfo);
	}


	return retList;
}

TMap<int, FString> FRoadRunnerImportUtil::ParseOpenDriveSignals(FXmlNode* openDriveNode)
{
	TMap<int, FString> openDriveMapping;

	for (const auto& child : openDriveNode->GetChildrenNodes())
	{
		const FString& tag = child->GetTag();

		// Skip non-roads
		if (!tag.Equals("road", ESearchCase::CaseSensitive))
			continue;

		auto signalsNode = child->FindChildNode("signals");
		if (!signalsNode)
			continue;

		for (const auto& signal : signalsNode->GetChildrenNodes())
		{
			int openDriveId = FCString::Atoi(*signal->GetAttribute("id"));

			auto userDataNode = signal->FindChildNode("userData");
			if (!userDataNode)
				continue;

			auto vectorSignalNode = userDataNode->FindChildNode("vectorSignal");
			if (!vectorSignalNode)
				continue;

			FString uuid = vectorSignalNode->GetAttribute("signalId");

			openDriveMapping.Emplace(openDriveId, uuid);
		}
	}

	return openDriveMapping;
}

////////////////////////////////////////////////////////////////////////////////
// Find the texture based off the import path and filename
UTexture* FRoadRunnerImportUtil::FindTexture(FString absFilePath, FString packagePath)
{
	if (absFilePath.IsEmpty())
	{
		return nullptr;
	}

	// Name the texture with file name
	FString textureName = FPaths::GetBaseFilename(absFilePath);
	textureName = ObjectTools::SanitizeObjectName(textureName);

	// Set where to find the texture in the project
	FString basePackageName = FPackageName::GetLongPackagePath(packagePath) / textureName;
	basePackageName = PackageTools::SanitizePackageName(basePackageName);

	// Try to get the object from the package
	FString objectPath = basePackageName + TEXT(".") + textureName;
	UTexture2D* existingTexture = LoadObject<UTexture2D>(NULL, *objectPath, nullptr, LOAD_Quiet | LOAD_NoWarn);

	return existingTexture;
}

////////////////////////////////////////////////////////////////////////////////
// Helper function to set texture parameter in a material instance
void FRoadRunnerImportUtil::SetTextureParameter(UMaterialInstanceConstant* material,
	const FName& paramName, const FString& baseFilePath, const FString& texturePath, const FString& packagePath)
{
	if (texturePath.IsEmpty())
		return;

	FString texFileAbsPath = FPaths::ConvertRelativePathToFull(baseFilePath / texturePath);
	UTexture * texture = FindTexture(texFileAbsPath, packagePath);
	if (texture)
	{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 18
		material->SetTextureParameterValueEditorOnly(paramName, texture);
#else // 4.19+
		material->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(paramName, EMaterialParameterAssociation::GlobalParameter), texture);
#endif
	}
}

////////////////////////////////////////////////////////////////////////////////
// Helper function to set color parameter in a material instance
void FRoadRunnerImportUtil::SetColorParameter(UMaterialInstanceConstant* material,
	const FName& paramName, const FString& colorString, float alphaVal)
{
	if (colorString.IsEmpty())
	{
		return;
	}

	TArray<FString> colorStrings;
	int numElements = colorString.ParseIntoArray(colorStrings, TEXT(","), true);
	if (numElements != 3)
	{
		UE_LOG(RoadRunnerImporter, Error, TEXT("Error: %s's %s value is invalid"), *(material->GetFName().ToString()), *(paramName.ToString()));
		return;
	}
	float r = FCString::Atof(*(colorStrings[0]));
	float g = FCString::Atof(*(colorStrings[1]));
	float b = FCString::Atof(*(colorStrings[2]));

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 18
	material->SetVectorParameterValueEditorOnly(paramName, FLinearColor(r, g, b, alphaVal));
#else
	material->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(paramName, EMaterialParameterAssociation::GlobalParameter), FLinearColor(r, g, b, alphaVal));
#endif

}

////////////////////////////////////////////////////////////////////////////////
// Helper function to set scalar parameter in a material instance
void FRoadRunnerImportUtil::SetScalarParameter(UMaterialInstanceConstant* material, const FName& paramName, const FString& valueString)
{
	if (valueString.IsEmpty())
		return;

	float value = FCString::Atof(*valueString);

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 18
	material->SetScalarParameterValueEditorOnly(paramName, value);
#else
	material->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(paramName, EMaterialParameterAssociation::GlobalParameter), value);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Mimics how Unreal renames materials. Needed for older exports.
// Chops off chars up to the last ':' and replaces invalid characters with '_'
FString FRoadRunnerImportUtil::ConvertMaterialName(const FString mtlName)
{
	FString newName = mtlName;
	// Start after last colon
	int namespaceIdx;
	if (newName.FindLastChar(':', namespaceIdx))
	{
		newName = mtlName.RightChop(namespaceIdx);
	}

	// Replace invalid characters
	return ObjectTools::SanitizeObjectName(newName);
}

////////////////////////////////////////////////////////////////////////////////
// Deletes the temporary folder created during re-import to back up the old
// version of assets.
void FRoadRunnerImportUtil::CleanUpTempFolder()
{
	if (!TempFolderExists())
	{
		return;
	}

	FString filesystemTempPath = GetTempFolderAbsolute();
	if (!FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*filesystemTempPath))
		UE_LOG(RoadRunnerImporter, Warning, TEXT("Failed to clean up TEMP directory. Manual cleanup may be needed."));

}

////////////////////////////////////////////////////////////////////////////////
// Check that the temporary folder exists in case previous import crashed
bool FRoadRunnerImportUtil::TempFolderExists()
{
	FString filesystemTempPath = GetTempFolderAbsolute();
	return FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*filesystemTempPath);
}

////////////////////////////////////////////////////////////////////////////////
// Fix up all object redirectors in the project
void FRoadRunnerImportUtil::FixUpRedirectors()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Form a filter from the paths
	FARFilter filter;
	filter.bRecursivePaths = true;

	filter.PackagePaths.Emplace("/Game");
	filter.ClassNames.Emplace(TEXT("ObjectRedirector"));

	// Query for a list of assets in the selected paths
	TArray<FAssetData> assetList;
	AssetRegistryModule.Get().GetAssets(filter, assetList);

	if (assetList.Num() > 0)
	{
		TArray<UObjectRedirector*> redirectors;
		for (const auto& asset : assetList)
		{
			UE_LOG(RoadRunnerImporter, Log, TEXT("Cleaning up redirector: %s"), *asset.PackageName.ToString());
			UPackage* pkg = PackageTools::LoadPackage(asset.PackageName.ToString());
			UObjectRedirector* redirector = LoadObject<UObjectRedirector>(pkg, *asset.AssetName.ToString());
			if(redirector)
				redirectors.Add(redirector);

		}

		// Load the asset tools module
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		AssetToolsModule.Get().FixupReferencers(redirectors);
	}
}
