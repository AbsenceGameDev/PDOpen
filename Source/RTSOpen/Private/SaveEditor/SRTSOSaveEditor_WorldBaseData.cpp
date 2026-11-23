/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#include "SaveEditor/SRTSOSaveEditor_WorldBaseData.h"

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
#include "GameplayTagContainer.h"

#define LOCTEXT_NAMESPACE "SRTSOSaveEditor_WorldBaseData"

typedef SNumericVectorInputBox<int32, UE::Math::TVector<int32>, 3> SNumericV3i;
typedef SNumericVectorInputBox<double, FVector, 3> SNumericV3d;
typedef SNumericEntryBox<int32> SNumericS1i;
typedef SNumericEntryBox<double> SNumericS1d;


//
// SAVE EDITOR MAIN
void SRTSOSaveEditor_WorldBaseData::Construct(const FArguments& InArgs, FRTSSaveData* InLinkedData)
{
	LinkedSaveDataCopy = InLinkedData;
	UpdateChildSlot(nullptr);
}

FText SRTSOSaveEditor_WorldBaseData::WorldBaseData_TitleText = LOCTEXT("TitleText_WorldBase", "WORLD BASE DATA");
FText SRTSOSaveEditor_WorldBaseData::GameSeed_TitleText = LOCTEXT("TitleText_Seed", "Game Seed: ");
FText SRTSOSaveEditor_WorldBaseData::GameTime_TitleText = LOCTEXT("TitleText_Time", "Elapsed Game Time: ");
FText SRTSOSaveEditor_WorldBaseData::GenerateSeed_ButtonText = LOCTEXT("ButtonText_Seed", "Generate New Seed");

void SRTSOSaveEditor_WorldBaseData::UpdateChildSlot(void* OpaqueData)
{
	SRTSOSaveEditorBase::UpdateChildSlot(OpaqueData);
	
	// Covers representing below fields
	// CopiedSaveData.Seeder;
	// CopiedSaveData.GameTime;
	
	const SNumericEntryBox<int32>::FOnValueCommitted CallbackOnCommitValue = SNumericEntryBox<int32>::FOnValueCommitted::CreateRaw(this, &SRTSOSaveEditor_WorldBaseData::OnSeedValueChanged);
	FOnClicked CallbackOnClickGenerate = FOnClicked::CreateRaw(this, &SRTSOSaveEditor_WorldBaseData::OnSeedValueGenerated);
	
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
					.Text(WorldBaseData_TitleText)
			]
			+ INSET_VERTICAL_SLOT(4)
			[
				SNew(SHorizontalBox)
				+ INSET_AUTO_HORIZONTAL_SLOT(0)
				[
					SNew(STextBlock)
						.Text(GameSeed_TitleText)
				]
				+ INSET_HORIZONTAL_SLOT(0)
				[
					SNew(SHorizontalBox)
					+ INSET_AUTO_HORIZONTAL_SLOT(0)
					[
						SNew(SNumericEntryBox<int32>)
							.Value(this, &SRTSOSaveEditor_WorldBaseData::GeCurrentSeedValue)
							.OnValueCommitted(CallbackOnCommitValue)
					]
					+ INSET_AUTO_HORIZONTAL_SLOT(0)
					[
						SNew(SButton)
							.Text(GenerateSeed_ButtonText)
							.OnClicked(this, &SRTSOSaveEditor_WorldBaseData::OnSeedValueGenerated)
					]
				]
				+ HORIZONTAL_SEPARATOR(10)
			
				+ INSET_AUTO_HORIZONTAL_SLOT(0)
				[
					SNew(STextBlock)
						.Text(GameTime_TitleText)
				]
				+ INSET_AUTO_HORIZONTAL_SLOT(0)
				[
					SNew(SNumericEntryBox<double>)
						.Value(this, &SRTSOSaveEditor_WorldBaseData::GeCurrentTimeValue)
						.OnValueCommitted(this, &SRTSOSaveEditor_WorldBaseData::OnGameTimeValueChanged)
				]	
			]
		]
	];	
}

TOptional<int32> SRTSOSaveEditor_WorldBaseData::GeCurrentSeedValue() const
{
	return LinkedSaveDataCopy->Seeder.GetCurrentSeed();
}
TOptional<double> SRTSOSaveEditor_WorldBaseData::GeCurrentTimeValue() const
{
	return LinkedSaveDataCopy->GameTime;
}


void SRTSOSaveEditor_WorldBaseData::OnSeedValueChanged(int32 NewSeed, ETextCommit::Type CommitType)
{
	switch (CommitType)
	{
		case ETextCommit::OnEnter:
		LinkedSaveDataCopy->Seeder = FRandomStream(NewSeed);
		UE_LOG(PDLog_SaveEditor, Warning, TEXT("SRTSOSaveEditor_WorldBaseData::OnSeedValueChanged -- New Seed Value: %i"), LinkedSaveDataCopy->Seeder.GetCurrentSeed())
	
		default: break;
	}
}

FReply SRTSOSaveEditor_WorldBaseData::OnSeedValueGenerated()
{
	const int32 NewSeed = FMath::Rand();
	LinkedSaveDataCopy->Seeder = FRandomStream(NewSeed);
	UE_LOG(PDLog_SaveEditor, Warning, TEXT("SRTSOSaveEditor_WorldBaseData::OnSeedValueGenerated -- New Seed Value: %i, RandSeedSource: %i"), LinkedSaveDataCopy->Seeder.GetCurrentSeed(), NewSeed)

	return FReply::Handled();
}

void SRTSOSaveEditor_WorldBaseData::OnGameTimeValueChanged(double NewGameTime, ETextCommit::Type CommitType)
{
	switch (CommitType)
	{
	case ETextCommit::OnEnter:
		LinkedSaveDataCopy->GameTime = NewGameTime;
		UE_LOG(PDLog_SaveEditor, Warning, TEXT("SRTSOSaveEditor_WorldBaseData::OnGameTimeValueChanged -- New Game Time Value: %f"), LinkedSaveDataCopy->GameTime)
		
	default: break;
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

