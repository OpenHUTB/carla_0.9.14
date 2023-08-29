// Copyright 2020 The MathWorks, Inc.

#include "RoadRunnerTrafficUtil.h"
#include "RoadRunnerRuntimeLog.h"

#include <Runtime/XmlParser/Public/XmlFile.h>

////////////////////////////////////////////////////////////////////////////////
// Get the modifed light bulb name by its original name under a given scene
// component node.
FString FRoadRunnerTrafficUtil::FindByNamePrefix(USCS_Node* parent, const FString& prefix)
{
	for (const auto& child : parent->GetChildNodes())
	{
		FString componentName = child->GetVariableName().ToString();

		FString regex = FString(TEXT("^"));
		regex += prefix;
		regex += FString(TEXT("(_[0-9]+)?.*Node.*"));
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
// Parses "LightState" element. Contains "Name" and "State".
FLightBulbState FRoadRunnerTrafficUtil::LoadLightBulbState(FXmlNode* lightStateNode)
{
	FLightBulbState lightBulbState;
	for (const auto& lightStateProperty : lightStateNode->GetChildrenNodes())
	{
		const FString& tag = lightStateProperty->GetTag();
		if (tag.Equals(TEXT("Name"), ESearchCase::CaseSensitive))
		{
			lightBulbState.Name = lightStateProperty->GetContent();
		}
		else if (tag.Equals(TEXT("State"), ESearchCase::CaseSensitive))
		{
			FString stateString = lightStateProperty->GetContent();
			lightBulbState.State = stateString.Equals(TEXT("true"), ESearchCase::CaseSensitive);
		}

	}
	return lightBulbState;
}

////////////////////////////////////////////////////////////////////////////////
// Parses "Configuration" element. Contains "Name" and multiple
// "LightState" elements.
FSignalConfiguration FRoadRunnerTrafficUtil::LoadSignalConfiguration(FXmlNode* configurationNode)
{
	FSignalConfiguration signalConfiguration;
	for (const auto& signalConfigurationProperty : configurationNode->GetChildrenNodes())
	{
		const FString& tag = signalConfigurationProperty->GetTag();
		if (tag.Equals(TEXT("Name"), ESearchCase::CaseSensitive))
		{
			signalConfiguration.Name = signalConfigurationProperty->GetContent();
		}
		else if (tag.Equals(TEXT("LightState"), ESearchCase::CaseSensitive))
		{
			FLightBulbState lightState = LoadLightBulbState(signalConfigurationProperty);
			signalConfiguration.LightBulbStates.Add(lightState);
		}
	}

	return signalConfiguration;
}

////////////////////////////////////////////////////////////////////////////////
// Parses "Signal" asset element. Contains "ID" and multiple
// "Configuration" elements.
FSignalAsset FRoadRunnerTrafficUtil::LoadSignalAsset(FXmlNode* signalNode)
{
	FSignalAsset signalAsset;
	for (const auto& signalAssetProperty : signalNode->GetChildrenNodes())
	{
		const FString& tag = signalAssetProperty->GetTag();
		if (tag.Equals(TEXT("ID"), ESearchCase::CaseSensitive))
		{
			signalAsset.Id = signalAssetProperty->GetContent();
		}
		else if (tag.Equals(TEXT("Configuration"), ESearchCase::CaseSensitive))
		{
			FSignalConfiguration signalConfiguration = LoadSignalConfiguration(signalAssetProperty);
			signalAsset.SignalConfigurations.Add(signalConfiguration);
		}
	}
	return signalAsset;
}

////////////////////////////////////////////////////////////////////////////////
// Load the signal assets into the map.
void FRoadRunnerTrafficUtil::LoadSignalAssets(FXmlNode* signalConfigNode, TMap<FString, FSignalAsset>& outUuidToSignalAssetMap)
{
	for (const auto& signalAssetsProperty : signalConfigNode->GetChildrenNodes())
	{
		const FString& tag = signalAssetsProperty->GetTag();
		if (tag.Equals(TEXT("Signal"), ESearchCase::CaseSensitive))
		{
			FSignalAsset signalAsset = LoadSignalAsset(signalAssetsProperty);
			outUuidToSignalAssetMap.Emplace(signalAsset.Id, signalAsset);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// Parses "Signal" state element. Contains the "ID" of the signal, the
// "SignalAsset" ID defined in SignalAssets, and the "State" for which
// configuration it is currently in.
FSignalState FRoadRunnerTrafficUtil::LoadSignalState(FXmlNode* signalStateNode, const TMap<FString, USCS_Node*>& uuidToComponentMap, const TMap<FString, FSignalAsset>& uuidToSignalAssetMap)
{
	FSignalState signalState;
	for (const auto& signalStateProperty : signalStateNode->GetChildrenNodes())
	{
		const FString& tag = signalStateProperty->GetTag();
		if (tag.Equals(TEXT("ID"), ESearchCase::CaseSensitive))
		{
			signalState.Id = signalStateProperty->GetContent();
		}
		else if (tag.Equals(TEXT("SignalAsset"), ESearchCase::CaseSensitive))
		{
			signalState.SignalAssetId = signalStateProperty->GetContent();
		}
		else if (tag.Equals(TEXT("ConfigurationIndex"), ESearchCase::CaseSensitive))
		{
			signalState.Configuration = FCString::Atoi(*signalStateProperty->GetContent());
		}
	}

	// TODO: short version can return here
	// HACK: check if map is empty for Datasmith
	if (uuidToComponentMap.Num() == 0)
		return signalState;

	// Find the signal configuration by the id
	if (uuidToSignalAssetMap.Contains(signalState.SignalAssetId))
	{
		UE_LOG(RoadRunnerRuntime, Warning, TEXT("Signal Asset %s could not be found."), *signalState.SignalAssetId);
		return signalState;
	}
	const FSignalAsset& signalAsset = uuidToSignalAssetMap[signalState.SignalAssetId];

	if (signalState.Configuration >= signalAsset.SignalConfigurations.Num() || signalState.Configuration < 0)
	{
		UE_LOG(RoadRunnerRuntime, Warning, TEXT("Signal Configuration for %s out of range."), *signalState.Id);
		return signalState;
	}
	const FSignalConfiguration& signalConfiguration = signalAsset.SignalConfigurations[signalState.Configuration];

	
	// Loop over light bulb states and set up the light instance states
	for (const auto& lightBulbState : signalConfiguration.LightBulbStates)
	{
		if (!uuidToComponentMap.Contains(signalState.Id))
		{
			UE_LOG(RoadRunnerRuntime, Warning, TEXT("Signal %s not found inside this blueprint."), *signalState.Id);
			continue;
		}
		FLightInstanceState lightInstanceState;
		lightInstanceState.ComponentName = FindByNamePrefix(uuidToComponentMap[signalState.Id], lightBulbState.Name);
		lightInstanceState.State = lightBulbState.State;
		signalState.LightInstanceStates.Add(lightInstanceState);
	}

	return signalState;
}

////////////////////////////////////////////////////////////////////////////////
// Parses "Interval" element. Contains the "Time" of its duration, and multiple
// "Signal" states.
FLightInterval FRoadRunnerTrafficUtil::LoadInterval(FXmlNode* intervalNode, const TMap<FString, USCS_Node*>& uuidToComponentMap, const TMap<FString, FSignalAsset>& uuidToSignalAssetMap)
{
	FLightInterval interval;
	for (const auto& intervalProperty : intervalNode->GetChildrenNodes())
	{
		const FString& tag = intervalProperty->GetTag();
		if (tag.Equals(TEXT("Time"), ESearchCase::CaseSensitive))
		{
			interval.Time = FCString::Atof(*intervalProperty->GetContent());
		}
		else if (tag.Equals(TEXT("Signal"), ESearchCase::CaseSensitive))
		{
			FSignalState signalState = LoadSignalState(intervalProperty, uuidToComponentMap, uuidToSignalAssetMap);
			interval.SignalStates.Add(signalState);
		}
	}
	return interval;
}

////////////////////////////////////////////////////////////////////////////////
// Parses "SignalPhase" element. Contains multiple "Interval" elements.
FSignalPhase FRoadRunnerTrafficUtil::LoadSignalPhase(FXmlNode* signalPhaseNode, const TMap<FString, USCS_Node*>& uuidToComponentMap, const TMap<FString, FSignalAsset>& uuidToSignalAssetMap)
{
	FSignalPhase signalPhase;
	for (const auto& signalPhaseProperty : signalPhaseNode->GetChildrenNodes())
	{
		const FString& tag = signalPhaseProperty->GetTag();
		if (tag.Equals(TEXT("Interval"), ESearchCase::CaseSensitive))
		{
			FLightInterval interval = LoadInterval(signalPhaseProperty, uuidToComponentMap, uuidToSignalAssetMap);
			signalPhase.Intervals.Add(interval);
		}
	}
	return signalPhase;
}

////////////////////////////////////////////////////////////////////////////////
// Parses "Junction" element. Contains its "ID" and multiple "SignalPhase" elements
FJunction FRoadRunnerTrafficUtil::LoadJunction(FXmlNode* junctionNode, const TMap<FString, USCS_Node*>& uuidToComponentMap, const TMap<FString, FSignalAsset>& uuidToSignalAssetMap)
{
	FJunction junction;
	for (const auto& junctionProperty : junctionNode->GetChildrenNodes())
	{
		const FString& tag = junctionProperty->GetTag();
		if (tag.Equals(TEXT("ID"), ESearchCase::CaseSensitive))
		{
			junction.Id = junctionProperty->GetContent();
		}
		else if (tag.Equals(TEXT("SignalPhase"), ESearchCase::CaseSensitive))
		{
			FSignalPhase signalPhase = LoadSignalPhase(junctionProperty, uuidToComponentMap, uuidToSignalAssetMap);
			junction.SignalPhases.Add(signalPhase);
		}

	}
	return junction;
}

////////////////////////////////////////////////////////////////////////////////
// Load the Junctions into the array.
TArray<FJunction> FRoadRunnerTrafficUtil::LoadSignalJunctions(FXmlNode* xmlSignalData, const TMap<FString, USCS_Node*>& uuidToComponentMap, const TMap<FString, FSignalAsset>& uuidToSignalAssetMap)
{
	TArray<FJunction> ret = TArray<FJunction>();

	if (!xmlSignalData)
		return ret;

	// "SignalData" has multiple Junction elements under it
	for (const auto& signalDataProperty : xmlSignalData->GetChildrenNodes())
	{
		const FString& tag = signalDataProperty->GetTag();
		if (tag.Equals(TEXT("Junction"), ESearchCase::CaseSensitive))
		{
			FJunction junction = LoadJunction(signalDataProperty, uuidToComponentMap, uuidToSignalAssetMap);
			ret.Add(junction);
		}
	}

	return ret;
}