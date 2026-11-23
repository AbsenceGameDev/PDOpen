/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "RTSOSharedUI.h"

#include "ConversationTypes.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/TileView.h"
#include "CommonTextBlock.h"
#include "Interfaces/RTSOConversationInterface.h"

void URTSOModularTile::NativePreConstruct()
{
	Super::NativePreConstruct();

	Refresh();	
}

void URTSOModularTile::Refresh()
{
	TextName->SetText(TileText);

	SizeBoxContainer->SetHeightOverride(Height);	
	SizeBoxContainer->SetWidthOverride(Width);		
}


FEventReply URTSOConversationSelectionEntry::MouseMove(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	URTSOConversationMessageWidget* ParentAsMessageWidget = Cast<URTSOConversationMessageWidget>(DirectParentReference);
	if (ParentAsMessageWidget == nullptr) { return FEventReply(); }
	
	return ParentAsMessageWidget->MouseMove(ChoiceIndex, MyGeometry, MouseEvent);
}

FEventReply URTSOConversationSelectionEntry::MouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	URTSOConversationMessageWidget* ParentAsMessageWidget = Cast<URTSOConversationMessageWidget>(DirectParentReference);
	if (ParentAsMessageWidget == nullptr) { return FEventReply(); }
	
	return ParentAsMessageWidget->MouseButtonDown(ChoiceIndex, MyGeometry, MouseEvent);
}

FEventReply URTSOConversationSelectionEntry::MouseButtonUp(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	URTSOConversationMessageWidget* ParentAsMessageWidget = Cast<URTSOConversationMessageWidget>(DirectParentReference);
	if (ParentAsMessageWidget == nullptr) { return FEventReply(); }
	
	return ParentAsMessageWidget->MouseButtonUp(ChoiceIndex, MyGeometry, MouseEvent);
}

FEventReply URTSOConversationSelectionEntry::MouseDoubleClick(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	URTSOConversationMessageWidget* ParentAsMessageWidget = Cast<URTSOConversationMessageWidget>(DirectParentReference);
	if (ParentAsMessageWidget == nullptr) { return FEventReply(); }
	
	return ParentAsMessageWidget->MouseDoubleClick(ChoiceIndex, MyGeometry, MouseEvent);
}

void URTSOConversationSelectionEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	const URTSOStructWrapper* Item = Cast<URTSOStructWrapper>(ListItemObject);
	if (Item == nullptr) { return; }
	
	// 1. Read data
	TextContent->SetText(Item->GetSelectionEntry());
	ChoiceIndex           = Item->GetChoiceIndex();
	DirectParentReference = Item->GetDirectParentReference();
	Tile->TileText = FText::FromString("Reply {" + FString::FromInt(ChoiceIndex) + "}");
	Tile->Refresh();
	
	// 2. Bind delegates
	TextContentBorder->OnMouseMoveEvent.BindDynamic(this, &URTSOConversationSelectionEntry::MouseMove); 
	TextContentBorder->OnMouseButtonDownEvent.BindDynamic(this, &URTSOConversationSelectionEntry::MouseButtonDown);
	TextContentBorder->OnMouseButtonUpEvent.BindDynamic(this, &URTSOConversationSelectionEntry::MouseButtonUp);
	TextContentBorder->OnMouseDoubleClickEvent.BindDynamic(this, &URTSOConversationSelectionEntry::MouseDoubleClick);
}

FEventReply URTSOConversationMessageWidget::MouseMove_Implementation(int32 ChoiceIdx, FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	return FEventReply(true);
}

FEventReply URTSOConversationMessageWidget::MouseButtonDown_Implementation(int32 ChoiceIdx, FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	LatestInteractedChoice = ChoiceIdx;
	return FEventReply(true);
}

FEventReply URTSOConversationMessageWidget::MouseButtonUp_Implementation(int32 ChoiceIdx, FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	if (LatestInteractedChoice != ChoiceIdx) { return FEventReply();}
	SelectChoice(ChoiceIdx);
	
	return FEventReply(true);
}

FEventReply URTSOConversationMessageWidget::MouseDoubleClick_Implementation(int32 ChoiceIdx, FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	return FEventReply(true);
}


void URTSOConversationMessageWidget::SetPayload_Implementation(const FClientConversationMessagePayload& Payload, AActor* PotentialCallbackActor)
{
	UE_LOG(PDLog_RTSOSharedUI, Warning, TEXT("URTSOConversationMessageWidget::SetPayload_Implementation"))
	// reserved for possible implementation

	CurrentPotentialCallbackActor = PotentialCallbackActor;

	ConversationSelectors->ClearListItems();

	if (Payload.Options.IsEmpty()) { return; }

	// // @todo come back to this, fix data not being overwritten in existing InstantiatedEntryObjects
	// This is stupid, should not have to clear the objects here but
	// somehow the data being overwritten is not
	// really being overwritten for some ungodly reason. Data locked perhaps?
	// InstantiatedEntryObjects.Empty();
	
	const int32 MaxStep = Payload.Options.Num() - 1;
	int32 Step = INDEX_NONE;
	for (;Step < MaxStep;)
	{
		const FClientConversationOptionEntry& Option = Payload.Options[++Step];
		FText BuildText = Option.ChoiceText;
		switch (Option.ChoiceType)
		{
		case EConversationChoiceType::ServerOnly:
			break;
		case EConversationChoiceType::UserChoiceAvailable:
			break;
		case EConversationChoiceType::UserChoiceUnavailable:
			BuildText = FText::FromString("(INVALID:) " + Option.ChoiceText.ToString());
			break;
		}

		// // @todo come back to this, fix data not being overwritten
		// const bool bExists = InstantiatedEntryObjects.IsValidIndex(Step);
		// URTSOStructWrapper* DataWrapper = bExists ?
		// 	InstantiatedEntryObjects[Step] : NewObject<URTSOStructWrapper>(this, URTSOStructWrapper::StaticClass());
		// DataWrapper->AssignData(BuildText, Step, this);
		// Data must be locked as it won't change here
		// if (bExists == false)
		// {
		// 	InstantiatedEntryObjects.Emplace(DataWrapper);
		// }

		URTSOStructWrapper* DataWrapper = NewObject<URTSOStructWrapper>(this, URTSOStructWrapper::StaticClass());		
		DataWrapper->AssignData(BuildText, Step, this);
		ConversationSelectors->AddItem(DataWrapper);

		const FString BuildString =
			FString::Printf(TEXT("URTSOConversationMessageWidget::SetPayload -- Option(%s)"), *Option.ChoiceText.ToString());
		UE_LOG(PDLog_RTSOSharedUI, Error, TEXT("%s"), *BuildString);	
		
	}
	const FString BuildString =
		FString::Printf(TEXT("URTSOConversationMessageWidget::SetPayload -- Options Count %i"), Payload.Options.Num());
	UE_LOG(PDLog_RTSOSharedUI, Error, TEXT("%s"), *BuildString);	
	
}

void URTSOConversationMessageWidget::SelectChoice_Implementation(int32 ChoiceSelection)
{
	UE_LOG(PDLog_RTSOSharedUI, Warning, TEXT("URTSOConversationMessageWidget::SelectChoice_Implementation"))
	// reserved for possible implementation

	if (CurrentPotentialCallbackActor == nullptr
		|| CurrentPotentialCallbackActor->GetClass()->ImplementsInterface(URTSOConversationSpeakerInterface::StaticClass()) == false)
	{
		return;
	}
	
	IRTSOConversationSpeakerInterface::Execute_ReplyChoice(CurrentPotentialCallbackActor, GetOwningPlayer(), ChoiceSelection);
}

void URTSOConversationMessageWidget::NativeDestruct()
{
	CurrentPotentialCallbackActor = nullptr; 
	Super::NativeDestruct();
}

void URTSOConversationWidget::SetPayload_Implementation(const FClientConversationMessagePayload& Payload, AActor* PotentialCallbackActor)
{
	UE_LOG(PDLog_RTSOSharedUI, Warning, TEXT("URTSOConversationWidget::SetPayload_Implementation"))
	// reserved for possible implementation
	
	TextContent->SetText(Payload.Message.Text);
	Tile->TileText = Payload.Message.ParticipantDisplayName;
	ConversationMessageWidget->SetPayload(Payload, PotentialCallbackActor);
	
	Tile->Refresh();
	Tile->InvalidateLayoutAndVolatility();
	ConversationMessageWidget->InvalidateLayoutAndVolatility();
	InvalidateLayoutAndVolatility();
}

void URTSOConversationWidget::SelectChoice_Implementation(int32 ChoiceSelection)
{
	UE_LOG(PDLog_RTSOSharedUI, Warning, TEXT("URTSOConversationWidget::SelectChoice_Implementation"))
	// reserved for possible implementation
	ConversationMessageWidget->SelectChoice(ChoiceSelection);
}	



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
