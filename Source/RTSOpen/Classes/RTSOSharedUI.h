/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "UObject/Interface.h"
#include "Blueprint/UserWidget.h"
#include "RTSOSharedUI.generated.h"

DECLARE_LOG_CATEGORY_CLASS(PDLog_RTSOSharedUI, Log, All);

/** @brief Tile view object type for loading conversation/choice data */
UCLASS(Blueprintable)
class URTSOStructWrapper : public UObject
{
	GENERATED_BODY()
public:
	
	/** @brief Self-explanatory */
	void AssignData(
		const FText& InSelectionEntry,
		const int32 InChoiceIndex,
		UUserWidget* InDirectParentReference)
	{
		SelectionEntry = InSelectionEntry;
		ChoiceIndex = InChoiceIndex;
		DirectParentReference = InDirectParentReference;
	}

	/** @brief Access to private SelectionEntry value */
	UFUNCTION(BlueprintCallable)
	const FText& GetSelectionEntry() const { return SelectionEntry; };
	
	/** @brief Access to private ChoiceIndex value */
	UFUNCTION(BlueprintCallable)
	int32 GetChoiceIndex() const { return ChoiceIndex; };

	/** @brief Access to private DirectParentReference value */
	UFUNCTION(BlueprintCallable)
	UUserWidget* GetDirectParentReference() const { return DirectParentReference; };

private:
	/** @brief Assigned by 'AssignData', retrieved by 'GetSelectionEntry'  */
	UPROPERTY(BlueprintReadWrite, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	FText SelectionEntry{};
	
	/** @brief Assigned by 'AssignData', retrieved by 'GetChoiceIndex'  */
	UPROPERTY(BlueprintReadWrite, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	int32 ChoiceIndex = INDEX_NONE;

	/** @brief Assigned by 'AssignData', retrieved by 'DirectParentReference'  */
	UPROPERTY(BlueprintReadWrite, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	UUserWidget* DirectParentReference = nullptr;
};

/** @brief Generic modular tile widget */
UCLASS(Abstract)
class URTSOModularTile : public UUserWidget
{
	GENERATED_BODY()

public:
	/** @brief Calls Refresh() */
	virtual void NativePreConstruct() override;

	/** @brief Updates width and height override and set 'TextName' to display 'TileText' */
	UFUNCTION(BlueprintCallable)
	virtual void Refresh();
	
	/** @brief Size box which will enforce a given size */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class USizeBox* SizeBoxContainer = nullptr;
	
	/** @brief UNamedSlot which will be an body of sorts, to the modular tile. */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UNamedSlot* NamedSlot = nullptr;

	/** @brief 'Shadow' in form of an image, for 'NamedSlot' */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UImage* ImageShadow = nullptr;
	
	/** @brief Text block widget for the tile name / title */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UTextBlock* TextName = nullptr;

	/** @brief The actual text that will be supplied to the 'TextName' textblock */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText TileText{};

	/** @brief Override slot height */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Height{0};		

	/** @brief Override slot width */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Width{0};
};

/** @brief Conversation selection (choice) widgets, meant to be used in a listview or tileview */
UCLASS(Abstract)
class URTSOConversationSelectionEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
protected:
	
	/* IUserObjectListEntry Start */
	/** @brief Sets entry data and binds delegates to our textborder */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	/* IUserObjectListEntry End */

	/** @brief Calls into 'ParentAsMessageWidget->MouseMove' and returns its event reply results */
	UFUNCTION() FEventReply MouseMove(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	/** @brief Calls into 'ParentAsMessageWidget->MouseButtonDown' and returns its event reply results */
	UFUNCTION() FEventReply MouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	/** @brief Calls into 'ParentAsMessageWidget->MouseButtonUp' and returns its event reply results */
	UFUNCTION() FEventReply MouseButtonUp(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	/** @brief Calls into 'ParentAsMessageWidget->MouseDoubleClick' and returns its event reply results */
	UFUNCTION() FEventReply MouseDoubleClick(FGeometry MyGeometry, const FPointerEvent& MouseEvent);	

	/** @brief Modular tile to contain and control the other elements   */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOModularTile* Tile = nullptr;

	/** @brief Border for the 'TextContent' text-block widget.*/
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UBorder* TextContentBorder = nullptr;	
	
	/** @brief 'TextContent' text-block widget. Will display the choice message */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UCommonTextBlock* TextContent = nullptr;

	/** @brief 'ChoiceIndex' integer. Will be supplied to 'Tile->TileText' text-block */
	UPROPERTY(BlueprintReadWrite, Meta = (ExposeOnSpawn=true))
	int32 ChoiceIndex = INDEX_NONE;

	/** @brief Reference to the direct parent, meaning the userwidget this is contained within and not necessarily the direct parent in the widget tree */
	UPROPERTY(BlueprintReadWrite, Meta = (ExposeOnSpawn=true))
	UUserWidget* DirectParentReference = nullptr;	
};

/** @brief Conversation message element widget */
UCLASS(BlueprintType, Blueprintable)
class URTSOConversationMessageWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	
	/** @brief Returns an event reply @return Always 'Handled' event reply */
	UFUNCTION(BlueprintNativeEvent) FEventReply MouseMove(int32 ChoiceIdx, FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	/** @brief Stores 'ChoiceIdx' into 'LatestInteractedChoice' @return Always 'Handled' event reply */
	UFUNCTION(BlueprintNativeEvent) FEventReply MouseButtonDown(int32 ChoiceIdx, FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	/** @brief Compares 'ChoiceIdx' with 'LatestInteractedChoice', then calls SelectChoice() if possible @return 'Unhandled' if comparison not matched, otherwise 'Handled' event reply */
	UFUNCTION(BlueprintNativeEvent) FEventReply MouseButtonUp(int32 ChoiceIdx, FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	/** @brief Returns an event reply @return Always 'Handled' event reply */
	UFUNCTION(BlueprintNativeEvent) FEventReply MouseDoubleClick(int32 ChoiceIdx, FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	
	/** @defgroup PayloadDescr @brief Creates a URTSOStructWrapper to pass along to the tileview, sets any necessary data before passing into the tileview */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SetPayload(const FClientConversationMessagePayload& Payload, AActor* PotentialCallbackActor);
	virtual void SetPayload_Implementation(const FClientConversationMessagePayload& Payload, AActor* PotentialCallbackActor); /**< @ingroup PayloadDescr */


	/** @defgroup ChoiceSelectDescr
	 * @brief Calls IRTSOConversationSpeakerInterface::Execute_ReplyChoice, if CurrentPotentialCallbackActor implements said interface */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SelectChoice(int32 ChoiceSelection);
	virtual void SelectChoice_Implementation(int32 ChoiceSelection); /**< @ingroup ChoiceSelectDescr */

	/** @brief Reserved for later use */
	virtual void NativeDestruct() override;
	
	/** @brief Tileview which will display our 'URTSOConversationSelectionEntry's */
	UPROPERTY(BlueprintReadWrite, Meta = (BindWidget))
	class UTileView* ConversationSelectors = nullptr;

	/** @brief Current potential callback actor, which is assumed to implement 'IRTSOConversationSpeakerInterface' */
	UPROPERTY(BlueprintReadOnly)
	AActor* CurrentPotentialCallbackActor;

	/** @brief Current list of instantiated entry objects */
	UPROPERTY(BlueprintReadWrite)
	TArray<URTSOStructWrapper*> InstantiatedEntryObjects{};

	/** @brief Last chosen conversation reply index */
	int32 LatestInteractedChoice = INDEX_NONE;
};

/** @brief Conversation base widget */
UCLASS(BlueprintType, Blueprintable)
class URTSOConversationWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	/** @defgroup BasePayloadDescr @brief Calls into 'ConversationMessageWidget->SetPayload' */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SetPayload(const FClientConversationMessagePayload& Payload, AActor* PotentialCallbackActor);
	virtual void SetPayload_Implementation(const FClientConversationMessagePayload& Payload, AActor* PotentialCallbackActor); /**< @ingroup BasePayloadDescr */

	/** @defgroup BaseChoiceSelectDescr @brief Calls into 'ConversationMessageWidget->SelectChoice' */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SelectChoice(int32 ChoiceSelection);
	virtual void SelectChoice_Implementation(int32 ChoiceSelection); /**< @ingroup BaseChoiceSelectDescr */
	
	/** @brief Actual message widget for the choice/reply messages */
	UPROPERTY(BlueprintReadWrite, Meta = (BindWidget))
	URTSOConversationMessageWidget* ConversationMessageWidget = nullptr;

	/** @brief Actual modular tile for the speakers message */
	UPROPERTY(BlueprintReadWrite, Meta = (BindWidget))
	URTSOModularTile* Tile = nullptr;

	/** @brief Actual text content for the speakers message */
	UPROPERTY(BlueprintReadWrite, Meta = (BindWidget))
	UCommonTextBlock* TextContent = nullptr;

	/** @brief Button which binds to logic that ends the conversation */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class URTSOMenuButton* ExitConversationButton = nullptr;
};


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
