// Copyright 2020 The MathWorks, Inc.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

////////////////////////////////////////////////////////////////////////////////
// Slate style for the RoadRunner importer plugin UI
class FRoadRunnerStyle
{
public:
	// Setup and cleanup functions
	static void Initialize();
	static void Shutdown();

	// Reloads textures used by slate renderer
	static void ReloadTextures();

	// The Slate style set for the plugin
	static const ISlateStyle& Get();

	// Helper function to get the brush for the RoadRunner logo
	static const FSlateBrush* GetLogoBrush();

	static FName GetStyleSetName();

private:
	static TSharedRef< class FSlateStyleSet > Create();
	static TSharedPtr< class FSlateStyleSet > StyleInstance;
};
