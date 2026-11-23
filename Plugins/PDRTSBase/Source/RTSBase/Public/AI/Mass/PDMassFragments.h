/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include "CoreMinimal.h"
#include "AI/PDMassCommon.h"

#include "MassCommonTypes.h"
#include "MassEntityManager.h"
#include "MassEntityTypes.h"
#include "PDBuildCommon.h"
#include "PDRTSCommon.h"
#include "SmartObjectTypes.h"

#include "PDMassFragments.generated.h"

class UNavigationPath;
class UAnimToTextureDataAsset;

// MassTags
/** @brief MassTag: RTSEntityTag */
USTRUCT() struct PDRTSBASE_API FPDMTag_RTSEntity : public FMassTag { GENERATED_BODY(); };

/** @brief MassFragment: SimpleMovementFragment */
USTRUCT()
struct FPDMFragment_SimpleMovement : public FMassFragment
{
	GENERATED_BODY();

	/** @brief Static location target */
	UPROPERTY(EditAnywhere)
	FVector Target;
};

/** @brief Entity Selection State :: Enum class : uint8 */
UENUM()
enum class EPDEntitySelectionState : uint8
{
	ENTITY_SELECTED = 0    UMETA(DisplayName="Entity Selected"),
	ENTITY_NOTSELECTED = 1 UMETA(DisplayName="Entity Not Selected"),
	ENTITY_UNSET = 2       UMETA(DisplayName="Entity Unset"), 
};

/** @brief Base fragment for RTS Agents : Update, sorted their values to ensure as little padding as possible */
USTRUCT(BlueprintType)
struct PDRTSBASE_API FPDMFragment_RTSEntityBase : public FMassFragment
{
	GENERATED_BODY()

	/** @todo Is the entity performing an action or not? */
	UPROPERTY()
	bool bAction  = false;
	
	/** @brief Selection clearing state for an entity running this fragment */
	UPROPERTY()
	bool bHasClearedSelection = true;
	
	/** @brief Selection state for an entity running this fragment */
	UPROPERTY()
	EPDEntitySelectionState SelectionState = EPDEntitySelectionState::ENTITY_UNSET;
	
	/** @brief Skin index for the static mesh vertex animation */
	UPROPERTY()
	float MeshSkinIdx = -1;

	/** @brief Selection Group Index for an entity running this fragment */
	UPROPERTY()
	int32 SelectionGroupIndex = INDEX_NONE;
	
	/** @brief OwnerID(Players) for an entity running this fragment */
	UPROPERTY()
	int32 OwnerID = INDEX_NONE;
	
	/** @brief Queued unit path actually at most contains a single entry and if set it
	 * runs a navpath calculation out-of-line with it's potential selection group */
	UPROPERTY()
	TArray<FVector> QueuedUnitPath{};	
	
	/** @brief The entities entity type, used by downstream systems  */
	UPROPERTY()
	FGameplayTag EntityType{TAG_AI_Type_BuilderUnit_Novice};	
	// /** @todo Do we have any queued entity-to-entity interactions */
	// UPROPERTY()
	// TArray<FMassEntityHandle> QueuedInteractables;
};

/** @brief Base fragment for entity ISM vertex animations */
USTRUCT()
struct PDRTSBASE_API FPDMFragment_EntityAnimation : public FMassFragment
{
	GENERATED_BODY()
	
	/** @brief Weak pointer to the A2T data */
	UPROPERTY(EditAnywhere)
	TWeakObjectPtr<UAnimToTextureDataAsset> A2TData;

	/** @brief Override animation set-up via task or other processor than default animation processor */
	UPROPERTY(EditAnywhere)
	uint8 bOverriddenAnimation : 1;
	
	/** @brief World start time for the animation */
	UPROPERTY(EditAnywhere)
	float InWorldStartTime = 0.0f;

	/** @brief Play-rate for the animation */
	UPROPERTY(EditAnywhere)
	float PlayRate = 1.0f;

	/** @brief Vertex-texture animation selector */
	UPROPERTY(EditAnywhere)
	EPDVertexAnimSelector AnimationStateIndex = EPDVertexAnimSelector::VertexAction;

	/** @brief Vertex-texture animation (start) position */
	UPROPERTY(EditAnywhere)
	int AnimPosition = 0;
};

/** @brief Target compound keeps track of the target, either a static location, a given actor and mass-entities*/
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDTargetCompound
{
	GENERATED_BODY()

	/** @brief Is valid check, does not take into regard if the entity actually exists */
	bool IsValidCompound() const
	{
		return
			ActionTargetAsActor != nullptr
			|| ActionTargetAsLocation.Get() != PD::Mass::InvalidLoc.Get()
			|| ActionTargetAsEntity != PD::Mass::InvalidHandle;
	};
	/** @brief Is valid check, takes into regard if the entity actually exists via a given entity manager */
	bool IsValidCompoundByManager(const FMassEntityManager& Manager) const
	{
		return
			ActionTargetAsActor != nullptr
			|| ActionTargetAsLocation.Get() != PD::Mass::InvalidLoc.Get()
			|| (ActionTargetAsEntity != PD::Mass::InvalidHandle && Manager.IsEntityValid(ActionTargetAsEntity));
	};
	
	/** @brief Target (if entity) */
	UPROPERTY(EditAnywhere)
	FMassEntityHandle ActionTargetAsEntity = FMassEntityHandle{0, 0};

	/** @brief Target (if static location) */
	UPROPERTY(EditAnywhere)
	FMassInt16Vector ActionTargetAsLocation;
	
	/** @brief Target (if actor) */
	UPROPERTY(EditAnywhere)
	AActor* ActionTargetAsActor = nullptr; // @todo Replace with Persistent Actor ID
};

/** @brief Shared fragment for when entities share animation data */
USTRUCT()
struct PDRTSBASE_API FPDWrappedSelectionGroupNavData
{
	GENERATED_BODY()

	/** @brief Data to be used in shared navpath fragment */
	UPROPERTY(EditAnywhere)
	TMap<int32 /*SelectionGroupIndex*/, const UNavigationPath*> SelectionGroupNavData;
};


/** @brief Shared fragment for when entities share navigation data */
USTRUCT()
struct PDRTSBASE_API FPDMFragment_SharedEntity : public FMassSharedFragment
{
	GENERATED_BODY()

	/** @brief Shared navpath fragment splitting up shared navdata between owner ID and their selection groups */
	UPROPERTY(EditAnywhere)
	TMap<int32 /* OwnerID */, FPDWrappedSelectionGroupNavData> SharedNavData;
};


/**
 * @brief Fragment given to entities to grant resources
 */
USTRUCT(BlueprintType)
struct PDRTSBASE_API FPDMFragment_Action : public FMassFragment
{
	GENERATED_BODY();

	/** @brief Action/Job tag */
	UPROPERTY(EditAnywhere)
	FGameplayTag ActionTag;

	/** @brief Optional targets to go with teh job-tag, in case teh job is associated with a given object or target */
	UPROPERTY(EditAnywhere)
	FPDTargetCompound OptTargets;
	
	/** @brief (Potential) Reward Type */
	UPROPERTY(EditAnywhere)
	FGameplayTag Reward;
	
	/** @brief (Potential) Reward Amount */
	UPROPERTY(EditAnywhere)
	int RewardAmount = 0;
};

/** @brief Shared fragment for when entities share animation data */
USTRUCT()
struct PDRTSBASE_API FPDMFragment_SharedAnimData : public FMassSharedFragment
{
	GENERATED_BODY()

	/** @brief Weak pointer to the (shared) A2T data */
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UAnimToTextureDataAsset> AnimData;
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