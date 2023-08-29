// Copyright 2020 The MathWorks, Inc.

#include "RoadRunnerStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Slate/SlateGameResources.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr< FSlateStyleSet > FRoadRunnerStyle::StyleInstance = NULL;

////////////////////////////////////////////////////////////////////////////////
// Create the singleton for the style
void FRoadRunnerStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Unregister the style and clear the singleton
void FRoadRunnerStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

////////////////////////////////////////////////////////////////////////////////

FName FRoadRunnerStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("RoadRunnerStyle"));
	return StyleSetName;
}

////////////////////////////////////////////////////////////////////////////////

const FSlateBrush* FRoadRunnerStyle::GetLogoBrush()
{
	return StyleInstance->GetBrush(TEXT("RoadRunner.Logo"));
}

////////////////////////////////////////////////////////////////////////////////
// Create the slate style set. Contains an image brush for the RoadRunner logo
TSharedRef< FSlateStyleSet > FRoadRunnerStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("RoadRunnerStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("RoadRunnerImporter")->GetBaseDir() / TEXT("Resources"));

	Style->Set("RoadRunner.Logo", new FSlateImageBrush(Style->RootToContentDir(TEXT("logo-mini"), TEXT(".png")), FVector2D(128.f, 128.f)));

	return Style;
}

////////////////////////////////////////////////////////////////////////////////

void FRoadRunnerStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

////////////////////////////////////////////////////////////////////////////////

const ISlateStyle& FRoadRunnerStyle::Get()
{
	return *StyleInstance;
}

////////////////////////////////////////////////////////////////////////////////
