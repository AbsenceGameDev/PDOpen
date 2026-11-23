/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "SaveEditor/RTSOSaveEditorWidget.h"

#include "Actors/RTSOController.h"
#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "SaveEditor/SRTSOSaveEditor.h"
#include "SaveEditor/SRTSOSaveEditor_ConversationsData.h"
#include "SaveEditor/SRTSOSaveEditor_EntityData.h"
#include "SaveEditor/SRTSOSaveEditor_InteractableData.h"
#include "SaveEditor/SRTSOSaveEditor_MissionTagsData.h"
#include "SaveEditor/SRTSOSaveEditor_PlayerBaseData.h"
#include "SaveEditor/SRTSOSaveEditor_PlayerInventoryData.h"
#include "SaveEditor/SRTSOSaveEditor_WorldBaseData.h"

#define LOADSLOT(SlotIdx) Cast<URTSOpenSaveGame>(UGameplayStatics::LoadGameFromSlot(FString::FromInt(SlotIdx), SlotIdx))

#define LOCTEXT_NAMESPACE "SRTSOSaveEditor"

FText URTSOSaveEditorUserWidget::LoadedSlot_TitleText = LOCTEXT("TitleText_PlayerInventories", "LOADED SLOT");


void URTSOSaveEditorInnerWidget::CopyData(URTSOpenSaveGame* InSaveGame)
{
	AsyncTask(ENamedThreads::GameThread,
		[this, InSaveGame]()
		{
			if (InSaveGame == nullptr)
			{
				OnFailedCopyData();
				return;
			}
			
			SaveGamePtr = InSaveGame;
			CopiedSaveData = SaveGamePtr->Data;
			OnCompletedCopyData();
		});
}

void URTSOSaveEditorInnerWidget::OnFailedCopyData()
{
}

void URTSOSaveEditorInnerWidget::OnCompletedCopyData()
{
	UE_LOG(PDLog_SaveEditor, Warning, TEXT("URTSOSaveEditorInnerWidget::OnCompletedCopyData"))

	// OnCompletedCopyData_Debug();
	
	// @todo  Rethink design here, want to avoid having to iterate the maps here,
	// ^@todo some of them are large enough it will be a performance penalty if I keep it like this
	LocationsAsSharedTupleArray.Empty();
	for (const TTuple<int, UE::Math::TVector<double>>& PlayerLocationTuple : CopiedSaveData.PlayerLocations)
	{
		FPlayerLocationStruct PlayerDatum{PlayerLocationTuple.Key, PlayerLocationTuple.Value};
		LocationsAsSharedTupleArray.Emplace(MakeShared<FPlayerLocationStruct>(PlayerDatum).ToSharedPtr());
	}

	InteractableAsSharedArray.Empty();
	for (FRTSSavedInteractable& Interactable : CopiedSaveData.Interactables)
	{
		Interactable.CopySelectedToSoftClass();
		InteractableAsSharedArray.Emplace(MakeShared<FRTSSavedInteractable>(Interactable).ToSharedPtr());
	}

	EntitiesAsSharedArray.Empty();
	for (const FRTSSavedWorldUnits& EntityUnit : CopiedSaveData.EntityUnits)
	{
		EntitiesAsSharedArray.Emplace(MakeShared<FRTSSavedWorldUnits>(EntityUnit).ToSharedPtr());
	}

	AllUserInventoriesAsSharedTupleArray.Empty();
	for (const TTuple<int32, FRTSSavedItems>& UserInventoryTuple : CopiedSaveData.Inventories)
	{
		FUserInventoriesStruct InvDatum{UserInventoryTuple.Key, UserInventoryTuple.Value};
		AllUserInventoriesAsSharedTupleArray.Emplace(MakeShared<FUserInventoriesStruct>(InvDatum).ToSharedPtr());
	}

	ConversationStatesAsSharedArray.Empty();
	for (TTuple<int32, FRTSSavedConversationActorData>& ConversationStateTuple : CopiedSaveData.ConversationActorState)
	{
		ConversationStateTuple.Value.CopySelectedToSoftClass();
		FConversationStateStruct ConversationStateDatum{ConversationStateTuple.Key, ConversationStateTuple.Value};
		ConversationStatesAsSharedArray.Emplace(MakeShared<FConversationStateStruct>(ConversationStateDatum).ToSharedPtr());
	}

	SelectEditor(EditorType); // ensure we update data
}

void URTSOSaveEditorInnerWidget::OnCompletedCopyData_Debug()
{
	// @todo  Rethink design here, want to avoid having to iterate the maps here,
	// ^@todo some of them are large enough it will be a performance penalty if I keep it like this
	for (const TTuple<int, UE::Math::TVector<double>>& PlayerLocationTuple : CopiedSaveData.PlayerLocations)
	{
		UE_LOG(PDLog_SaveEditor, Warning, TEXT("URTSOSaveEditorInnerWidget::OnCompletedCopyData -- Player Base Data Iter: ID(%i) "), PlayerLocationTuple.Key)
	}

	for (const FRTSSavedInteractable& Interactable : CopiedSaveData.Interactables)
	{
		UE_LOG(PDLog_SaveEditor, Warning, TEXT("URTSOSaveEditorInnerWidget::OnCompletedCopyData -- Interactable Data Iter: ID(%i) "), Interactable.InstanceIndex)
	}

	for (const FRTSSavedWorldUnits& EntityUnit : CopiedSaveData.EntityUnits)
	{
		UE_LOG(PDLog_SaveEditor, Warning, TEXT("URTSOSaveEditorInnerWidget::OnCompletedCopyData -- Entity Data Iter: ID(%i) "), EntityUnit.InstanceIndex.Index)
	}

	for (const TTuple<int32, FRTSSavedItems>& UserInventoryTuple : CopiedSaveData.Inventories)
	{
		UE_LOG(PDLog_SaveEditor, Warning, TEXT("URTSOSaveEditorInnerWidget::OnCompletedCopyData -- Inv. Data Iter: ID(%i) "), UserInventoryTuple.Key)
	}

	for (const TTuple<int32, FRTSSavedConversationActorData>& ConversationStateTuple : CopiedSaveData.ConversationActorState)
	{
		UE_LOG(PDLog_SaveEditor, Warning, TEXT("URTSOSaveEditorInnerWidget::OnCompletedCopyData -- Conv. Data Iter: ID(%i) "), ConversationStateTuple.Key)
	}
}

void URTSOSaveEditorInnerWidget::UpdateInnerEditor()
{
	//
	// Check if we already have a child with our slate save editor class
	FChildren* WrappedChildren = InnerSlateWrapbox->GetChildren();
	const int32 Limit = WrappedChildren->Num();
	for (int32 Step = 0; Step < Limit; )
	{
		TSharedRef<SWidget> CurrentChild = WrappedChildren->GetChildAt(Step);
		SRTSOSaveEditorBase* PotentialSaveEditor = nullptr;

		switch (EditorType)
		{
		case EPDSaveDataThreadSelector::EPlayers:
			PotentialSaveEditor	= static_cast<SRTSOSaveEditor_PlayerBaseData*>(CurrentChild.ToSharedPtr().Get());
			static_cast<SRTSOSaveEditor_PlayerBaseData*>(CurrentChild.ToSharedPtr().Get())->LocationsAsSharedTupleArray = &LocationsAsSharedTupleArray;
			break;
		case EPDSaveDataThreadSelector::EInteractables:
			PotentialSaveEditor	= static_cast<SRTSOSaveEditor_InteractableData*>(CurrentChild.ToSharedPtr().Get());
			static_cast<SRTSOSaveEditor_InteractableData*>(CurrentChild.ToSharedPtr().Get())->InteractableAsSharedArray = &InteractableAsSharedArray;
			break;
		case EPDSaveDataThreadSelector::EEntities:
			PotentialSaveEditor	= static_cast<SRTSOSaveEditor_EntityData*>(CurrentChild.ToSharedPtr().Get());
			static_cast<SRTSOSaveEditor_EntityData*>(CurrentChild.ToSharedPtr().Get())->EntitiesAsSharedArray = &EntitiesAsSharedArray;
			break;
		case EPDSaveDataThreadSelector::EInventories:
			PotentialSaveEditor	= static_cast<SRTSOSaveEditor_PlayerInventoryData*>(CurrentChild.ToSharedPtr().Get());
			static_cast<SRTSOSaveEditor_PlayerInventoryData*>(CurrentChild.ToSharedPtr().Get())->AllUserInventoriesAsSharedTupleArray = &AllUserInventoriesAsSharedTupleArray;
			break;
		case EPDSaveDataThreadSelector::EConversationActors:
			PotentialSaveEditor	= static_cast<SRTSOSaveEditor_ConversationsData*>(CurrentChild.ToSharedPtr().Get());
			static_cast<SRTSOSaveEditor_ConversationsData*>(CurrentChild.ToSharedPtr().Get())->ConversationStatesAsSharedArray = &ConversationStatesAsSharedArray;
			break;
		case EPDSaveDataThreadSelector::EPlayerConversationProgress:
			PotentialSaveEditor	= static_cast<SRTSOSaveEditor_MissionTagsData*>(CurrentChild.ToSharedPtr().Get());
			static_cast<SRTSOSaveEditor_MissionTagsData*>(CurrentChild.ToSharedPtr().Get())->UserMissionTagsAsSharedArray = &UserMissionTagsAsSharedArray;
			break;
		case EPDSaveDataThreadSelector::EWorldBaseData:
			PotentialSaveEditor	= static_cast<SRTSOSaveEditor_WorldBaseData*>(CurrentChild.ToSharedPtr().Get());
			break;
		case EPDSaveDataThreadSelector::EEnd:
			break;
		}

		if (PotentialSaveEditor != nullptr)
		{
			SharedExistingSaveEditor = MakeShareable(PotentialSaveEditor);
			InnerSlateWrapbox->Slot().AttachWidget(SharedExistingSaveEditor.ToSharedRef());
			break;
		}
		
		Step++;
	}

	if (SharedExistingSaveEditor == nullptr)
	{
		switch (EditorType)
		{
		case EPDSaveDataThreadSelector::EPlayers:
			{
				const TSharedRef<SRTSOSaveEditor_PlayerBaseData> PlayerBaseDataRef = SNew(SRTSOSaveEditor_PlayerBaseData, &CopiedSaveData, LocationsAsSharedTupleArray);
				SharedExistingSaveEditor	= PlayerBaseDataRef.ToSharedPtr();
				break;
			}
		case EPDSaveDataThreadSelector::EInteractables:
			{
				const TSharedRef<SRTSOSaveEditor_InteractableData> DataRef = SNew(SRTSOSaveEditor_InteractableData, &CopiedSaveData, InteractableAsSharedArray);
				SharedExistingSaveEditor	= DataRef.ToSharedPtr();
				break;
			}
		case EPDSaveDataThreadSelector::EEntities:
			{
				const TSharedRef<SRTSOSaveEditor_EntityData> DataRef = SNew(SRTSOSaveEditor_EntityData, &CopiedSaveData, EntitiesAsSharedArray);
				SharedExistingSaveEditor	= DataRef.ToSharedPtr();
				break;
			}
		case EPDSaveDataThreadSelector::EInventories:
			{
				const TSharedRef<SRTSOSaveEditor_PlayerInventoryData> DataRef = SNew(SRTSOSaveEditor_PlayerInventoryData, &CopiedSaveData, AllUserInventoriesAsSharedTupleArray);
				SharedExistingSaveEditor	= DataRef.ToSharedPtr();
				break;
			}
		case EPDSaveDataThreadSelector::EConversationActors:
			{
				const TSharedRef<SRTSOSaveEditor_ConversationsData> DataRef = SNew(SRTSOSaveEditor_ConversationsData, &CopiedSaveData, ConversationStatesAsSharedArray);
				SharedExistingSaveEditor	= DataRef.ToSharedPtr();
				break;
			}
		case EPDSaveDataThreadSelector::EPlayerConversationProgress:
			{
				const TSharedRef<SRTSOSaveEditor_MissionTagsData> DataRef = SNew(SRTSOSaveEditor_MissionTagsData, &CopiedSaveData, UserMissionTagsAsSharedArray);
				SharedExistingSaveEditor	= DataRef.ToSharedPtr();
				break;
			}
		case EPDSaveDataThreadSelector::EWorldBaseData:
			{
				const TSharedRef<SRTSOSaveEditor_WorldBaseData> DataRef = SNew(SRTSOSaveEditor_WorldBaseData, &CopiedSaveData);
				SharedExistingSaveEditor	= DataRef.ToSharedPtr();
				break;
			}
		case EPDSaveDataThreadSelector::EEnd:
			break;
		}
		
		TPanelChildren<SWrapBox::FSlot>::FScopedWidgetSlotArguments ScopedArgs = InnerSlateWrapbox->AddSlot();
		ScopedArgs.AttachWidget(SharedExistingSaveEditor.ToSharedRef());
		// ScopedArgs.HAlign(HAlign_Fill);
		// ScopedArgs.VAlign(VAlign_Top);
		// ScopedArgs.FillEmptySpace(true);
	}
}

TSharedRef<SWidget> URTSOSaveEditorInnerWidget::RebuildWidget()
{
	if (InnerSlateWrapbox.IsValid() == false)
	{
		InnerSlateWrapbox = SNew(SWrapBox);
	}
	
	UpdateInnerEditor();
	if (SharedExistingSaveEditor != nullptr)
	{
		SharedExistingSaveEditor->UpdateChildSlot(nullptr); 
	}
	
	return InnerSlateWrapbox.ToSharedRef();
}

void URTSOSaveEditorInnerWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	InnerSlateWrapbox.Reset();
	SharedExistingSaveEditor.Reset();
	
	Super::ReleaseSlateResources(bReleaseChildren);
}

void URTSOSaveEditorInnerWidget::SelectEditor(EPDSaveDataThreadSelector NewEditorType)
{
	EditorType = NewEditorType;

	InnerSlateWrapbox->ClearChildren();
	if (SharedExistingSaveEditor.IsValid())
	{
		SharedExistingSaveEditor.Reset();
	}
	
	UpdateInnerEditor();
}

void URTSOSaveEditorInnerWidget::ResetFieldData(EPDSaveDataThreadSelector SaveDataGroupSelector)
{
	if (SaveGamePtr == nullptr)
	{
		UE_LOG(PDLog_SaveEditor, Error, TEXT("SRTSOSaveEditor::ResetFieldData -- 'SaveGamePtr' has gone stale"))
		return;
	}
	
	switch (SaveDataGroupSelector)
	{
	case EPDSaveDataThreadSelector::EWorldBaseData:
		CopiedSaveData.Seeder = SaveGamePtr->Data.Seeder;
		CopiedSaveData.GameTime = SaveGamePtr->Data.GameTime;
		break;
	case EPDSaveDataThreadSelector::EPlayers:
		CopiedSaveData.PlayerLocations = SaveGamePtr->Data.PlayerLocations;
		break;
	case EPDSaveDataThreadSelector::EInteractables:
		CopiedSaveData.Interactables = SaveGamePtr->Data.Interactables;
		break;
	case EPDSaveDataThreadSelector::EEntities:
		CopiedSaveData.EntityUnits = SaveGamePtr->Data.EntityUnits;
		break;
	case EPDSaveDataThreadSelector::EInventories:
		CopiedSaveData.Inventories = SaveGamePtr->Data.Inventories;
		break;
	case EPDSaveDataThreadSelector::EConversationActors:
		CopiedSaveData.ConversationActorState = SaveGamePtr->Data.ConversationActorState;
		break;
	case EPDSaveDataThreadSelector::EPlayerConversationProgress:
		CopiedSaveData.PlayersAndConversationTags = SaveGamePtr->Data.PlayersAndConversationTags;
		break;
	case EPDSaveDataThreadSelector::EEnd:
		CopiedSaveData.Seeder = SaveGamePtr->Data.Seeder;
		CopiedSaveData.GameTime = SaveGamePtr->Data.GameTime;
		CopiedSaveData.PlayerLocations = SaveGamePtr->Data.PlayerLocations;
		CopiedSaveData.Interactables = SaveGamePtr->Data.Interactables;
		CopiedSaveData.EntityUnits = SaveGamePtr->Data.EntityUnits;
		CopiedSaveData.Inventories = SaveGamePtr->Data.Inventories;
		CopiedSaveData.ConversationActorState = SaveGamePtr->Data.ConversationActorState;
		CopiedSaveData.PlayersAndConversationTags = SaveGamePtr->Data.PlayersAndConversationTags;
		break;
	}
}


UButton* URTSOSaveEditorUserWidget::GetCategoryButton(EPDSaveDataThreadSelector Button) const
{
	UButton* SelectedButton = nullptr;
	switch (Button)
	{
	case EPDSaveDataThreadSelector::EWorldBaseData:
		SelectedButton = Btn_WorldBaseData->Hitbox;
		break;
	case EPDSaveDataThreadSelector::EPlayers:
		SelectedButton = Btn_PlayerBaseData->Hitbox;
		break;
	case EPDSaveDataThreadSelector::EInteractables:
		SelectedButton = Btn_InteractableData->Hitbox;
		break;
	case EPDSaveDataThreadSelector::EEntities:
		SelectedButton = Btn_EntityData->Hitbox;
		break;
	case EPDSaveDataThreadSelector::EInventories:
		SelectedButton = Btn_PlayerInventoriesData->Hitbox;
		break;
	case EPDSaveDataThreadSelector::EConversationActors:
		SelectedButton = Btn_ConversationStateData->Hitbox;
		break;
	case EPDSaveDataThreadSelector::EPlayerConversationProgress:
		SelectedButton = Btn_MissionProgressTagsData->Hitbox;
		break;
	case EPDSaveDataThreadSelector::EEnd:
		break;
	}

	return SelectedButton;
}

void URTSOSaveEditorUserWidget::Category_ResetButtonState(EPDSaveDataThreadSelector PreviousButton) const
{
	UButton* SelectedButton = GetCategoryButton(PreviousButton);
	if (SelectedButton == nullptr) { return; }
	
	SelectedButton->SetColorAndOpacity(FLinearColor::Gray);	
}

void URTSOSaveEditorUserWidget::Category_ActivateButtonState(EPDSaveDataThreadSelector NewButton) const
{
	UButton* SelectedButton = GetCategoryButton(NewButton);
	if (SelectedButton == nullptr) { return; }

	SelectedButton->SetColorAndOpacity(FColor::Silver);
}


void URTSOSaveEditorUserWidget::SelectCategory(EPDSaveDataThreadSelector CategoryButton) const
{
	if (Inner->EditorType == CategoryButton) { return; }
	Category_ResetButtonState(Inner->EditorType);
	
	Inner->SelectEditor(CategoryButton);
	Category_ActivateButtonState(CategoryButton);

	Inner->InvalidateLayoutAndVolatility();
}

void URTSOSaveEditorUserWidget::SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector CategoryButton) 
{
	SelectCategory(CategoryButton);
	InvalidateLayoutAndVolatility();
}

void URTSOSaveEditorUserWidget::Category_WorldBaseData()
{
	SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EWorldBaseData);
}

void URTSOSaveEditorUserWidget::Category_PlayerBaseData()
{
	SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EPlayers);
}

void URTSOSaveEditorUserWidget::Category_InteractableData()
{
	SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EInteractables);
}

void URTSOSaveEditorUserWidget::Category_EntityData()
{
	SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EEntities);
}

void URTSOSaveEditorUserWidget::Category_PlayerInventoriesData()
{
	SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EInventories);
}

void URTSOSaveEditorUserWidget::Category_ConversationStateData()
{
	SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EConversationActors);
}

void URTSOSaveEditorUserWidget::Category_MissionProgressTagsData()
{
	SelectCategoryAndInvalidateCache(EPDSaveDataThreadSelector::EPlayerConversationProgress);
}

void URTSOSaveEditorUserWidget::OnAnimationStarted_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationStarted_Implementation(Animation);

	if (Animation != CategoryLoadingAnimation) { return; }
	
	LoadViewWidgetSwitch->SetActiveWidgetIndex(1);
	TabsLoadViewWidgetSwitch->SetActiveWidgetIndex(1);
}

void URTSOSaveEditorUserWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation != CategoryLoadingAnimation) { return; }

	LoadViewWidgetSwitch->SetActiveWidgetIndex(0);
	TabsLoadViewWidgetSwitch->SetActiveWidgetIndex(0);
}


void URTSOSaveEditorUserWidget::BindButtonDelegates()
{
	ARTSOController* AsController = Cast<ARTSOController>(ActorToBindAt);
	if (AsController == nullptr) { return; }

	// @todo Spawn confirm dialog if we have unsaved changes, do it within ARTSOController::CloseSaveEditor!
	ExitButton->Hitbox->OnPressed.AddUniqueDynamic(AsController, &ARTSOController::CloseSaveEditor);

	Slot0->Hitbox->OnReleased.AddUniqueDynamic(this, &URTSOSaveEditorUserWidget::LoadSlotData0);
	Slot1->Hitbox->OnReleased.AddUniqueDynamic(this, &URTSOSaveEditorUserWidget::LoadSlotData1);
	Slot2->Hitbox->OnReleased.AddUniqueDynamic(this, &URTSOSaveEditorUserWidget::LoadSlotData2);
	Slot3->Hitbox->OnReleased.AddUniqueDynamic(this, &URTSOSaveEditorUserWidget::LoadSlotData3);
	Slot4->Hitbox->OnReleased.AddUniqueDynamic(this, &URTSOSaveEditorUserWidget::LoadSlotData4);

	Btn_WorldBaseData->Hitbox->OnPressed.AddUniqueDynamic(this, &URTSOSaveEditorUserWidget::Category_WorldBaseData);
	Btn_PlayerBaseData->Hitbox->OnPressed.AddUniqueDynamic(this, &URTSOSaveEditorUserWidget::Category_PlayerBaseData);
	Btn_InteractableData->Hitbox->OnPressed.AddUniqueDynamic(this, &URTSOSaveEditorUserWidget::Category_InteractableData);
	Btn_EntityData->Hitbox->OnPressed.AddUniqueDynamic(this, &URTSOSaveEditorUserWidget::Category_EntityData);
	Btn_PlayerInventoriesData->Hitbox->OnPressed.AddUniqueDynamic(this, &URTSOSaveEditorUserWidget::Category_PlayerInventoriesData);
	Btn_ConversationStateData->Hitbox->OnPressed.AddUniqueDynamic(this, &URTSOSaveEditorUserWidget::Category_ConversationStateData);
	Btn_MissionProgressTagsData->Hitbox->OnPressed.AddUniqueDynamic(this, &URTSOSaveEditorUserWidget::Category_MissionProgressTagsData);	
}

void URTSOSaveEditorUserWidget::LoadSlotData(int32 SlotIdx, bool bFirstLoad)
{
	// Loading the slot might take a while, so we play a widget animation to cover it while waiting 
	PlayAnimation(CategoryLoadingAnimation);

	const FText SlotIdxText = FText::FromString(LoadedSlot_TitleText.ToString() + ": #" + FString::FromInt(SlotIdx));
	DataViewTitle->SetText(SlotIdxText);
	AsyncTask(ENamedThreads::GameThread,
		[&, Slot = SlotIdx, bFirstLoad]()
		{
			if (bFirstLoad)
			{
				BindButtonDelegates();
			}
			
			LoadedGameSaveForModification = LOADSLOT(Slot);
			Inner->CopyData(LoadedGameSaveForModification);
			StopAnimation(CategoryLoadingAnimation);
		} );
}

void URTSOSaveEditorUserWidget::LoadSlotData0() { LoadSlotData(0); }
void URTSOSaveEditorUserWidget::LoadSlotData1() { LoadSlotData(1); }
void URTSOSaveEditorUserWidget::LoadSlotData2() { LoadSlotData(2); }
void URTSOSaveEditorUserWidget::LoadSlotData3() { LoadSlotData(3); }
void URTSOSaveEditorUserWidget::LoadSlotData4() { LoadSlotData(4); }


#undef LOCTEXT_NAMESPACE

/*
Business Source License 1.1

Parameters

Licensor:             Ario Amin (@ Permafrost Development)
Licensed Work:        RTSOpen (version 0.1.0, Source available on github)
                      The Licensed Work is (c) 2026 Ario Amin (@ Permafrost Development)
Additional Use Grant: You may make free use of the Licensed Work in a commercial product or service provided these additional conditions as met; 
                      1. Must give attributions to the original author of the Licensed Work, in 'Credits' if that is applicable.
                      2. The Licensed Work must be 'Compiled' before being redistributed.
                      3. The Licensed Work 'Source' may be linked but may not be packaged into the product or service being sold
                      4. Must not be resold or repackaged or redistributed as another product, is only allowed to be used within a commercial or non-commercial game project.
                      5. Teams whose 'Total Finances' exceed $100,000 USD for the most recent 12-month period must contact the owner for a custom license or buy the framework from a marketplace it has been made available on.

                      "Credits" indicate a scrolling screen with attributions. This is usually in a products end-state

                      "Total Finances" means the largest of your aggregate gross revenues, entire budget, or funding (no matter the source).
                      "Package" means the collection of files distributed by the Licensor, and derivatives of that collection
                      and/or of those files..   

                      "Source" form means the source code, documentation source, and configuration files for the Package, usually in human-readable format.

                      "Compiled" form means the compiled bytecode, object code, binary, or any other
                      form resulting from mechanical transformation or translation of the Source form.

Change Date:          2030-04-26

Change License:       Apache License, Version 2.0

For information about alternative licensing arrangements for the Software,
please visit: https://permadev.se/

Notice

The Business Source License (this document, or the “License”) is not an Open
Source license. However, the Licensed Work will eventually be made available
under an Open Source License, as stated in this License.

License text copyright (c) 2017 MariaDB Corporation Ab, All Rights Reserved.
“Business Source License” is a trademark of MariaDB Corporation Ab.

-----------------------------------------------------------------------------

Business Source License 1.1

Terms

The Licensor hereby grants you the right to copy, modify, create derivative
works, redistribute, and make non-production use of the Licensed Work. The
Licensor may make an Additional Use Grant, above, permitting limited
production use.

Effective on the Change Date, or the fourth anniversary of the first publicly
available distribution of a specific version of the Licensed Work under this
License, whichever comes first, the Licensor hereby grants you rights under
the terms of the Change License, and the rights granted in the paragraph
above terminate.

If your use of the Licensed Work does not comply with the requirements
currently in effect as described in this License, you must purchase a
commercial license from the Licensor, its affiliated entities, or authorized
resellers, or you must refrain from using the Licensed Work.

All copies of the original and modified Licensed Work, and derivative works
of the Licensed Work, are subject to this License. This License applies
separately for each version of the Licensed Work and the Change Date may vary
for each version of the Licensed Work released by Licensor.

You must conspicuously display this License on each original or modified copy
of the Licensed Work. If you receive the Licensed Work in original or
modified form from a third party, the terms and conditions set forth in this
License apply to your use of that work.

Any use of the Licensed Work in violation of this License will automatically
terminate your rights under this License for the current and all other
versions of the Licensed Work.

This License does not grant you any right in any trademark or logo of
Licensor or its affiliates (provided that you may use a trademark or logo of
Licensor as expressly required by this License).

TO THE EXTENT PERMITTED BY APPLICABLE LAW, THE LICENSED WORK IS PROVIDED ON
AN “AS IS” BASIS. LICENSOR HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS,
EXPRESS OR IMPLIED, INCLUDING (WITHOUT LIMITATION) WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT, AND
TITLE.

MariaDB hereby grants you permission to use this License’s text to license
your works, and to refer to it using the trademark “Business Source License”,
as long as you comply with the Covenants of Licensor below.

Covenants of Licensor

In consideration of the right to use this License’s text and the “Business
Source License” name and trademark, Licensor covenants to MariaDB, and to all
other recipients of the licensed work to be provided by Licensor:

1. To specify as the Change License the GPL Version 2.0 or any later version,
   or a license that is compatible with GPL Version 2.0 or a later version,
   where “compatible” means that software provided under the Change License can
   be included in a program with software provided under GPL Version 2.0 or a
   later version. Licensor may specify additional Change Licenses without
   limitation.

2. To either: (a) specify an additional grant of rights to use that does not
   impose any additional restriction on the right granted in this License, as
   the Additional Use Grant; or (b) insert the text “None”.

3. To specify a Change Date.

4. Not to modify this License in any other way.
 **/
