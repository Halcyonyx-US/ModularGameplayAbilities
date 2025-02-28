﻿// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "UObject/WeakObjectPtr.h"

class FMGABlueprintEditor;
class FMGANewAttributeViewModel;
class SMGAPositiveActionButton;
class SWidget;
class UAttributeSet;
class UBlueprint;

/**
 * Details customization for Attribute Sets
 *
 * Mainly to add a "+" button to add a new attribute member variable.
 */
class FMGAAttributeSetDetails : public IDetailCustomization
{
public:
	FMGAAttributeSetDetails() = default;

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& InDetailLayout) override;

private:
	/** The UObject we are editing */
	TWeakObjectPtr<UAttributeSet> AttributeBeingCustomized;

	/** The blueprint we are editing */
	TWeakObjectPtr<UBlueprint> BlueprintBeingCustomized;
	
	/** Weak reference to the Blueprint editor */
	TWeakPtr<FMGABlueprintEditor> BlueprintEditorPtr;

	/** Constructs a DetailsView widget for the new attribute window */
	TSharedRef<SWidget> CreateNewAttributeVariableWidget();

	static void HandleAttributeWindowCancel(const TSharedPtr<FMGANewAttributeViewModel>& InViewModel);
	void HandleAttributeWindowFinish(const TSharedPtr<FMGANewAttributeViewModel>& InViewModel) const;
};
