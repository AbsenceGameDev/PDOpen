/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RTSOActionLogInterface.h"
#include "Interfaces/PDRTSBuilderInterface.h"
#include "UObject/Interface.h"
#include "Widgets/Slate/SRTSOActionLog.h"
#include "RTSOConversationInterface.generated.h"

struct FGameplayTag;
/** @brief BOILERPLATE */
UINTERFACE()
class URTSOConversationSpeakerInterface : public UInterface { GENERATED_BODY() };

/** @brief Conversation Interface, apply to speaker actors (mission giver for example) */
class RTSOPEN_API IRTSOConversationSpeakerInterface
{
	GENERATED_BODY()

public:
	/** @brief Speaker interface events - When waiting for a client reply */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void BeginWaitingForChoices(int32 ActorID);

	/** @brief Speaker interface events - On a client reply */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void ReplyChoice(AActor* Caller, int32 Choice);
};

/** @brief BOILERPLATE */
UINTERFACE() class URTSOConversationInterface : public UInterface { GENERATED_BODY() };

/** @brief Conversation Interface, apply to listener actors (player for example) */
class RTSOPEN_API IRTSOConversationInterface
{
	GENERATED_BODY()

public:
	/** @defgroup ConversationInterface_AddProgressTag
	 * @brief Reserved for later use. Adds the given tag to the list of acquired conversation tags
	 * @todo source the progression tags from here and make use of these for the savegame data */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void AddUniqueProgressionTag(const FGameplayTag& NewTag);
	virtual void AddUniqueProgressionTag_Implementation(const FGameplayTag& NewTag)
	{
		AcquiredConversationProgressionTags.AddTag(NewTag);
	} /**< @ingroup ConversationInterface_AddProgressTag */

	/** @defgroup ConversationInterface_RemoveProgressTag
	 * @brief Reserved for later use. Removes the given tag from the list of acquired conversation tags
	 * @todo source the progression tags from here and make use of these for the savegame data */	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void RemoveProgressionTag(const FGameplayTag& TagToRemove);
	virtual void RemoveProgressionTag_Implementation(const FGameplayTag& TagToRemove)
	{
		if (AcquiredConversationProgressionTags.HasTag(TagToRemove))
		{
			AcquiredConversationProgressionTags.RemoveTag(TagToRemove);
		}
	} /**< @ingroup ConversationInterface_RemoveProgressTag */


	/** @defgroup ConversationInterface_AddProgressTagSet
	 * @brief Reserved for later use. Adds the given tag set to the list of acquired conversation tags
	 * @todo source the progression tags from here and make use of these for the savegame data */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void AddUniqueProgressionTagSet(const TSet<FGameplayTag>& NewTags);
	virtual void AddUniqueProgressionTagSet_Implementation(const TSet<FGameplayTag>& NewTags)
	{
		FGameplayTagContainer NewContainer{};
		for(FGameplayTag NewTag : NewTags)
		{
			NewContainer.AddTag(NewTag);
		}
		
		AcquiredConversationProgressionTags.AppendTags(NewContainer);
	}

	/** @defgroup ConversationInterface_RemoveProgressTagSet
	 * @brief Reserved for later use. Removes the given tag set from the list of acquired conversation tags
	 * @todo source the progression tags from here and make use of these for the savegame data */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void RemoveProgressionTagSet(const TSet<FGameplayTag>& TagsToRemove);
	virtual void RemoveProgressionTagSet_Implementation(const TSet<FGameplayTag>& TagsToRemove)
	{
		for (const FGameplayTag& Tag : TagsToRemove)
		{
			AcquiredConversationProgressionTags.RemoveTag(Tag);
		}
	}

	/** @defgroup ConversationInterface_AddProgressTagContainer
	 * @brief Reserved for later use. Adds the given tag set to the list of acquired conversation tags
	 * @todo source the progression tags from here and make use of these for the savegame data */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void AddProgressionTagContainer(const FGameplayTagContainer& NewTags);
	virtual void AddProgressionTagContainer_Implementation(const FGameplayTagContainer& NewTags)
	{
		AcquiredConversationProgressionTags.AppendTags(NewTags);
	}

	
	/** @defgroup ConversationInterface_RemoveProgressTagSet
	 * @brief Reserved for later use. Removes the given tag set from the list of acquired conversation tags
	 * @todo source the progression tags from here and make use of these for the savegame data */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void RemoveProgressionTagContainer(const FGameplayTagContainer& TagsToRemove);
	virtual void RemoveProgressionTagContainer_Implementation(const FGameplayTagContainer& TagsToRemove)
	{
		AcquiredConversationProgressionTags.RemoveTags(TagsToRemove);
	}
	
	/** @brief Returns the list of acquired conversation progression tags */
	virtual const FGameplayTagContainer& GetProgressionTags()
	{
		return AcquiredConversationProgressionTags;
	}

	/** @defgroup ConversationInterface_HasProgressionTag
	 * @brief Reserved for later use. Checks if the map of acquired conversation tags contains the 'CompareTag'
	 * @todo source the progression tags from here and make use of these for the savegame data */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool HasProgressionTag(const FGameplayTag& CompareTag);
	virtual bool HasProgressionTag_Implementation(const FGameplayTag& CompareTag)
	{
		return AcquiredConversationProgressionTags.HasTag(CompareTag);
	}
	
public:
	/** @brief Data interface, actual data is hidden completely form the engine but accessible in code
	 * - indirectly accessible in engine via the interface functions base implementation*/
	FGameplayTagContainer AcquiredConversationProgressionTags{};
};


USTRUCT(Blueprintable)
struct FRTSOMissionProgressionTagSets : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ProgressionTagSet;
	
};

/** @brief BOILERPLATE */
UINTERFACE() class URTSOMissionProgressor : public UInterface { GENERATED_BODY() };

/** @brief Mission Interface, apply to any actor that will create mission progress  */
class RTSOPEN_API IRTSOMissionProgressor
{
	GENERATED_BODY()

	static void SendToActionLog(AActor* Caller, FGameplayTagContainer SelectedTags)
	{
		if (Caller->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()))
		{
			FString BuildString;
			for (const FGameplayTag& Tag : SelectedTags)
			{
				BuildString += Tag.GetTagName().ToString() + ", "; // Trailing comma, who cares, faster to jsut leave it here 
			}

			const FRTSOActionLogEvent NewActionEvent{
				FString::Printf(TEXT(
					"OwnerID(%s) -- Received Mission Progress Tags: {%s} "),
					*Caller->GetName(), *BuildString)}; 
			URTSActionLogSubsystem::DispatchEvent(IPDRTSBuilderInterface::Execute_GetBuilderID(Caller), NewActionEvent);		
		}		
	}
	
public:
	/** @defgroup MissionProgressorInterface_AddTagToCaller
	 * @brief Reserved for later use. Adds the given tag to the list of acquired conversation tags
	 * @todo source the progression tags from here and make use of these for the savegame data */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void AddTagToCaller(AActor* Caller, const FGameplayTag& NewTag);
	virtual void AddTagToCaller_Implementation(AActor* Caller, const FGameplayTag& NewTag)
	{
		if (Caller == nullptr
			|| Caller->GetClass()->ImplementsInterface(URTSOConversationInterface::StaticClass()) == false)
		{
			return;
		}
		
		IRTSOConversationInterface* CallerInterface = Cast<IRTSOConversationInterface>(Caller);
		CallerInterface->AddUniqueProgressionTag(NewTag);

		SendToActionLog(Caller, FGameplayTagContainer{NewTag});
	} /**< @ingroup MissionProgressorInterface_AddTagToCaller */

	/** @defgroup MissionProgressorInterface_AddTagToCaller
	 * @brief Reserved for later use. Adds the given tag to the list of acquired conversation tags
	 * @todo source the progression tags from here and make use of these for the savegame data */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void AddTagContainerToCallerFromSelectorTag(AActor* Caller, const FGameplayTag& SelectorTag);
	virtual void AddTagContainerToCallerFromSelectorTag_Implementation(AActor* Caller, const FGameplayTag& SelectorTag)
	{
		if (Caller == nullptr
			|| Caller->GetClass()->ImplementsInterface(URTSOConversationInterface::StaticClass()) == false)
		{
			return;
		}
		const FGameplayTagContainer SelectedTags = SelectorTagToTagContainer_Implementation(Caller, SelectorTag);
		
		IRTSOConversationInterface* CallerInterface = Cast<IRTSOConversationInterface>(Caller);		
		CallerInterface->AddProgressionTagContainer_Implementation(SelectedTags);

		SendToActionLog(Caller, SelectedTags);
	} /**< @ingroup MissionProgressorInterface_AddTagToCaller */
	
	/** @defgroup MissionProgressorInterface_SelectorTagToTagContainer
	 * @brief Reserved for later use. Checks if the map of acquired conversation tags contains the 'CompareTag'
	 * @todo source the progression tags from here and make use of these for the savegame data */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FGameplayTagContainer SelectorTagToTagContainer(AActor* Caller, const FGameplayTag& SelectorTag);
	virtual FGameplayTagContainer SelectorTagToTagContainer_Implementation(AActor* Caller, const FGameplayTag& SelectorTag)
	{
		if (Caller == nullptr
			|| Caller->GetClass()->ImplementsInterface(URTSOConversationInterface::StaticClass()) == false)
		{
			return FGameplayTagContainer{};
		}

		IRTSOConversationInterface* CallerInterface = Cast<IRTSOConversationInterface>(Caller);
		return MissionProgressionTagsToGive.FindOrAdd(SelectorTag).ProgressionTagSet;
	} /**< @ingroup MissionProgressorInterface_SelectorTagToTagContainer */
	
public:
	/** @brief Data interface, actual data is hidden completely form the engine but accessible in code
	 * - indirectly accessible in engine via the interface functions base implementation*/
	TMap<FGameplayTag, FRTSOMissionProgressionTagSets> MissionProgressionTagsToGive{};
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