/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#include "Widgets/Slate/SRTSOMissionList.h"
#include "Actors/RTSOController.h"

#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Views/SListView.h"
#include "Styling/AppStyle.h"

// Class picker 
#include "Kismet2/SClassPickerDialog.h"


#include "GameplayTagContainer.h"
#include "PDRTSBaseSubsystem.h"
#include "Interfaces/PDInteractInterface.h"
#include "Chaos/AABB.h"
#include "Interfaces/RTSOConversationInterface.h"

#define LOCTEXT_NAMESPACE "SRTSOMissionList"

typedef SNumericVectorInputBox<int32, UE::Math::TVector<int32>, 3> SNumericV3i;
typedef SNumericVectorInputBox<double, FVector, 3> SNumericV3d;
typedef SNumericEntryBox<int32> SNumericS1i;
typedef SNumericEntryBox<double> SNumericS1d;

FText SRTSOMissionList::MissionBase_TitleText = LOCTEXT("TitleText_MissionScreen", "MISSIONS");
FText SRTSOMissionList::MissionProgress_BaseData_Active_TitleText = LOCTEXT("TitleText_Active_BaseData", "ACTIVE");
FText SRTSOMissionList::MissionProgress_BaseData_Inactive_TitleText = LOCTEXT("TitleText_Inactive_BaseData", "ACTIVE");
FText SRTSOMissionList::MissionProgress_BaseData_ActorID_TitleText = LOCTEXT("ActorID_SingleConversation_BaseData", "Conversation Actor ID: ");
FText SRTSOMissionList::MissionProgress_BaseData_Type_TitleText = LOCTEXT("Type_SingleConversation_BaseData", "Type: ");

FText SRTSOMissionList::MissionProgress_MissionData_TitleText = LOCTEXT("TitleText_SingleConversation_MissionData", "MISSION DATA");
FText SRTSOMissionList::MissionProgress_MissionData_MissionList_TitleText = LOCTEXT("MissionList_SingleConversation_MissionData", "Mission List: ");
FText SRTSOMissionList::MissionProgress_MissionData_ProgressionPerPlayer_TitleText = LOCTEXT("PlayerProgress_SingleConversation_MissionData", "Progression Per Player: ");

//
// SAVE EDITOR MAIN
void SRTSOMissionList::Construct(const FArguments& InArgs, int32 InOwnerID, FRTSSaveData* InLinkedData, TArray<TSharedPtr<FRTSOMissionProgress>>& ArrayRef)
{
	LinkedSaveDataCopy = InLinkedData;

	OwnerID = InOwnerID;

	APawn* OwnerPawn = nullptr;
	if (UPDRTSBaseSubsystem::Get()->SharedOwnerIDMappings.Contains(OwnerID))
	{
		AActor* OwnerActor = *UPDRTSBaseSubsystem::Get()->SharedOwnerIDMappings.Find(OwnerID);
		ARTSOController* AsController = Cast<ARTSOController>(OwnerActor);
		OwnerPawn = AsController->GetPawn();
	}
	if (OwnerPawn == nullptr) { return; }

	MissionsAsSharedArray->Empty();
	for (TTuple<int32, FRTSSavedConversationActorData>& ConversationStateTuple : LinkedSaveDataCopy->ConversationActorState)
	{
		ConversationStateTuple.Value.CopySelectedToSoftClass();
		FRTSOMissionProgress MissionStateDatum; //{ConversationStateTuple.Key, ConversationStateTuple.Value};


		FRTSOConversationMetaProgressionListWrapper* SelectedMetaProgressionMapWrapper = nullptr;
		for (TTuple<int32, FRTSOConversationMetaProgressionListWrapper> Progression : ConversationStateTuple.Value.ProgressionPerPlayer)
		{
			const int32 PotentialOwnerID = Progression.Key;
			if ( PotentialOwnerID != OwnerID ) { continue; }
			
			SelectedMetaProgressionMapWrapper = &Progression.Value;
			break;
		}
		
		if (SelectedMetaProgressionMapWrapper == nullptr) { continue; }

		for (TTuple<FGameplayTag, FRTSOConversationMetaProgressionDatum>& MetaProgressionTuple : SelectedMetaProgressionMapWrapper->ProgressionDataMap)
		{
			// @NOTE Need to think this through, write some thoughts on paper before I finish this
			// @NOTE cont. @note ActorTagSet is the players accumulated tags,
			// @NOTE cont. these are currently acquired by conversation steps, I need to implement below todos
			// @DONE I need a '@todo IRTSMissionProgressor' interface
			// @DONE cont. which is put on actors which progress our mission so we can pass tags cleanly
			// @DONE cont. We also need to actually increment BaseProgression when we get past a phase in a mission,
			// @DONE cont. currently we are not doing this it seems like, however much I thought I had set it up
			FGameplayTag MissionTag = MetaProgressionTuple.Key;
			FRTSOConversationMetaProgressionDatum MetaProgressionDatum = MetaProgressionTuple.Value;
			
			FRTSOConversationMetaProgressionDatum ConstructedMetaProgression;
			ConstructedMetaProgression.MissionTag = MissionTag;    
			ConstructedMetaProgression.BaseProgression = MetaProgressionDatum.BaseProgression;

			// @todo After above todo is finished, we then need to parse through the PhaseRequiredTags for a default mission datum
			// @todo cont. and fill the 'ConstructedMetaProgression.PhaseRequiredTags' below appropriately, or something similar to signify which
			const FGameplayTagContainer& ActorTagSet = Cast<IRTSOConversationInterface>(OwnerPawn)->GetProgressionTags();
			ConstructedMetaProgression.PhaseRequiredTags; // Could be re-used as objectives?

			MissionStateDatum.ProgressionDataMap.Emplace(MissionTag,ConstructedMetaProgression);
			
		}

		
		MissionsAsSharedArray->Emplace(MakeShared<FRTSOMissionProgress>(MissionStateDatum).ToSharedPtr());
	}
	
	
	UpdateChildSlot(&ArrayRef);
}

void SRTSOMissionList::UpdateChildSlot(void* OpaqueData)
{
	if (TitleFont.TypefaceFontName.IsNone())
	{
		// @todo Set up a custom slate styleset for the saveeditors fonts and icons 
		TitleFont = FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont"));
		TitleFont.Size *= 8;
	}

	
	// Covers representing below fields
	// CopiedSaveData.ConversationActorState;

	if (OpaqueData != nullptr)
	{
		MissionsAsSharedArray = static_cast<TArray<TSharedPtr<FRTSOMissionProgress>>*>(OpaqueData);
	}
	
	
	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SHorizontalBox)
		+ INSET_HORIZONTAL_SLOT(0)
		[
			SNew(SVerticalBox)
			+ INSET_AUTO_VERTICAL_SLOT(0)
			[
				SNew(STextBlock)
					.Font(TitleFont)
					.Text(MissionBase_TitleText)
			]
			
			+ INSET_VERTICAL_SLOT(0)
			[
				SNew(SScrollBox)
				.ScrollBarAlwaysVisible(true)
				.ScrollBarVisibility(EVisibility::Visible)
				.ScrollBarThickness(UE::Slate::FDeprecateVector2DParameter(10))
				.Orientation(EOrientation::Orient_Vertical)
				+SScrollBox::Slot()
				[
					SNew(SListView<TSharedPtr<FRTSOMissionProgress>>)
						.ListItemsSource(MissionsAsSharedArray)
						.OnGenerateRow( this, &SRTSOMissionList::MakeListViewWidget_AllMissionData )
						.OnSelectionChanged( this, &SRTSOMissionList::OnComponentSelected_AllMissionData )
				]

			]
			+ INSET_VERTICAL_SLOT(FMath::Clamp(MissionsAsSharedArray->Num() * 4.f, 0.f, 30.f))
		]
	];	
}


TSharedRef<ITableRow> SRTSOMissionList::MakeListViewWidget_AllMissionData(TSharedPtr<FRTSOMissionProgress> InItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
	FRTSOMissionProgress& SavedConversationStateDatum = *InItem.Get();
	
	//
	// Gather shared ptrs
	TArray<TSharedPtr<FName>> CurrentMissionsAsSharedTupleArray;
	for (const TTuple<FGameplayTag, FRTSOConversationMetaProgressionDatum>& SelectedMission : SavedConversationStateDatum.ProgressionDataMap)
	{
		// CurrentMissionsAsSharedTupleArray.Emplace(MakeShared<FName>(SelectedMission).ToSharedPtr());
		// SharedTupleArray.Emplace(MakeShared<FMissionData>(MissionState).ToSharedPtr());
	}
	
	//
	// Widget layout
	SRTSOMissionList* MutableThis = const_cast<SRTSOMissionList*>(this);
	check(MutableThis != nullptr)
	
	MutableThis->ConversationStateTable = SNew( STableRow< TSharedPtr<FRTSOMissionProgress> >, OwnerTable );

	// ...
	// ...
	
	return MutableThis->ConversationStateTable.ToSharedRef();	
}

void SRTSOMissionList::OnComponentSelected_AllMissionData(TSharedPtr<FRTSOMissionProgress> InItem, ESelectInfo::Type InSelectInfo)
{
	FSlateApplication::Get().DismissAllMenus();

	switch (InSelectInfo)
	{
	case ESelectInfo::OnKeyPress:
		break;
	case ESelectInfo::OnNavigation:
		break;
	case ESelectInfo::OnMouseClick:
		break;
	case ESelectInfo::Direct:
		break;
	}
	
	if(OnMissionDataChosen.IsBound())
	{
		OnMissionDataChosen.Execute(*InItem.Get());
	}			
}


#undef LOCTEXT_NAMESPACE

/**
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

