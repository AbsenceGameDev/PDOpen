/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Widgets/RTSOMainMenuBase.h"

#include "RTSOSharedUI.h"
#include "Actors/RTSOController.h"
#include "Core/RTSOBaseGM.h"
#include "Widgets/RTSOActiveMainMenu.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void URTSOMainMenuBase::NativeOnActivated()
{
	Super::NativeOnActivated();

	// in lieu of a Clear function, or rather in lieu of an actual implementation of Clear which clears elements in the stack
	TArray<UCommonActivatableWidget*> Widgets{};
	for (UCommonActivatableWidget* ItWidget : WidgetStack->GetWidgetList())
	{
		const URTSOMenuWidget* AsMenu = Cast<URTSOMenuWidget>(ItWidget);
		if (AsMenu == nullptr) { continue; }

		Widgets.Emplace(ItWidget);
	}
	for (UCommonActivatableWidget* ItWidget : Widgets)
	{
		WidgetStack->RemoveWidget(*ItWidget);
	}

	UClass* SelectedClass = WidgetClass != nullptr ? WidgetClass.Get() : URTSOMenuWidget::StaticClass();
	ActiveMenuInstance = WidgetStack->AddWidget<URTSOMenuWidget>(SelectedClass);
	ActiveMenuInstance->OwningMenuBaseWidget = this;
}

void URTSOMainMenuBase::BindButtonDelegates(AActor* ActorToBindAt)
{
	ARTSOController* AsController = Cast<ARTSOController>(ActorToBindAt);
	if (AsController == nullptr) { return; }
	
	for (UCommonActivatableWidget* ItWidget : WidgetStack->GetWidgetList())
	{
		const URTSOMenuWidget* AsMenu = Cast<URTSOMenuWidget>(ItWidget);
		if (AsMenu == nullptr) { continue; }

		UE_LOG(PDLog_RTSOSharedUI, Warning, TEXT("URTSOMainMenuBase::BindButtonDelegates -- Binding delegates"));
		
		// All these objects are marked with a metaspecifier BindWidget, thus we can be sure this piece of code never gets run if the stacked URTSOMenuWidget is not valid
		AsMenu->ClearDelegates();
		AsMenu->ResumeButton->Hitbox->OnReleased.AddUniqueDynamic(AsController, &ARTSOController::OnReleased_Resume);
		AsMenu->QuitButton->Hitbox->OnReleased.AddUniqueDynamic(AsController, &ARTSOController::OnReleased_QuitGame);
		AsMenu->SaveEditor->Hitbox->OnPressed.AddUniqueDynamic(AsController, &ARTSOController::OpenSaveEditor);
		SetupInnerMenuDelegates(AsMenu);
	}
}

void URTSOMainMenuBase::SetupInnerMenuDelegates(const URTSOMenuWidget* OuterMenu)
{
	OuterMenu->SettingsButton->OwningWidget =
		OuterMenu->SaveButton->OwningWidget =
		OuterMenu->LoadButton->OwningWidget = this;
	
	if (OuterMenu->SettingsButton->PotentialTargetWidgetClass != nullptr)
	{
		OuterMenu->SettingsButton->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::SettingsTargetWidget);
	}
	
	if (OuterMenu->SaveButton->PotentialTargetWidgetClass != nullptr)
	{
		OuterMenu->SaveButton->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::SaveTargetWidget);
	}

	if (OuterMenu->LoadButton->PotentialTargetWidgetClass)
	{
		OuterMenu->LoadButton->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::LoadTargetWidget);
	}
}

void URTSOMainMenuBase::SettingsTargetWidget()
{
	if (ActiveMenuInstance == nullptr) { return; }
	WidgetStack->AddWidget(ActiveMenuInstance->SettingsButton->PotentialTargetWidgetClass);
}

void URTSOMainMenuBase::SaveTargetWidget()
{
	if (ActiveMenuInstance == nullptr ||
		ActiveMenuInstance->SaveButton->PotentialTargetWidgetClass->IsChildOf(URTSOMenuWidget_SaveGame::StaticClass()) == false)
	{
		return;
	}
	
	URTSOMenuWidget_SaveGame* SaveWidget = WidgetStack->AddWidget<URTSOMenuWidget_SaveGame>(ActiveMenuInstance->SaveButton->PotentialTargetWidgetClass);
	SaveWidget->OwningStack = this;
	SaveWidget->Slot0->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::SaveSlot0);
	SaveWidget->Slot1->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::SaveSlot1);
	SaveWidget->Slot2->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::SaveSlot2);
	SaveWidget->Slot3->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::SaveSlot3);
	SaveWidget->Slot4->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::SaveSlot4);
	SaveWidget->ExitButton->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::CloseSaveSlot);
}

void URTSOMainMenuBase::LoadTargetWidget()
{
	if (ActiveMenuInstance == nullptr ||
		ActiveMenuInstance->LoadButton->PotentialTargetWidgetClass->IsChildOf(URTSOMenuWidget_SaveGame::StaticClass()) == false)
	{
		return;
	}
	
	URTSOMenuWidget_SaveGame* LoadWidget = WidgetStack->AddWidget<URTSOMenuWidget_SaveGame>(ActiveMenuInstance->LoadButton->PotentialTargetWidgetClass);
	LoadWidget->OwningStack = this;
	LoadWidget->Slot0->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::LoadSlot0);
	LoadWidget->Slot1->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::LoadSlot1);
	LoadWidget->Slot2->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::LoadSlot2);
	LoadWidget->Slot3->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::LoadSlot3);
	LoadWidget->Slot4->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::LoadSlot4);
	LoadWidget->ExitButton->Hitbox->OnReleased.AddDynamic(this, &URTSOMainMenuBase::CloseLoadSlot);
}

void URTSOMainMenuBase::SaveOrLoadSlot(TEnumAsByte<ERTSOSaveType> Type, int32 SlotIdx)
{
	ARTSOBaseGM* GM = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARTSOBaseGM>() : nullptr;
	if (GM == nullptr) { return; }
	
	URTSOSaveGameDialog* Dialog = CreateWidget<URTSOSaveGameDialog>(this, ConfirmDialogClass != nullptr ? ConfirmDialogClass.Get() : URTSOSaveGameDialog::StaticClass()); 
	Dialog->Type = Type;
	Dialog->SlotIdx = SlotIdx;

	// @todo move dialog-messages into a string-table
	switch (Type)
	{
	case SAVE:
		Dialog->DialogMessage = FText::FromString("Are you sure you want to save slot (" + FString::Printf(TEXT("%i )"), SlotIdx));
		Dialog->SaveSuccessCallback.BindUObject(GM, &ARTSOBaseGM::ProcessChangesAndSaveGame);
		break;
	case LOAD:
		Dialog->DialogMessage = FText::FromString("Are you sure you want to load slot (" + FString::Printf(TEXT("%i )"), SlotIdx));
		Dialog->SaveSuccessCallback.BindUObject(GM, &ARTSOBaseGM::LoadGame);
		break;
	}
	Dialog->AddToViewport(10);
	Dialog->SetupDelegates();
}

void URTSOMainMenuBase::CloseLoadSlot()
{
	CloseSaveSlot(); // Closes any URTSOMenuWidget_SaveGame, from which both the loadmenu and savemenu derives
}

void URTSOMainMenuBase::CloseSaveSlot()
{
	// in lieu of a Clear function, or rather in lieu of an actual implementation of Clear which clears elements in the stack
	TArray<UCommonActivatableWidget*> Widgets{};
	for (UCommonActivatableWidget* ItWidget : WidgetStack->GetWidgetList())
	{
		const URTSOMenuWidget_SaveGame* AsMenu = Cast<URTSOMenuWidget_SaveGame>(ItWidget);
		if (AsMenu == nullptr) { continue; }

		Widgets.Emplace(ItWidget);
	}
	for (UCommonActivatableWidget* ItWidget : Widgets)
	{
		WidgetStack->RemoveWidget(*ItWidget);
	}
}

void URTSOMainMenuBase::PushToWidgetStack_Implementation(TSubclassOf<UCommonActivatableWidget> Subclass)
{
	WidgetStack->AddWidget(Subclass);
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
