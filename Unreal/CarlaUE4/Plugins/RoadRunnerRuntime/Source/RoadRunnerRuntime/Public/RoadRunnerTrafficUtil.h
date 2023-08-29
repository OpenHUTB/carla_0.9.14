// Copyright 2020 The MathWorks, Inc.

#pragma once

#include "CoreMinimal.h"

#include "RoadRunnerTrafficJunction.h"
#include "Containers/Map.h"
#include <unordered_map>

class FXmlNode;
class USCS_Node;

////////////////////////////////////////////////////////////////////////////////
// Import helpers for traffic signals
class ROADRUNNERRUNTIME_API FRoadRunnerTrafficUtil
{

public:

	static FString FindByNamePrefix(USCS_Node* parent, const FString& prefix);
	static FLightBulbState LoadLightBulbState(FXmlNode* lightStateNode);
	static FSignalConfiguration LoadSignalConfiguration(FXmlNode* configurationNode);
	static FSignalAsset LoadSignalAsset(FXmlNode* signalNode);
	static void LoadSignalAssets(FXmlNode* signalConfigNode, TMap<FString, FSignalAsset>& outUuidToSignalAssetMap);
	static FSignalState LoadSignalState(FXmlNode* signalStateNode, const TMap<FString, USCS_Node*>& uuidToComponentMap, const TMap<FString, FSignalAsset>& uuidToSignalAssetMap);
	static FLightInterval LoadInterval(FXmlNode* intervalNode, const TMap<FString, USCS_Node*>& uuidToComponentMap, const TMap<FString, FSignalAsset>& uuidToSignalAssetMap);
	static FSignalPhase LoadSignalPhase(FXmlNode* signalPhaseNode, const TMap<FString, USCS_Node*>& uuidToComponentMap, const TMap<FString, FSignalAsset>& uuidToSignalAssetMap);
	static FJunction LoadJunction(FXmlNode* junctionNode, const TMap<FString, USCS_Node*>& uuidToComponentMap, const TMap<FString, FSignalAsset>& uuidToSignalAssetMap);
	static TArray<FJunction> LoadSignalJunctions(FXmlNode* xmlSignalData, const TMap<FString, USCS_Node*>& uuidToComponentMap, const TMap<FString, FSignalAsset>& uuidToSignalAssetMap);
};
