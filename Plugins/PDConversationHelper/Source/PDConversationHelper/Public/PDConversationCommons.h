/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "GameFeaturesSubsystem.h"
#include "ConversationInstance.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "PDConversationCommons.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCompleted_ActivateFeature);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBeginWaitingForChoicesDlgt, UObject*, InterfaceObject,  int32, ActorID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBeginWaitingForChoicesDlgt, int32, ActorID);

UENUM(Blueprintable, BlueprintType)
enum class EPDKeyProcessResult : uint8 
{
	KEY_SUCCESS UMETA(DisplayName="Valid or whitelisted Key"), 
	KEY_FAIL    UMETA(DisplayName="Invalid or blacklisted Key"),
};

/**
 * @brief This instance adds some flags we want to use alongside with an overridden function,
 * so the system works as intended in single-player and in co-op/multiplayer alongside with a map of owner/actor IDs
 * and their waiting-state
 */
UCLASS(BlueprintType)
class PDCONVERSATIONHELPER_API UPDConversationInstance : public UConversationInstance
{
	GENERATED_BODY()

public:
	/** @brief Empty, reserved for further use */
	UFUNCTION(BlueprintCallable)
	virtual void PauseConversationAndSendClientChoices(const FConversationContext& Context, const FClientConversationMessage& ClientMessage) override;
	
	/** @brief Delegate to send a request for the conversation to advance/resume to the given actor/owner ID */
	UPROPERTY()
	TMap<int32 /*ActorID*/, FBeginWaitingForChoicesDlgt> OnBegin_WaitChoicesDelegateMap;
	
	/** @brief Map of Actor/Owner IDs and their waiting state*/
	UPROPERTY(BlueprintReadWrite)
	TMap<int32 /*ActorID*/, bool> ChoiceWaitingStateMap;
};

/** @brief This class holds some helper and possibly debug functions we might want to use */
UCLASS()
class PDCONVERSATIONHELPER_API UPDConversationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** @brief Gets the previous/latest message that was sent */
	UFUNCTION(BlueprintCallable)
	static const FClientConversationMessagePayload& GetPreviousMessage(UConversationParticipantComponent* ConversationParticipantComponent);
	
	/** @brief Checks with teh game feature subsystem if the given plugin has been loaded and activated */
	UFUNCTION(BlueprintCallable)
	static bool IsGameFeaturePluginActive(FString PluginName);
	
	/** @brief Prints the message to screen */
	UFUNCTION(BlueprintCallable)
	static void Debug_PrintConversationMessageToScreen(UObject* WorldContext, const FClientConversationMessage& Message, FLinearColor MessageColour);

	/** @brief Prints the message to screen, alternative input parameters */
	UFUNCTION(BlueprintCallable)
	static void Debug_PrintConversationTextToScreen(UObject* WorldContext, const FName& Participant, const FText& Text, FLinearColor MessageColour);		
};


/** @brief This class is an async latent action based class, deriving from UBlueprintAsyncActionBase.
 * We dispatch a call from at and it works much like a delay in the graph, as in it resumes the execution from the correct node when a reply has been received.  */
UCLASS()
class PDCONVERSATIONHELPER_API UPDAsyncAction_ActivateFeature : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPDAsyncAction_ActivateFeature(const FObjectInitializer& ObjectInitializer);

	/** @brief Called when the action activates */
	virtual void Activate() override;
	
	/** @brief Called when the action completes */
	void Completed(const UE::GameFeatures::FResult& Result);
	
	/** @brief Static helper to create an action instance of this object */
	UFUNCTION(BlueprintCallable, Meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true" ))
	static UPDAsyncAction_ActivateFeature* CreateActionInstance(UObject* WorldContextObject, FString GameFeatureName);
public:
	/** @brief Cached name to the game feature data plugin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn))
	FString GameFeatureDataPluginName;
	
	/** @brief Delegate to (possibly) fire at end of the action*/
	UPROPERTY(BlueprintAssignable)
	FCompleted_ActivateFeature Succeeded;

	/** @brief Delegate to (possibly) fire at end of the action*/
	UPROPERTY(BlueprintAssignable)
	FCompleted_ActivateFeature Failed;
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