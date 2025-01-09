﻿// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "Details/Slate/SMGANewAttributeVariableWidget.h"

#include "AttributeSet.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Editor.h"
#include "MGAEditorLog.h"
#include "Details/Slate/MGANewAttributeViewModel.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_VariableGet.h"
#include "PinTypeSelectorFilter.h"
#include "ScopedTransaction.h"
#include "SlateOptMacros.h"
#include "Attributes/ModularAttributeSetBase.h"
#include "Details/Slate/SMGAVariableRowWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/EngineVersionComparison.h"
#include "Styling/MGAAppStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "RigVMDeveloperTypeUtils.h"

#define LOCTEXT_NAMESPACE "SMGANewAttributeVariableWidget"

class FMGAAttributePinFilter : public IPinTypeSelectorFilter
{
public:
	explicit FMGAAttributePinFilter(const TWeakObjectPtr<UBlueprint> InBlueprint)
		: BlueprintPtr(InBlueprint)
	{
	}

	virtual bool ShouldShowPinTypeTreeItem(const FPinTypeTreeItem InItem) const override
	{
		if (!InItem.IsValid())
		{
			return false;
		}

		FString CPPType;
		UObject* CPPTypeObject = nullptr;
		RigVMTypeUtils::CPPTypeFromPinType(InItem.Get()->GetPinType(false), CPPType, &CPPTypeObject);

		if (const UScriptStruct* ScriptStruct = Cast<UScriptStruct>(CPPTypeObject))
		{
			if (ScriptStruct->IsChildOf(FGameplayAttributeData::StaticStruct()))
			{
				return true;
			}
		}

		return false;
	}

private:
	TWeakObjectPtr<UBlueprint> BlueprintPtr;
};

FVector2d SMGANewAttributeVariableWidget::DesiredSizeOverride = FVector2d(540.f, 280.f);

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SMGANewAttributeVariableWidget::Construct(const FArguments& InArgs, const TSharedPtr<FMGANewAttributeViewModel>& InViewModel)
{
	OnCancelDelegate = InArgs._OnCancel;
	OnFinishDelegate = InArgs._OnFinish;

	ViewModel = InViewModel;
	check(ViewModel.IsValid())

	const FString DocLink = TEXT("Shared/Editors/BlueprintEditor/VariableDetails");
	const FText VarNameTooltip = LOCTEXT("VarNameTooltip", "The name of the variable.");
	const FText DescriptionTooltip = LOCTEXT("VarToolTipDescription", "Extra information about this variable, shown when cursor is over it.");
	const FText VarTypeTooltip = LOCTEXT("VarTypeTooltip", "The type of the variable.");
	const FText ReplicationTooltip = LOCTEXT(
		"VariableReplicate_Tooltip",
		"Should this Variable be replicated over the network?\n\n"
		"You may want to turn this off if you're dealing with \"meta\" attributes."
	);
	const FSlateFontInfo FontInfo = FMGAAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"));

	// Get an appropriate name validator
	NameValidator = CreateNameValidator();

	// Pin type selector filter, to choose the appropriate Gameplay Attribute Data type
	const TSharedPtr<FMGAAttributePinFilter> CustomFilter = MakeShared<FMGAAttributePinFilter>(ViewModel->GetCustomizedBlueprint());

	TArray<TSharedPtr<IPinTypeSelectorFilter>> CustomPinTypeFilters;
	CustomPinTypeFilters.Add(CustomFilter);

	const UEdGraphSchema* Schema = GetDefault<UEdGraphSchema_K2>();

	constexpr float PaddingAmount = 8.0f;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(PaddingAmount))
		.BorderImage(FMGAAppStyle::GetBrush(TEXT("Docking.Tab.ContentAreaBrush")))
		// .BorderImage(FMGAAppStyle::GetBrush("DetailsView.CategoryTop"))
		[
			SNew(SBorder)
			.Padding(FMargin(18.f))
			.BorderImage(FMGAAppStyle::GetBrush("DetailsView.CategoryTop"))
			.BorderBackgroundColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SMGAVariableRowWidget)
					.NameContent()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("VariableNameLabel", "Variable Name"))
						.ToolTipText(VarNameTooltip)
						.Font(FontInfo)
					]
					.ValueContent()
					[
						SAssignNew(VarNameEditableTextBox, SEditableTextBox)
						.Text(this, &SMGANewAttributeVariableWidget::OnGetVarName)
						.ToolTipText(VarNameTooltip)
						.OnTextChanged(this, &SMGANewAttributeVariableWidget::OnVarNameChanged)
						.OnTextCommitted(this, &SMGANewAttributeVariableWidget::OnVarNameCommitted)
						.OnVerifyTextChanged(this, &SMGANewAttributeVariableWidget::HandleVerifyVariableNameChanged)
						.Font(FontInfo)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SMGAVariableRowWidget)
					.ValuePadding(FMargin(6.f, 3.f, 0.f, 3.f))
					.NameContent()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("VariableTypeLabel", "Variable Type"))
						.ToolTipText(VarTypeTooltip)
						.Font(FontInfo)
					]
					.ValueContent()
					[
						SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(GetDefault<UEdGraphSchema_K2>(), &UEdGraphSchema_K2::GetVariableTypeTree))
						.TargetPinType(this, &SMGANewAttributeVariableWidget::OnGetVarType)
						.OnPinTypeChanged(this, &SMGANewAttributeVariableWidget::OnVarTypeChanged)
						.Schema(Schema)
						.TypeTreeFilter(ETypeTreeFilter::None)
						.Font(FontInfo)
						.SelectorType(SPinTypeSelector::ESelectorType::Partial)
						.ToolTipText(VarTypeTooltip)
#if UE_VERSION_NEWER_THAN(5, 1, -1)
						.CustomFilters(CustomPinTypeFilters)
#else
						.CustomFilter(CustomFilter)
#endif
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SMGAVariableRowWidget)
					.EditableTextHeight(60.f)
					.VAlign(VAlign_Top)
					.FillWidth(5.f)
					.NameContent()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DescriptionLabel", "Description"))
						.ToolTipText(DescriptionTooltip)
						.Font(FontInfo)
					]
					.ValueContent()
					[
						SNew(SMultiLineEditableTextBox)
						.Text(this, &SMGANewAttributeVariableWidget::HandleGetDescriptionText)
						.ToolTipText(this, &SMGANewAttributeVariableWidget::HandleGetDescriptionText)
						.OnTextChanged(this, &SMGANewAttributeVariableWidget::HandleDescriptionTextChanged)
						.OnTextCommitted(this, &SMGANewAttributeVariableWidget::HandleDescriptionTextCommitted)
						.Padding(FMargin(8.0f, 8.0f))
						.Font(FontInfo)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SMGAVariableRowWidget)
					.NameContent()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("VariableReplicationLabel", "Replication"))
						.ToolTipText(ReplicationTooltip)
						.Font(FontInfo)
					]
					.ValueContent()
					[
						SNew(SCheckBox)
						.IsChecked(this, &SMGANewAttributeVariableWidget::HandleGetReplicationCheckboxState)
						.OnCheckStateChanged(this, &SMGANewAttributeVariableWidget::HandleReplicationCheckboxStateChanged)
						.ToolTipText(ReplicationTooltip)
					]
				]

				+SVerticalBox::Slot()
				  .FillHeight(1.f)
				  .HAlign(HAlign_Right)
				  .VAlign(VAlign_Bottom)
				  .Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FCoreStyle::Get().GetMargin("StandardDialog.SlotPadding"))
					.MinDesiredSlotWidth(FCoreStyle::Get().GetFloat("StandardDialog.MinDesiredSlotWidth"))
					.MinDesiredSlotHeight(FCoreStyle::Get().GetFloat("StandardDialog.MinDesiredSlotHeight"))

					+SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
						.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("DialogButtonText"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ContentPadding(FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding"))
						.OnClicked(this, &SMGANewAttributeVariableWidget::HandleFinishButtonClicked)
						.ToolTipText(LOCTEXT("AddButtonTooltip", "Add Attribute"))
						.Text(LOCTEXT("AddButtonLabel", "Add Attribute"))
						.IsEnabled(this, &SMGANewAttributeVariableWidget::CanFinish)
					]
					+SUniformGridPanel::Slot(1, 0)
					[
						// cancel button
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
						.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("DialogButtonText"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ContentPadding(FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding"))
						.OnClicked(this, &SMGANewAttributeVariableWidget::HandleCancelButtonClicked)
						.ToolTipText(LOCTEXT("CancelButtonTooltip", "Cancel"))
						.Text(LOCTEXT("CancelButtonLabel", "Cancel"))
					]
				]
			]
		]
	];

	FText ErrorText;
	HandleVerifyVariableNameChanged(FText::FromString(ViewModel->GetVariableName()), ErrorText);
	if (!ErrorText.IsEmpty())
	{
		VarNameEditableTextBox->SetError(ErrorText);
	}

	if (GEditor)
	{
		TWeakPtr<SMGANewAttributeVariableWidget> LocalWeakThis = SharedThis(this);
		GEditor->GetTimerManager()->SetTimerForNextTick([LocalWeakThis]
		{
			if (const TSharedPtr<SMGANewAttributeVariableWidget> StrongThis = LocalWeakThis.Pin(); StrongThis.IsValid())
			{
				// Set focus to the variable name box on creation
				FSlateApplication::Get().SetKeyboardFocus(StrongThis->VarNameEditableTextBox);
				FSlateApplication::Get().SetUserFocus(0, StrongThis->VarNameEditableTextBox);
				StrongThis->VarNameEditableTextBox->SelectAllText();
			}
		});
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

bool SMGANewAttributeVariableWidget::AddMemberVariable(UBlueprint* InBlueprint, const FString& InVarName, const FEdGraphPinType& InPinType, const FString& InDescription, const bool bInIsReplicated)
{
	check(InBlueprint);

	const FScopedTransaction Transaction(LOCTEXT("AddAttribute", "Add Attribute"));

	const FName VarName = FindUniqueKismetName(InBlueprint, InVarName);
	MGA_EDITOR_LOG(Verbose, TEXT("SMGANewAttributeVariableWidget::AddMemberVariable - InVarName: %s, VarName: %s"), *InVarName, *VarName.ToString())

	const bool bSuccess = FBlueprintEditorUtils::AddMemberVariable(InBlueprint, VarName, InPinType);
	if (!bSuccess)
	{
		MGA_EDITOR_LOG(Error, TEXT("Failed to add new variable to blueprint (%s)"), *InBlueprint->GetName());
		return false;
	}

	// Handle desc
	FBlueprintEditorUtils::SetBlueprintVariableMetaData(InBlueprint, VarName, nullptr, TEXT("tooltip"), InDescription);

	// Handle rep condition
	if (bInIsReplicated)
	{
		uint64* PropFlagPtr = FBlueprintEditorUtils::GetBlueprintVariablePropertyFlags(InBlueprint, VarName);
		if (PropFlagPtr != nullptr)
		{
			*PropFlagPtr |= CPF_Net;

			const FString NewFuncName = FString::Printf(TEXT("OnRep_%s"), *VarName.ToString());
			UEdGraph* FuncGraph = FindObject<UEdGraph>(InBlueprint, *NewFuncName);
			if (!FuncGraph)
			{
				FuncGraph = FBlueprintEditorUtils::CreateNewGraph(InBlueprint, FName(*NewFuncName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
				FBlueprintEditorUtils::AddFunctionGraph<UClass>(InBlueprint, FuncGraph, false, nullptr);
				check(FuncGraph);

				// Get the K2 node entry point
				TArray<UK2Node_FunctionEntry*> Entries;
				FuncGraph->GetNodesOfClass(Entries);
				const UK2Node_FunctionEntry* FunctionEntry = Entries.IsValidIndex(0) ? Entries[0] : nullptr;

				// Create the rep function graph
				UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(FuncGraph, UK2Node_CallFunction::StaticClass(), NAME_None, RF_Transactional);

				// Figure out the property type, so that we know which member reference to build
				FName MemberName = TEXT("HandleRepNotifyForGameplayAttributeData");
				if (InPinType.PinSubCategoryObject.IsValid() && InPinType.PinSubCategoryObject->GetName() == TEXT("MGAGameplayClampedAttributeData"))
				{
					MemberName = TEXT("HandleRepNotifyForGameplayClampedAttributeData");
				}
				
				FMemberReference MemberReference;
				MemberReference.SetExternalMember(MemberName, UModularAttributeSetBase::StaticClass());

				Node->FunctionReference = MemberReference;
				Node->CreateNewGuid();
				Node->AllocateDefaultPins();

				const FVector2D NodeLocation = FunctionEntry ?  FVector2D(FunctionEntry->NodePosX + 256, FunctionEntry->NodePosY - 16) : FuncGraph->GetGoodPlaceForNewNode();
				Node->NodePosX = NodeLocation.X;
				Node->NodePosY = NodeLocation.Y;
				Node->PostPlacedNewNode();

				if (UEdGraphPin* Pin = Node->FindPin(TEXT("InAttribute")))
				{
					const FVector2D GraphPosition = FuncGraph->GetGoodPlaceForNewNode();
					UClass* VariableSource = FBlueprintEditorUtils::GetMostUpToDateClass(InBlueprint->GeneratedClass);

					FEdGraphSchemaAction_K2NewNode Action;
					UK2Node_Variable* VarNode = NewObject<UK2Node_VariableGet>();
					Action.NodeTemplate = VarNode;

					UEdGraphSchema_K2::ConfigureVarNode(VarNode, VarName, VariableSource, InBlueprint);
					Action.PerformAction(FuncGraph, Pin, GraphPosition);
				}

				FuncGraph->Modify();
				FuncGraph->AddNode(Node, true, false);

				// Wire up the connection between entry node and new function node
				if (FunctionEntry)
				{
					Node->GetExecPin()->MakeLinkTo(FunctionEntry->FindPin(UEdGraphSchema_K2::PN_Then));
				}
			}

			if (FuncGraph)
			{
				ReplicationOnRepFuncChanged(InBlueprint, NewFuncName, VarName);
			}
		}
	}

	// And recompile
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(InBlueprint);

	return true;
}

void SMGANewAttributeVariableWidget::ReplicationOnRepFuncChanged(UBlueprint* InBlueprint, const FString& InNewOnRepFunc, const FName& InVarName)
{
	check(InBlueprint);
	
	const FName NewFuncName = FName(*InNewOnRepFunc);
	FBlueprintEditorUtils::SetBlueprintVariableRepNotifyFunc(InBlueprint, InVarName, NewFuncName);
	uint64* PropFlagPtr = FBlueprintEditorUtils::GetBlueprintVariablePropertyFlags(InBlueprint, InVarName);
	if (PropFlagPtr != nullptr)
	{
		if (NewFuncName != NAME_None)
		{
			*PropFlagPtr |= CPF_RepNotify;
			*PropFlagPtr |= CPF_Net;
		}
		else
		{
			*PropFlagPtr &= ~CPF_RepNotify;
		}
	}
}

TSharedPtr<INameValidatorInterface> SMGANewAttributeVariableWidget::CreateNameValidator() const
{
	check(ViewModel.IsValid());

	const UEdGraphSchema* Schema = nullptr;
	const UBlueprint* BlueprintPtr = ViewModel->GetCustomizedBlueprint().Get();
	if (BlueprintPtr)
	{
		TArray<UEdGraph*> Graphs;
		BlueprintPtr->GetAllGraphs(Graphs);
		if (Graphs.Num() > 0)
		{
			Schema = Graphs[0]->GetSchema();
		}
	}

	if (Schema)
	{
		return Schema->GetNameValidator(BlueprintPtr, NAME_None, nullptr, FEdGraphSchemaAction_K2Var::StaticGetTypeId());
	}

	return nullptr;
}

FText SMGANewAttributeVariableWidget::OnGetVarName() const
{
	check(ViewModel.IsValid());
	return FText::FromString(ViewModel->GetVariableName());
}

void SMGANewAttributeVariableWidget::OnVarNameChanged(const FText& InText) const
{
	check(ViewModel.IsValid());
	ViewModel->SetVariableName(InText.ToString());
}

void SMGANewAttributeVariableWidget::OnVarNameCommitted(const FText& InText, const ETextCommit::Type InTextCommit)
{
	check(ViewModel.IsValid());
	ViewModel->SetVariableName(InText.ToString());

	MGA_EDITOR_LOG(Verbose, TEXT("SMGANewAttributeVariableWidget::OnVarNameCommitted - InText: %s, InTextCommit: %s"), *InText.ToString(), *UEnum::GetValueAsString(InTextCommit))
	
	if (InTextCommit == ETextCommit::OnEnter && CanFinish())
	{
		HandleFinishButtonClicked();
	}
}

bool SMGANewAttributeVariableWidget::HandleVerifyVariableNameChanged(const FText& InText, FText& OutErrorText) const
{
	if (NameValidator.IsValid())
	{
		const EValidatorResult ValidatorResult = NameValidator->IsValid(InText.ToString());
		switch (ValidatorResult)
		{
		case EValidatorResult::Ok:
			// case EValidatorResult::ExistingName:
			// These are fine, don't need to surface to the user
			return true;
		default:
			OutErrorText = INameValidatorInterface::GetErrorText(InText.ToString(), ValidatorResult);
			return false;
		}
	}

	return true;
}

FEdGraphPinType SMGANewAttributeVariableWidget::OnGetVarType() const
{
	check(ViewModel.IsValid());
	return ViewModel->GetPinType();
}

void SMGANewAttributeVariableWidget::OnVarTypeChanged(const FEdGraphPinType& InEdGraphPin) const
{
	check(ViewModel.IsValid());
	ViewModel->SetPinType(InEdGraphPin);
}

FText SMGANewAttributeVariableWidget::HandleGetDescriptionText() const
{
	check(ViewModel.IsValid());
	return FText::FromString(ViewModel->GetDescription());
}

// ReSharper disable once CppParameterNeverUsed
void SMGANewAttributeVariableWidget::HandleDescriptionTextCommitted(const FText& InText, ETextCommit::Type InArg) const
{
	HandleDescriptionTextChanged(InText);
}

void SMGANewAttributeVariableWidget::HandleDescriptionTextChanged(const FText& InText) const
{
	check(ViewModel.IsValid());
	ViewModel->SetDescription(InText.ToString());
}

ECheckBoxState SMGANewAttributeVariableWidget::HandleGetReplicationCheckboxState() const
{
	check(ViewModel.IsValid());
	return ViewModel->GetbIsReplicated() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SMGANewAttributeVariableWidget::HandleReplicationCheckboxStateChanged(const ECheckBoxState InCheckBoxState) const
{
	check(ViewModel.IsValid());
	ViewModel->SetbIsReplicated(InCheckBoxState == ECheckBoxState::Checked);
}

FVector2D SMGANewAttributeVariableWidget::ComputeDesiredSize(float InLayoutScaleMultiplier) const
{
	return DesiredSizeOverride;
}

FReply SMGANewAttributeVariableWidget::HandleCancelButtonClicked()
{
	CloseWindow();
	OnCancelDelegate.ExecuteIfBound(ViewModel);
	return FReply::Handled();
}

FReply SMGANewAttributeVariableWidget::HandleFinishButtonClicked()
{
	CloseWindow();
	OnFinishDelegate.ExecuteIfBound(ViewModel);
	return FReply::Handled();
}

bool SMGANewAttributeVariableWidget::CanFinish() const
{
	if (VarNameEditableTextBox.IsValid())
	{
		return !VarNameEditableTextBox->HasError();
	}
	
	return false;
}

void SMGANewAttributeVariableWidget::CloseWindow()
{
	const TSharedPtr<SWindow> ContainingWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	if (ContainingWindow.IsValid())
	{
		ContainingWindow->RequestDestroyWindow();
	}
}

FName SMGANewAttributeVariableWidget::FindUniqueKismetName(const UBlueprint* InBlueprint, const FString& InBaseName, UStruct* InScope)
{
	const TSharedPtr<FKismetNameValidator> KismetNameValidator = MakeShared<FKismetNameValidator>(InBlueprint, NAME_None, InScope);
	check(KismetNameValidator.IsValid());

	// Ask validator if it is an existing variable name (or invalid), only go through FBlueprintEditorUtils::FindUniqueKismetName if that is the case
	// (5.0 always consider input name invalid and always suffix compared to 5.1, cause it starts with KismetName empty)
	
	const EValidatorResult ValidatorResult = KismetNameValidator->IsValid(InBaseName);
	if (ValidatorResult == EValidatorResult::Ok)
	{
		return FName(*InBaseName);
	}

	return FBlueprintEditorUtils::FindUniqueKismetName(InBlueprint, InBaseName, InScope);
}

#undef LOCTEXT_NAMESPACE
