/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include "CoreMinimal.h"
#include "PDBuildCommon.h"
#include "PDRTSCommon.h"
#include "Actors/PDInteractActor.h"
#include "AI/Mass/RTSOMassFragments.h"
#include "Components/PDInventoryComponent.h"
#include "Interfaces/PDRTSBuildableGhostInterface.h"
#include "Interfaces/RTSOConversationInterface.h"
#include "RTSOInteractableBuildingBase.generated.h"

struct FRTSOLightInventoryFragment;

/** @brief Buildable light inv. construct. Keeps inventories for each build state and one for the finished state  */
USTRUCT(Blueprintable)
struct FRTSOBuildableInventories
{
	GENERATED_BODY()

	/** @brief Ghost, per-stage inventories  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FRTSOLightInventoryFragment> LightInventoriesPerGhostStage{};

	/** @brief Main inventory  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRTSOLightInventoryFragment StorageInventory{};	
};


/**
 * @brief An actor with a job tag tied to it, meant to be used for interactable buildings
 * @todo declare and define the OnInteract interface function
 */
UCLASS()
class RTSOPEN_API ARTSOInteractableBuildingBase
	: public APDInteractActor
	, public IPDRTSBuildableGhostInterface
	, public IRTSOMissionProgressor
{
	GENERATED_BODY()

public:
	/** @brief Sets default job to 'TAG_AI_Job_WalkToTarget' and enables the tickcomponent*/ 
	ARTSOInteractableBuildingBase();
	
	/** @brief Template base call  */
	template<bool TIsGhost>
	void RefreshStaleSettings(){};
	
	/** @brief Only calls Super. Reserved for later use  */
	virtual void Tick(float DeltaTime) override;
	/** @brief Only calls Super. Reserved for later use  */
	virtual void BeginPlay() override;
	/** @brief  Calls 'OnBuildingDestroyed' then calls Super::BeginDestroy */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** @brief Overridden but solely calls super. Reserved for later use */
	virtual void AddTagToCaller_Implementation(AActor* Caller, const FGameplayTag& NewTag) override;
	/** @brief Overridden but solely calls super. Reserved for later use */
	virtual FGameplayTagContainer SelectorTagToTagContainer_Implementation(AActor* Caller, const FGameplayTag& SelectorTag) override;
	void OnBuildSuccessful(AActor* InstigatorActor) const;
	
	
	/** @brief adds 'JobTag' to a 'FGameplayTagContainer' and returns it */
	virtual FGameplayTagContainer GetGenericTagContainer_Implementation() const override;

	virtual FGameplayTag GetInstigatorTag_Implementation() const override;
	
	/* APDInteractActor Interface Start */
	virtual bool GetCanInteract_Implementation() const override;
	virtual void OnInteract_Implementation(const FPDInteractionParamsWithCustomHandling& InteractionParams, EPDInteractResult& InteractResult) const override;
	/* APDInteractActor Interface End */
	
	/** @brief  Processes a spawn, load settings (and set state) based on being a ghost or not */
	template<bool TIsGhost>
	void ProcessSpawn();
	
	/** @brief If able, withdraws the recurring cost.
	 * @param Bank (Optional player inv)
	 * @param EntityInv (Optional calling entity)
	 * @param ImmutableStage (Current stage of buildable) */
	bool WithdrawRecurringCostFromBankOrEntity(UPDInventoryComponent* Bank, FRTSOLightInventoryFragment* EntityInv, const int32& ImmutableStage);

	/** @brief If able, withdraws everything in the inventory from the bank or entity.
	 * @param Bank (Optional player inv)
	 * @param EntityInv (Optional calling entity) */
	FString WithdrawAllPossibleFromBankOrEntity(UPDInventoryComponent* Bank, FRTSOLightInventoryFragment* EntityInv, bool bLogAction = false);
	/** @brief If able, deposits everything in the inventory to the bank or entity.
	 * @param Bank (Optional player inv)
	 * @param EntityInv (Optional calling entity) */
	FString DepositAllPossibleToBankOrEntity(UPDInventoryComponent* Bank, FRTSOLightInventoryFragment* EntityInv, bool bLogAction = false);

	/** @brief Handles processing if we require workers to build, requires resource per stages */
	void ProcessIfWorkersRequired();
	/** @brief Handles processing if we don't require workers to build. Fast-forwards through stages */
	void ProcessIfWorkersNotRequired();
	
	/** @brief Calls into the RTSSubsystem adn removes the buildable from the tracking octree */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "")
	void OnBuildingDestroyed();
	
	/* IPDRTSBuildableGhostInterface Interface Start */
	
	/** @brief Call when we want to get a ghosts meshes. Used for encroachment/boundary checks */
	virtual TArray<UStaticMeshComponent*> GetGhostMeshes_Implementation() override;
	virtual void SetGhostAsEncroached_Implementation(bool bIsEncroached) override;
	virtual bool GetGhostAsEncroached_Implementation() override;
	virtual void OnSpawnedAsGhost_Implementation(const FGameplayTag& BuildableTag, const FGameplayTag& BuildContextTag, bool bInIsPreviewGhost, bool bInRequiresWorkersToBuild) override;
	virtual void OnSpawnedAsMain_Implementation(const FGameplayTag& BuildableTag, const FGameplayTag& BuildContextTag)  override;
	virtual void TransitionFromGhostToMain_Implementation() override;
	void Internal_ProgressGhostStage(const bool bForceProgressThroughStage, const bool bChainAll);
	virtual void ProgressGhostStage_Implementation(const bool bChainAll) override;
	virtual bool AttemptFinalizeGhost_Implementation() override;
	/* IPDRTSBuildableGhostInterface Interface End */
	
	/** @brief Returns tbe buildings own light inventory constructs. One for finished stage and one for the ghosts each build stage  */
	virtual FRTSOBuildableInventories& ReturnBuildableInventories();
	virtual FRTSOLightInventoryFragment& GetCurrentInventory();
	FRTSOLightInventoryFragment CalculateFreeInventorySpace();

private:
	/** @brief Refreshes ghosts settings, if they have been marked as stale */
	void RefreshStaleSettings_Ghost();
	/** @brief Refreshes main buildable settings, if they have been marked as stale */
	void RefreshStaleSettings_Main();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (RequiredAssetDataTags="RowStructure=/Script/RTSOpen.RTSOMissionProgressionTagSets"))
	TArray<FDataTableRowHandle> PerGhostStage_ProgressionTagSetsHandle;
	TArray<FRTSOMissionProgressionTagSets> PerGhostStage_ProgressionTags{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (RowType="/Script/RTSOpen.RTSOMissionProgressionTagSets"))
	FDataTableRowHandle OnSuccessfulBuild_ProgressionTagSetsHandle;
	FRTSOMissionProgressionTagSets OnSuccessfulBuild_ProgressionTags;

	/** @brief Collision settings for complete buildable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDRTSBuildableCollisionSettings BuildableCollisionSettings{false};
	/** @brief Collision settings for ghost buildable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDRTSBuildableCollisionSettings GhostCollisionSettings{false};

	/** @brief Finished progressing current state */
	bool CurrentStateFinishedProgressing = false;
	/** @brief Active progress current state  */
	bool RunningStateProgressFunction = true;

	/** @brief Handle to default inventory, relevant for all builings that has storage limits. If not set then the building can hold infinite */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (RowType="/Script/RTSOpen.RTSOInventoryDefaultRow"))
	FDataTableRowHandle TargetInventoryLimit{};

private:
	/** @brief light inventory constructs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	FRTSOBuildableInventories BuildableInventories{};
	
	/** @brief JobTag associated with this actor */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	FGameplayTag JobTag{};

	/** @brief Main material, gets set from data-asset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	UMaterialInstance* MainMat = nullptr;

	/** @brief Ghost material, gets set from data-asset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	UMaterialInstance* GhostMat = nullptr;

	/** @brief Ghost material, gets set from data-asset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	UMaterialInstance* GhostMat_Encroached = nullptr;	

	/** @brief What buildable tag spawned us, gets set when spawned */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	FGameplayTag InstigatorBuildableTag{};

	/** @brief The building context from which we were spawned */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	FGameplayTag SpawnBuildContextTag{};

	/** @return true if the building a preview ghost, false if it is a placed ghost or not a ghost at all */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	bool bIsPreviewGhost = true; // Default to preview
	
	/** @return true if the ghost is encroaching another building */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	bool bIsEncroachedGhost = false; // Default to false
	
	/** @return Static name used as selector in BuildableCollisionSettings and GhostCollisionSettings */
	static inline FName BoxcompName = "Boxcomp"; 
	/** @return Static name used as selector in BuildableCollisionSettings and GhostCollisionSettings */
	static inline FName MeshName = "Mesh";

	bool bHasInitializedInv = false;
	FRTSOInventoryDefaultRow InventoryLimit{};	
};

/** @brief RefreshStaleSettings<true>(), Calls RefreshStaleSettings_Ghost */
template<>
void inline ARTSOInteractableBuildingBase::RefreshStaleSettings<true>()
{
	RefreshStaleSettings_Ghost();
};
/** @brief RefreshStaleSettings<false>(), Calls RefreshStaleSettings_Main */
template<>
void inline ARTSOInteractableBuildingBase::RefreshStaleSettings<false>()
{
	RefreshStaleSettings_Main();
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
