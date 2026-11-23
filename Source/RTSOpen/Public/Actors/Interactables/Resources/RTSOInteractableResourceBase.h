/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include "CoreMinimal.h"
#include "PDInventorySubsystem.h"
#include "PDRTSSharedHashGrid.h"
#include "Actors/PDInteractActor.h"
#include "AI/Mass/RTSOMassFragments.h"
#include "Interfaces/RTSOConversationInterface.h"
#include "RTSOInteractableResourceBase.generated.h"

USTRUCT(Blueprintable)
struct FRTSOResourceBehaviourSettings : public FTableRowBase
{
	GENERATED_BODY()

	/** @brief Availability settings, does it have infinite or limited amounts? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERTSResourceAvailability OverrideAvailability = PD::Interactable::Behaviour::Availability::EUndefined;
	/** @brief Requirement settings, do we always allow trade or do we need to have the minimum amount available before allowing a trade? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERTSResourceRequirement OverrideRequirements = PD::Interactable::Behaviour::Requirements::EUndefined;	
};

class UMassEntitySubsystem;
/**
 * @brief An actor with a job tag tied to it along with rewards and potential mission progress, meant to be used for interactable resources/items
 */
UCLASS()
class RTSOPEN_API ARTSOInteractableResourceBase
	: public APDInteractActor
	, public IRTSOMissionProgressor
{
	GENERATED_BODY()

public:
	/** @brief Sets default job to 'TAG_AI_Job_WalkToTarget' and enables the tickcomponent*/ 
	ARTSOInteractableResourceBase();
	/** @brief Caches a pointer to the entity subsystem and it's entity manager*/
	virtual void BeginPlay() override;
	/** @brief Ticks usage the cooldown. @todo move into a progression/stat system*/
	virtual void Tick(float DeltaTime) override;

	/** @brief Untracks the actor from the resource tracker */
	virtual void BeginDestroy() override;
	
	/** @brief Overridden but solely calls super. Reserved for later use */
	virtual void AddTagToCaller_Implementation(AActor* Caller, const FGameplayTag& NewTag) override;
	/** @brief Overridden but solely calls super. Reserved for later use */
	virtual FGameplayTagContainer SelectorTagToTagContainer_Implementation(AActor* Caller, const FGameplayTag& SelectorTag) override;
	void OnInteractionSuccessful(AActor* InstigatorActor) const;

	void ProcessTradeIfInfiniteInventory(
		const FPDInteractionParamsWithCustomHandling& InteractionParams,
		EPDInteractResult& InteractResult,
		UPDInventoryComponent* InstigatorInvComponent,
		FRTSOLightInventoryFragment* InstigatorInventoryFragment,
		UPDInventorySubsystem* InvSubsystem) const;

	void ProcessTradeIfLimitedInventory(
		const FPDInteractionParamsWithCustomHandling& InteractionParams,
		EPDInteractResult& InteractResult,
		UPDInventoryComponent* InstigatorInvComponent,
		FRTSOLightInventoryFragment* InstigatorInventoryFragment,
		UPDInventorySubsystem* InvSubsystem,
		bool& bMustWaitForRegeneration) const;

	FORCEINLINE const FRTSOLightInventoryFragment& GetInventoryFragment() const
	{
		return InventoryFragment;
	}

	
	
	/** @brief Handles interaction with the calling (mass) entity or calling actor.
	 * @details Gives the callers inventory component or inventory fragment the items listed in 'LinkedItemResources' */
	virtual void OnInteract_Implementation(const FPDInteractionParamsWithCustomHandling& InteractionParams, EPDInteractResult& InteractResult) const override;

	/** @brief Base class just returns the job-tag. could be extended to return other tags if needed */
	virtual FGameplayTagContainer GetGenericTagContainer_Implementation() const override;

	/** @brief Base class just returns the harvest-time. could be extended to return modified or different values if needed */	
	virtual double GetInteractionTime_Implementation() const override { return GetInteractionSettings().InteractionTimeInSeconds; };
	
	/** @brief Checks if we have infinite inventory, via the override settings or falls back to global settings */
	UFUNCTION()
	bool HasInfiniteInventory() const;

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif // WITH_EDITOR

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (RequiredAssetDataTags="RowStructure=/Script/RTSOpen.RTSOMissionProgressionTagSets"))
	TMap<FGameplayTag, FDataTableRowHandle> OnSuccessfulInteraction_GrantedProgressionTagSets;
	TMap<FGameplayTag, FRTSOMissionProgressionTagSets> ProgressionTagSetsToGrant{};

private:
	/** @brief @note Anything above index 0 is reserved and not used as of yet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	TArray<FGameplayTagContainer> MissionProgressionSelectorTags{};
	
	/** @brief What job is this resource tied to, default value is set to TAG_AI_Job_GatherResource in this classes ctor */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	FGameplayTag JobTag{};
	
	/** @brief Tick accumulator */
	double RefreshTickAcc = 0.0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess="true", RowType="/Script/RTSOpen.RTSOResourceBehaviourSettings"))
	FDataTableRowHandle ResourceBehaviourSettings;
	FRTSOResourceBehaviourSettings CachedBehaviourSettings;
	
	/** @brief The items archetype that defines an interaction with this resource. */	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess="true", RowType="/Script/PDInventory.PDItemDefaultDatum"))
	TMap<FGameplayTag /*resource/item tag*/, int32 /*count*/> TradeArchetype;

	/** @brief The items contained by this resource.
	 * Base settings that defines our trading behaviour can be found in 'URTSInteractableResourceSettings' and
	 * may be overriden via the Override members of this class. */		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess="true"))
	FRTSOLightInventoryFragment InventoryFragment;

	/** @brief The mass entity subsystem, used to fetch the entity manager*/
	UPROPERTY()
	UMassEntitySubsystem* EntitySubsystem = nullptr;
	/** @brief The mass entity manager, used to read and write to different fragments on requested entities */
	const FMassEntityManager* EntManager = nullptr;

	/** @brief @todo This is not being incremented anywhere, no rules how to handle it. Must resolve this */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess="true"))
	int32 TagOrder = 0;

	/** @brief */
	UPROPERTY()
	FPDGridCell PreviousGridCell;
	
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