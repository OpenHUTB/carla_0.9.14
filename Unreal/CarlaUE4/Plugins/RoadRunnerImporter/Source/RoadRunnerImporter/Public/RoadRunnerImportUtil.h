// Copyright 2020 The MathWorks, Inc.

#pragma once

#include "CoreMinimal.h"

class FXmlNode;
class UTexture;
class UMaterialInstanceConstant;

////////////////////////////////////////////////////////////////////////////////
// Helper functions for importing from RoadRunner metadata
class ROADRUNNERIMPORTER_API FRoadRunnerImportUtil
{
public:
	// Struct to hold material information parsed from the rrdata
	struct MaterialInfo
	{
		FString Name;
		FString DiffuseMap;
		FString NormalMap;
		FString SpecularMap;
		FString DiffuseColor;
		FString SpecularColor;
		FString SpecularFactor;
		FString TransparencyMap;
		FString TransparencyFactor;
		FString Roughness;
		FString Emission;
		FString EmissionColor;
		FString TextureScaleU;
		FString TextureScaleV;
		FString TwoSided;
		FString DrawQueue;
		FString ShadowCaster;
		FString IsDecal;
		FString SegmentationType;
	};

	// Parse materials from the rrdata.xml file
	static TArray<MaterialInfo> ParseMaterialXml(FXmlNode* matList);

	// Parse signal mapping from OpenDrive
	static TMap<int, FString> ParseOpenDriveSignals(FXmlNode* openDriveNode);

	static UTexture* FindTexture(FString absFilePath, FString packagePath);

	// Helpers to set material instance parameters
	static void SetTextureParameter(UMaterialInstanceConstant* material, const FName& paramName, const FString& baseFilePath, const FString& texturePath, const FString& packagePath);
	static void SetColorParameter(UMaterialInstanceConstant* material, const FName& paramName, const FString& colorString, float alphaVal);
	static void SetScalarParameter(UMaterialInstanceConstant* material, const FName& paramName, const FString& valueString);

	// Handle issue from old RoadRunner exports where full paths were exported
	static FString ConvertMaterialName(const FString mtlName);

	// Save files to be replaced in case re-import fails
	static bool TempFolderExists();
	static void CleanUpTempFolder();

	static void FixUpRedirectors();

	static const FString TempFolderPath;
};