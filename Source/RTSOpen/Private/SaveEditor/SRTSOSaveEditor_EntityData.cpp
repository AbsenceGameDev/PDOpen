/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#include "SaveEditor/SRTSOSaveEditor_EntityData.h"
#include "SaveEditor/SRTSOSaveEditor.h"

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
#include "Interfaces/PDInteractInterface.h"
#include "SaveEditor/SRTSOTagPicker.h"

#define LOCTEXT_NAMESPACE "SRTSOSaveEditor_EntityData"

typedef SNumericVectorInputBox<int32, UE::Math::TVector<int32>, 3> SNumericV3i;
typedef SNumericVectorInputBox<double, FVector, 3> SNumericV3d;
typedef SNumericEntryBox<int32> SNumericS1i;
typedef SNumericEntryBox<double> SNumericS1d;

FText SRTSOSaveEditor_EntityData::WorldEntities_TitleText = LOCTEXT("TitleText_WorldEntity", "WORLD ENTITIES");
FText SRTSOSaveEditor_EntityData::Entity_TitleText = LOCTEXT("TitleText_SingleEntity", "ENTITY");
FText SRTSOSaveEditor_EntityData::Entity_BaseData_TitleText = LOCTEXT("TitleText_InteractableData_BaseData", "BASE DATA");
FText SRTSOSaveEditor_EntityData::Entity_BaseData_Index_TitleText = LOCTEXT("EntityIndex_InteractableData_BaseData", "Index: ");
FText SRTSOSaveEditor_EntityData::Entity_BaseData_Type_TitleText = LOCTEXT("Type_InteractableData_BaseData", "Type: ");

FText SRTSOSaveEditor_EntityData::Entity_StateData_TitleText = LOCTEXT("TitleText_InteractableData_StateData", "STATE DATA");
FText SRTSOSaveEditor_EntityData::Entity_StateData_Location_TitleText = LOCTEXT("Location_InteractableData_StateData", "Location: ");
FText SRTSOSaveEditor_EntityData::Entity_StateData_Health_TitleText = LOCTEXT("Health_InteractableData_StateData", "Health: ");

FText SRTSOSaveEditor_EntityData::Entity_OwnerData_TitleText = LOCTEXT("TitleText_InteractableData_OwnerData", "OWNER DATA");
FText SRTSOSaveEditor_EntityData::Entity_OwnerData_OwnerID_TitleText = LOCTEXT("UserID_InteractableData_StateData", "Owner UserID: ");
FText SRTSOSaveEditor_EntityData::Entity_OwnerData_SelectionGroup_TitleText = LOCTEXT("SelectionGroup_InteractableData_StateData", "Selection Group: ");

FText SRTSOSaveEditor_EntityData::Entity_ActionData_TitleText = LOCTEXT("TitleText_InteractableData_ActionData", "ACTION DATA");
FText SRTSOSaveEditor_EntityData::Entity_ActionData_BaseData_TitleText = LOCTEXT("TitleText_InteractableData_ActionData_BaseData", "BASE DATA");
FText SRTSOSaveEditor_EntityData::Entity_ActionData_BaseData_ActionType_TitleText = LOCTEXT("Type_InteractableData_ActionData_BaseData", "Action Type: ");

FText SRTSOSaveEditor_EntityData::Entity_ActionData_RewardData_TitleText = LOCTEXT("TitleText_InteractableData_ActionData_OptionalReward", "OPTIONAL REWARD");
FText SRTSOSaveEditor_EntityData::Entity_ActionData_RewardData_Type_TitleText = LOCTEXT("Type_InteractableData_ActionData_OptionalReward", "Type: ");
FText SRTSOSaveEditor_EntityData::Entity_ActionData_RewardData_Amount_TitleText = LOCTEXT("Amount_InteractableData_ActionData_OptionalReward", "Amount: ");

FText SRTSOSaveEditor_EntityData::Entity_ActionData_TargetData_TitleText = LOCTEXT("TitleText_InteractableData_ActionData_OptionalTargets", "OPTIONAL TARGETS");
FText SRTSOSaveEditor_EntityData::Entity_ActionData_TargetData_AsEntity_TitleText = LOCTEXT("AsEntity_InteractableData_ActionData_OptionalTargets", "As Entity: ");
FText SRTSOSaveEditor_EntityData::Entity_ActionData_TargetData_AsActor_TitleText = LOCTEXT("AsActor_InteractableData_ActionData_OptionalTargets", "As Actor: ");
FText SRTSOSaveEditor_EntityData::Entity_ActionData_TargetData_AsPureLocation_TitleText = LOCTEXT("AsPureLoc_InteractableData_ActionData_OptionalTargets", "As Pure Location: ");


//
// SAVE EDITOR MAIN
void SRTSOSaveEditor_EntityData::Construct(const FArguments& InArgs, FRTSSaveData* InLinkedData, TArray<TSharedPtr<FRTSSavedWorldUnits>>& ArrayRef)
{
	LinkedSaveDataCopy = InLinkedData;
	UpdateChildSlot(&ArrayRef);
}

void SRTSOSaveEditor_EntityData::UpdateChildSlot(void* OpaqueData)
{
	SRTSOSaveEditorBase::UpdateChildSlot(OpaqueData);
	// Covers representing below fields
	// CopiedSaveData.EntityUnits;


	if (OpaqueData != nullptr)
	{
		EntitiesAsSharedArray = static_cast<TArray<TSharedPtr<FRTSSavedWorldUnits>>*>(OpaqueData);
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
					.Text(WorldEntities_TitleText)
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
					SNew(SListView<TSharedPtr<FRTSSavedWorldUnits>>)
						.ListItemsSource(EntitiesAsSharedArray)
						.OnGenerateRow( this, &SRTSOSaveEditor_EntityData::MakeListViewWidget_EntityData )
						.OnSelectionChanged( this, &SRTSOSaveEditor_EntityData::OnComponentSelected_EntityData )
				]
			]
			+ INSET_VERTICAL_SLOT(FMath::Clamp(EntitiesAsSharedArray->Num() * 2.f, 0.f, 20.f))
		]
		
	];	
}

TSharedRef<ITableRow> SRTSOSaveEditor_EntityData::MakeListViewWidget_EntityData(TSharedPtr<FRTSSavedWorldUnits> InItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
	SRTSOSaveEditor_EntityData* MutableThis = const_cast<SRTSOSaveEditor_EntityData*>(this);
	check(MutableThis != nullptr)

	const FRTSSavedWorldUnits& SavedEntityDatum = *InItem.Get();

	// @todo IMPLEMENT CALLBACKS
	const FOnClicked OnEntityTypeClicked =
		FOnClicked::CreateLambda(
			[&, SavedEntityDatumCopy = SavedEntityDatum]() -> FReply
			{
				// Create the window to pick the class
				if (TagPicker.IsValid() == false)
				{
					TSharedRef<SRTSOTagPicker> InstancedTagPicker = SNew(SRTSOTagPicker);
					(TSharedPtr<SRTSOTagPicker>)TagPicker = (TSharedPtr<SRTSOTagPicker>)InstancedTagPicker.ToSharedPtr();
				}
		
				TagPicker->SetVisibility(TagPicker->GetVisibility().IsVisible() ? EVisibility::Collapsed : EVisibility::Visible);
				TagPicker->RequestScrollToView(SavedEntityDatumCopy.EntityUnitTag);
				
				return FReply::Handled();
			});

	const FOnClicked OnEntityActionTagChanged =
		FOnClicked::CreateLambda(
			[&, SavedEntityDatumCopy = SavedEntityDatum]() -> FReply
			{
				// Create the window to pick the class
				if (TagPicker.IsValid() == false)
				{
					TSharedRef<SRTSOTagPicker> InstancedTagPicker = SNew(SRTSOTagPicker);
					(TSharedPtr<SRTSOTagPicker>)TagPicker = (TSharedPtr<SRTSOTagPicker>)InstancedTagPicker.ToSharedPtr();
				}
		
				TagPicker->SetVisibility(TagPicker->GetVisibility().IsVisible() ? EVisibility::Collapsed : EVisibility::Visible);
				TagPicker->RequestScrollToView(SavedEntityDatumCopy.CurrentAction.ActionTag);
				
				return FReply::Handled();
			});

	const FOnClicked OnOptRewardItemTypeChanged =
		FOnClicked::CreateLambda(
			[&, SavedEntityDatumCopy = SavedEntityDatum]() -> FReply
			{
				// Create the window to pick the class
				if (TagPicker.IsValid() == false)
				{
					TSharedRef<SRTSOTagPicker> InstancedTagPicker = SNew(SRTSOTagPicker);
					(TSharedPtr<SRTSOTagPicker>)TagPicker = (TSharedPtr<SRTSOTagPicker>)InstancedTagPicker.ToSharedPtr();
				}
		
				TagPicker->SetVisibility(TagPicker->GetVisibility().IsVisible() ? EVisibility::Collapsed : EVisibility::Visible);
				TagPicker->RequestScrollToView(SavedEntityDatumCopy.CurrentAction.Reward);
				
				return FReply::Handled();
			});		
	
	SNumericV3d::FOnVectorValueCommitted OnEntityLocationChanged;
	OnEntityLocationChanged.BindLambda(
		[ImmutableThis = this, InstanceIndex = SavedEntityDatum.InstanceIndex](const FVector& UpdatedVector, ETextCommit::Type CommitType)
		{
			if (CommitType != ETextCommit::OnEnter) { return; }
			
			SRTSOSaveEditor_EntityData* MutableThis = const_cast<SRTSOSaveEditor_EntityData*>(ImmutableThis);
			check(MutableThis != nullptr)
			FRTSSaveData* MutableSaveData = MutableThis->LinkedSaveDataCopy;

			
			// Slow, @todo come back to this
			for (FRTSSavedWorldUnits& CurrentUnit : MutableSaveData->EntityUnits)
			{
				if (CurrentUnit.InstanceIndex != InstanceIndex) { continue;}
				
				CurrentUnit.Location = UpdatedVector;
				break;
			}
		});
	
	SNumericS1d::FOnValueCommitted OnEntityHealthChanged;
	OnEntityHealthChanged.BindLambda(
		[ImmutableThis = this, InstanceIndex = SavedEntityDatum.InstanceIndex](double UpdatedValue, ETextCommit::Type CommitType)
		{
			if (CommitType != ETextCommit::OnEnter) { return; }
			
			const SRTSOSaveEditor_EntityData* MutableThis = const_cast<SRTSOSaveEditor_EntityData*>(ImmutableThis);
			check(MutableThis != nullptr)
			FRTSSaveData* MutableSaveData = MutableThis->LinkedSaveDataCopy;

			// Slow, @todo come back to this
			for (FRTSSavedWorldUnits& CurrentInteractable : MutableSaveData->EntityUnits)
			{
				if (CurrentInteractable.InstanceIndex != InstanceIndex) { continue;}
				
				CurrentInteractable.Health = UpdatedValue;
				break;
			}
		});
	
	SNumericS1i::FOnValueCommitted OnEntityOwnerIDChanged;
	OnEntityOwnerIDChanged.BindLambda(
		[ImmutableThis = this, InstanceIndex = SavedEntityDatum.InstanceIndex](int32 NewOwnerID, ETextCommit::Type CommitType)
		{
			if (CommitType != ETextCommit::OnEnter) { return; }
			
			const SRTSOSaveEditor_EntityData* MutableThis = const_cast<SRTSOSaveEditor_EntityData*>(ImmutableThis);
			check(MutableThis != nullptr)
			FRTSSaveData* MutableSaveData = MutableThis->LinkedSaveDataCopy;

			// Slow, @todo come back to this
			for (FRTSSavedWorldUnits& CurrentInteractable : MutableSaveData->EntityUnits)
			{
				if (CurrentInteractable.InstanceIndex != InstanceIndex) { continue;}
				
				CurrentInteractable.OwnerID = NewOwnerID;
				break;
			}
		});	
	
	SNumericS1i::FOnValueCommitted OnEntityOwnerSelectionGroupChanged;
	OnEntityOwnerSelectionGroupChanged.BindLambda(
		[ImmutableThis = this, InstanceIndex = SavedEntityDatum.InstanceIndex](int32 NewOwnerSelectionGroup, ETextCommit::Type CommitType)
		{
			if (CommitType != ETextCommit::OnEnter) { return; }
			
			const SRTSOSaveEditor_EntityData* MutableThis = const_cast<SRTSOSaveEditor_EntityData*>(ImmutableThis);
			check(MutableThis != nullptr)
			FRTSSaveData* MutableSaveData = MutableThis->LinkedSaveDataCopy;

			// Slow, @todo come back to this
			for (FRTSSavedWorldUnits& CurrentInteractable : MutableSaveData->EntityUnits)
			{
				if (CurrentInteractable.InstanceIndex != InstanceIndex) { continue;}
				
				CurrentInteractable.SelectionIndex = NewOwnerSelectionGroup;
				break;
			}
		});
	
	SNumericS1i::FOnValueCommitted OnOptionalRewardAmountChanged;
	OnOptionalRewardAmountChanged.BindLambda(
		[ImmutableThis = this, InstanceIndex = SavedEntityDatum.InstanceIndex](int32 NewOptRewardAmount, ETextCommit::Type CommitType)
		{
			if (CommitType != ETextCommit::OnEnter) { return; }
			
			const SRTSOSaveEditor_EntityData* MutableThis = const_cast<SRTSOSaveEditor_EntityData*>(ImmutableThis);
			check(MutableThis != nullptr)
			FRTSSaveData* MutableSaveData = MutableThis->LinkedSaveDataCopy;

			// Slow, @todo come back to this
			for (FRTSSavedWorldUnits& CurrentInteractable : MutableSaveData->EntityUnits)
			{
				if (CurrentInteractable.InstanceIndex != InstanceIndex) { continue;}
				
				CurrentInteractable.CurrentAction.RewardAmount = NewOptRewardAmount;
				break;
			}
		});
	
	SNumericS1i::FOnValueCommitted OnSavedEntityIndexChanged;
	OnSavedEntityIndexChanged.BindLambda(
		[ImmutableThis = this, InstanceIndex = SavedEntityDatum.InstanceIndex](const int32& NewSavedEntityIndex, ETextCommit::Type CommitType)
		{
			if (CommitType != ETextCommit::OnEnter) { return; }
			
			const SRTSOSaveEditor_EntityData* MutableThis = const_cast<SRTSOSaveEditor_EntityData*>(ImmutableThis);
			check(MutableThis != nullptr)
			FRTSSaveData* MutableSaveData = MutableThis->LinkedSaveDataCopy;

			// Slow, @todo come back to this
			for (FRTSSavedWorldUnits& CurrentUnit : MutableSaveData->EntityUnits)
			{
				if (CurrentUnit.InstanceIndex != InstanceIndex) { continue;}
				
				CurrentUnit.InstanceIndex.Index = NewSavedEntityIndex;
				break;
			}
		});	
	
	SNumericS1i::FOnValueCommitted OnOptTargetEntityIndexChanged;
	OnOptTargetEntityIndexChanged.BindLambda(
		[ImmutableThis = this, InstanceIndex = SavedEntityDatum.InstanceIndex](const int32& NewTargetEntityIndex, ETextCommit::Type CommitType)
		{
			if (CommitType != ETextCommit::OnEnter) { return; }
			
			const SRTSOSaveEditor_EntityData* MutableThis = const_cast<SRTSOSaveEditor_EntityData*>(ImmutableThis);
			check(MutableThis != nullptr)
			FRTSSaveData* MutableSaveData = MutableThis->LinkedSaveDataCopy;

			// Slow, @todo come back to this
			for (FRTSSavedWorldUnits& CurrentUnit : MutableSaveData->EntityUnits)
			{
				if (CurrentUnit.InstanceIndex != InstanceIndex) { continue;}
				
				CurrentUnit.CurrentAction.OptTargets.ActionTargetAsEntity.Index = NewTargetEntityIndex;
				break;
			}
		});	
	
	SNumericV3d::FOnVectorValueCommitted OnEntityTargetLocationChanged;
	OnEntityTargetLocationChanged.BindLambda(
		[ImmutableThis = this, InstanceIndex = SavedEntityDatum.InstanceIndex](const FVector& UpdatedVector, ETextCommit::Type CommitType)
		{
			if (CommitType != ETextCommit::OnEnter) { return; }
			
			const SRTSOSaveEditor_EntityData* MutableThis = const_cast<SRTSOSaveEditor_EntityData*>(ImmutableThis);
			check(MutableThis != nullptr)
			FRTSSaveData* MutableSaveData = MutableThis->LinkedSaveDataCopy;

			// Slow, @todo come back to this
			for (FRTSSavedWorldUnits& CurrentUnit : MutableSaveData->EntityUnits)
			{
				if (CurrentUnit.InstanceIndex != InstanceIndex) { continue;}
				
				CurrentUnit.CurrentAction.OptTargets.ActionTargetAsLocation = UpdatedVector;
				break;
			}
		});


	// @todo need to ensure this works as intended, if not then it means the result is cached and not updated each frame and in that case another strategy has to be employed
	const auto ResolveLocation = [CopiedLocation = SavedEntityDatum.Location]() -> FVector
	{
		return CopiedLocation;
	};	
	const auto ResolveUsability = [CopiedUsability = SavedEntityDatum.Health]() -> double
	{
		return CopiedUsability;
	};
	const auto ResolveSelfEntityIndex = [CopiedTargetIndex = SavedEntityDatum.InstanceIndex.Index]()-> int32
	{
		return CopiedTargetIndex;
	};	
	const auto ResolveOwnerID = [CopiedID = SavedEntityDatum.OwnerID]() -> int32
	{
		return CopiedID;
	};	
	const auto ResolveOwnerSelectionGroup = [CopiedSelectionGroupIdx = SavedEntityDatum.SelectionIndex]() -> int32
	{
		return CopiedSelectionGroupIdx;
	};
	const auto ResolveRewardAmount = [CopiedRewardAmount = SavedEntityDatum.CurrentAction.RewardAmount]()-> int32
	{
		return CopiedRewardAmount;
	};
	const auto ResolveActionTarget_Entity = [CopiedTargetIndex = SavedEntityDatum.CurrentAction.OptTargets.ActionTargetAsEntity.Index]()-> int32
	{
		return CopiedTargetIndex;
	};
	const auto ResolveActionTarget_PureLocation = [CopiedTargetLocation = SavedEntityDatum.CurrentAction.OptTargets.ActionTargetAsLocation.Get()]()-> FVector
	{
		return CopiedTargetLocation;
	};
	
	
	MutableThis->EntityTable = SNew( STableRow< TSharedPtr<FRTSSavedWorldUnits> >, OwnerTable )
		[
			SNew(SVerticalBox)
			+ INSET_VERTICAL_SLOT(4)
			[
				SNew(STextBlock)
					.Text(Entity_TitleText)
			]
			+ VERTICAL_SEPARATOR(1)


			+ INSET_VERTICAL_SLOT(4)
			[
				SNew(STextBlock)
					.Text(Entity_BaseData_TitleText)
			]	
			+ INSET_VERTICAL_SLOT(2)
			[
				SNew(SHorizontalBox)
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(SHorizontalBox)
					+ INSET_HORIZONTAL_SLOT(0)
					[
						SNew(STextBlock)
							.Text(Entity_BaseData_Index_TitleText)
					]
					+ INSET_HORIZONTAL_SLOT(0)
					[
						SNew(SNumericEntryBox<int32>)
							.Value_Lambda(ResolveSelfEntityIndex)
							.OnValueCommitted(OnSavedEntityIndexChanged)							
					]
				]
			]
			+ INSET_VERTICAL_SLOT(2)
			[
				SNew(SHorizontalBox)
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(SHorizontalBox)
					+ INSET_HORIZONTAL_SLOT(0)
					[
						SNew(STextBlock)
							.Text(Entity_BaseData_Type_TitleText)
					]
					+ INSET_HORIZONTAL_SLOT(0)
					[
						SNew(SButton)
							.Text(FText::FromString(SavedEntityDatum.EntityUnitTag.ToString()))
							.OnClicked(OnEntityTypeClicked)
					]			
				]
			]
			+ VERTICAL_SEPARATOR(1)
			
			
			+ INSET_VERTICAL_SLOT(4)
			[
				SNew(STextBlock)
					.Text(Entity_StateData_TitleText)
			]				
			+ INSET_VERTICAL_SLOT(2)
			[
				SNew(SHorizontalBox)
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(STextBlock)
						.Text(Entity_StateData_Location_TitleText)
				]
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(SNumericV3d)
						.Vector_Lambda(ResolveLocation)
						.OnVectorCommitted(OnEntityLocationChanged)
				]
				+ HORIZONTAL_SEPARATOR(1)
				
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(SHorizontalBox)
					+ INSET_HORIZONTAL_SLOT(0)
					[
						SNew(STextBlock)
							.Text(Entity_StateData_Health_TitleText)
					]
					+ INSET_HORIZONTAL_SLOT(0)
					[
						SNew(SNumericEntryBox<double>)
							.Value_Lambda(ResolveUsability)
							.OnValueCommitted(OnEntityHealthChanged)	 
					]			
				]					
			]
			+ VERTICAL_SEPARATOR(1) 
			
			
			+ INSET_VERTICAL_SLOT(4)
			[
				SNew(STextBlock)
					.Text(Entity_OwnerData_TitleText)
			]				
			+ INSET_VERTICAL_SLOT(2)
			[
				SNew(SHorizontalBox)
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(SHorizontalBox)
					+ INSET_HORIZONTAL_SLOT(0)
					[
						SNew(STextBlock)
							.Text(Entity_OwnerData_OwnerID_TitleText)
					]
					+ INSET_HORIZONTAL_SLOT(0)
					[
						SNew(SNumericS1i)
							.Value_Lambda(ResolveOwnerID)
							.OnValueCommitted(OnEntityOwnerIDChanged)
					]
				]
				+ HORIZONTAL_SEPARATOR(1)

				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(SHorizontalBox)
					+ INSET_HORIZONTAL_SLOT(0)
					[
						SNew(STextBlock)
							.Text(Entity_OwnerData_SelectionGroup_TitleText)
					]
					+ INSET_HORIZONTAL_SLOT(0)
					[
						SNew(SNumericS1i)
							.Value_Lambda(ResolveOwnerSelectionGroup)
							.OnValueCommitted(OnEntityOwnerSelectionGroupChanged)
					]
				]
			]
			+ VERTICAL_SEPARATOR(1) 
			
			
			+ INSET_VERTICAL_SLOT(4)
			[
				SNew(STextBlock)
					.Text(Entity_ActionData_TitleText)
			]
			+ INSET_VERTICAL_SLOT(2)
			[
				SNew(STextBlock)
					.Text(Entity_ActionData_BaseData_TitleText)
			]					
			+ INSET_VERTICAL_SLOT(2)
			[
				SNew(SHorizontalBox)
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(STextBlock)
						.Text(Entity_ActionData_BaseData_ActionType_TitleText)
				]
				+ INSET_HORIZONTAL_SLOT(0)
				[
					// @todo implement SLightGameplayTagPicker then finish impl of 
					SNew(SButton)
						.Text(FText::FromString(SavedEntityDatum.CurrentAction.ActionTag.ToString()))
						.OnClicked(OnEntityActionTagChanged)						
					// SNew(STextBlock)
					// 	.Text(FText::FromString(SavedEntityDatum.CurrentAction.ActionTag.ToString()))
				]
			]
			+ VERTICAL_SEPARATOR(1) 
			
			
			+ INSET_VERTICAL_SLOT(4)
			[
					SNew(STextBlock)
						.Text(Entity_ActionData_RewardData_TitleText)
			]			
			+ INSET_VERTICAL_SLOT(2)
			[
				SNew(SHorizontalBox)
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(STextBlock)
						.Text(Entity_ActionData_RewardData_Type_TitleText)
				]
				+ INSET_HORIZONTAL_SLOT(0)
				[
					// @todo implement SLightGameplayTagPicker then finish impl of 
					SNew(SButton)
						.Text(FText::FromString(SavedEntityDatum.CurrentAction.Reward.ToString()))
						.OnClicked(OnOptRewardItemTypeChanged)
				]
				+ HORIZONTAL_SEPARATOR(1) 
				
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(STextBlock)
						.Text(Entity_ActionData_RewardData_Amount_TitleText)
				]
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(SNumericS1i)
						.Value_Lambda(ResolveRewardAmount)
						.OnValueCommitted(OnOptionalRewardAmountChanged)
				]				
			]
			+ VERTICAL_SEPARATOR(2) 

			
			+ INSET_VERTICAL_SLOT(4)
			[
				SNew(STextBlock)
					.Text(Entity_ActionData_TargetData_TitleText)
			]			
			+ INSET_VERTICAL_SLOT(2)
			[
				SNew(SHorizontalBox)
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(STextBlock)
						.Text(Entity_ActionData_TargetData_AsEntity_TitleText)
				]
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(SNumericEntryBox<int32>)
						.Value_Lambda(ResolveActionTarget_Entity) 
						.OnValueCommitted(OnOptTargetEntityIndexChanged)
				]
				+ HORIZONTAL_SEPARATOR(1) 
				
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(STextBlock)
						.Text(Entity_ActionData_TargetData_AsActor_TitleText)

				]
				+ INSET_HORIZONTAL_SLOT(0)
				[
					// @todo need a custom slate to allow selecting arbitrary target
					// @todo need to ActionTargetAsActor from being a pointer to being the ActorID to even be able to represent it in the save editor and even in the save game for that matter
					SNew(STextBlock)
						.Text(FText::FromString(SavedEntityDatum.CurrentAction.OptTargets.ActionTargetAsActor != nullptr ? SavedEntityDatum.CurrentAction.OptTargets.ActionTargetAsActor->GetName() : "N/A"))
				]
				+ HORIZONTAL_SEPARATOR(1) 
				
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(STextBlock)
						.Text(Entity_ActionData_TargetData_AsPureLocation_TitleText)

				]
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(SNumericV3d)
						.Vector_Lambda(ResolveActionTarget_PureLocation)
						.OnVectorCommitted(OnEntityTargetLocationChanged)
				]			
			]				
			+ INSET_VERTICAL_SLOT(4)
		];

	return MutableThis->EntityTable.ToSharedRef();
}

void SRTSOSaveEditor_EntityData::OnComponentSelected_EntityData(TSharedPtr<FRTSSavedWorldUnits> InItem, ESelectInfo::Type InSelectInfo)
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
	
	if(OnEntityDataChosen.IsBound())
	{
		OnEntityDataChosen.Execute(*InItem.Get());
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

