// Copyright 2020 The MathWorks, Inc.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateColor.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/Docking/TabManager.h"
#include "SlateFwd.h"

////////////////////////////////////////////////////////////////////////////////
// RoadRunner import option window
class SRoadRunnerImporterOptionWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRoadRunnerImporterOptionWindow)
		: _OwnerWindow()
		, _FullPath(TEXT(""))
		{}
	
		SLATE_ARGUMENT(TSharedPtr<SWindow>, OwnerWindow)
		SLATE_ARGUMENT(FString, FullPath)
		SLATE_ARGUMENT(class URoadRunnerImporterOptions*, ImportOptions)
		SLATE_ARGUMENT(bool, VersionOutOfDate)
	SLATE_END_ARGS()

public:

	SRoadRunnerImporterOptionWindow();
	~SRoadRunnerImporterOptionWindow();
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }

	void CloseFbxSceneOption();

	FReply OnImport()
	{
		ShouldImport_ = true;
		CloseFbxSceneOption();
		return FReply::Handled();
	}

	FReply OnCancel()
	{
		ShouldImport_ = false;
		CloseFbxSceneOption();
		return FReply::Handled();
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			return OnCancel();
		}

		return FReply::Unhandled();
	}

	bool ShouldImport() const
	{
		return ShouldImport_;
	}


private:
	bool CanCloseTab();

	bool CanImport() const;

	// Checkbox options
	ECheckBoxState ShouldOverwriteMaterials() const;
	void OnOverwriteMaterialsChanged(ECheckBoxState NewState);
	ECheckBoxState ShouldImportSignals() const;
	void OnImportSignalsChanged(ECheckBoxState NewState);

	// Display warning if plugin is out of date
	EVisibility GetVersionWarningVisibility()
	{
		if (VersionOutOfDate)
			return EVisibility::Visible;
		else
			return EVisibility::Hidden;
	}

private:

	TSharedPtr< SWindow > OwnerWindow;
	FString FullPath;
	class URoadRunnerImporterOptions* ImportOptions;

	bool ShouldImport_;
	bool VersionOutOfDate;
};
