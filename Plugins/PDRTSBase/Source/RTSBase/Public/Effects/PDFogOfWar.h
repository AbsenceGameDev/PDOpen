/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "PDRTSSharedHashGrid.h"
#include "Serialization/BufferArchive.h"
#include "PDFogOfWar.generated.h"

class UTextureRenderTarget2D;

/** @brief What level of visibility does this area have? */
UENUM()
enum class EPDWorldVisibility : uint8
{
	EVisible,
	EObscured,
	ENotVisible,
};

/** @brief A simple struct which is acting the update states for the fog of war system */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDVisitedWorldDatum
{
	GENERATED_BODY()

	/** @brief The position for this update, we only care about X & Y */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDGridCell WorldPosition;

	/** @brief The level of visibility for this update */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite)
	EPDWorldVisibility WorldVisibility;	
	
};

/** @brief Key mapping comparison functions, used  for tset searching  */
struct FPDFogOfWarKeyFuncs : BaseKeyFuncs<FPDVisitedWorldDatum, FPDGridCell, false>
{
	/** @brief Returns what the t-set should consider the actual key of FPDStatMapping  */
	static const FPDGridCell& GetSetKey(const FPDVisitedWorldDatum& Element) { return Element.WorldPosition; }
	/** @brief Comparison function used by the t-set   */
	static bool Matches(const FPDGridCell& A, const FPDGridCell& B) { return A.IsSame2D(B); }
	/** @brief Gets inners typehash of the key, we want FPDStatMapping's with differing indexes and same tag to still match */
	static uint32 GetKeyHash(const FPDGridCell& Key)
	{
		FPDGridCell KeyCopy = Key;
		KeyCopy.Z = 0; // Always clear z
		return GetTypeHash(KeyCopy);
	}
};

/** @brief A simple struct which is acting as a simplified world definition, defaults to a 8km x 8km world size */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDWorldDescriptor
{
	GENERATED_BODY()
	
	/** @brief The absolute size of the positive part of the world plane */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D SizePositive {800000};

	/** @brief The absolute size of the negative part of the world plane */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D SizeNegative {800000};
	
	/** @brief The center of the world plane */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Pivot;
};

class FPDPixelArchive : public FBufferArchive
{
public:

	FPDPixelArchive(uint32 Command)
	{
		// make sure the command is at the start
		*this << Command;
	}

	// helper to serialize FColor arrays a bit cleaner
	FORCEINLINE FPDPixelArchive& operator<<(TArray<FColor>& Cols)
	{
		Cols.BulkSerialize(*this, false);
		return *this;
	}	
};

/** @brief A simple struct which is acting as a players world mappings */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDWorldData
{
    GENERATED_BODY()

	~FPDWorldData() { DeinitializeFOWTexture(); };
	
	/** @brief Checks with WorldCellStates if we've already been to the cell
	 * and calls 'ProcessChangeForCellVisibilityState' if the cell is new or it's visibility state has changed */
	void VisitedWorldLocation(const FPDVisitedWorldDatum& WorldLocation);

	/** @brief Convenience function: 
	 * @details Converts the location into a hashgrid cell (FPDGridCell) and constructs a FPDVisitedWorldDatum,
	 * to then call the overloaded 'VisitedWorldLocation' with the constructed type */
	void VisitedWorldLocation(const FVector& WorldLocation);

	/** @brief Initializes the member 'FOWTexture' */
	void InitializeFOWTexture();

	/** @brief Set texture data to either all black or all white*/
	void FillMap(const bool bDiscovered) const;

	/** @brief Set texture data for the pixels covering the given cell to white, black or gray/silver*/
	void UpdateCellStateOnMap(const FPDVisitedWorldDatum& WorldLocation) const;	
	
	/** @brief Set texture data to all white, removing all FOW */
	void SetMapDiscovered() const;
	
	/** @brief Set texture data to all black, clearing any FOW progress */
	void SetMapUndiscovered() const;

	/** @brief De-initializes the member 'FOWTexture' */
	void DeinitializeFOWTexture();

	/** @brief Update Texture Object from Texture Data */
	void UpdateTexture(bool bFreeInnerData = false);

	/** @brief Copies the data from 'SourceTextureData' raw array, into a TArray 'TargetTextureData' */
	static void CopyWorldFOWDataToFriendlyFormat(uint8* SourceTextureData, TArray<FColor>& TargetTextureData, uint32 PixelCount);	
	
	/** @brief Serializes the FoW data into the referenced archive 'Ar'.
	 * @details Calls 'CopyWorldFOWDataToFriendlyFormat' then calls BulkSerialize on the resulting array */
	static void SerializeWorldFOWData(FPDPixelArchive& Ar, uint8* SourceTextureData, uint32 PixelCount);

	/** @brief @todo / @inprogress Deserialize the FoW data from the referenced archive 'Ar'.
	 * @details Iterates the serialized properties of 'Ar' to find a matching array property */
	static TArray<FColor> DeserializeWorldFOWData(FPDPixelArchive& Ar);	
	TArray<FColor> DeserializeWorldFOWData(FPDPixelArchive& Ar, const bool bUpdateCurrentlyDisplayedTextureData);	
	
	/** @brief Describes the world this data pertains to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDWorldDescriptor WorldDescriptor;

	/** @brief The absolute world size */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite)
	FVector2D TotalWorldSize;
	
	
private:
	/** @brief World Cells and their state */
	TSet<FPDVisitedWorldDatum, FPDFogOfWarKeyFuncs> WorldCellStates;
	
	/** @brief FOW Texture, dynamic resource */
	UPROPERTY()
	UTexture2D* FOWTexture = nullptr;

	/** @brief Update Region Struct */
	FUpdateTextureRegion2D* TextureRegion = nullptr;
	
	/** @brief Array that contains the pixel values for the FOW texture */
	uint8* FOWTextureData = nullptr;

	/** @brief Total Bytes of Texture Data */
	uint32 FOWTextureDataSize;

	/** @brief Texture Data Sqrt Size */
	uint32 FOWTextureDataSqrtSize;

	/** @brief Total Count of Pixels in Texture */
	uint32 FOWTextureTotalPixels;	

	int32 InitCount = 0;
	
	friend class UPDFogOfWarSubsystem;
};

/** @brief Settings (table row) type for the Fog of war system. */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDFogOfWarSettings : public FTableRowBase
{
    GENERATED_BODY()
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog of War")
	FGameplayTag FOWMode;
};

UCLASS(BlueprintType)
class UPDFogOfWarSettingsSource : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** @brief Potential Sources for fog of war settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog of War", Meta = (RequiredAssetDataTags = "RowStructure=/Script/PDRTSBase.PDFogOfWarSettings"))
	TArray<TSoftObjectPtr<UDataTable>> SettingsTables{};	
};

/** @brief The camera manager class. Gets settings from a settings datatable-row handle */
UCLASS(BlueprintType)
class PDRTSBASE_API UPDFogOfWarSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UPDFogOfWarSubsystem();
	
	/** @brief Calls Super::BeginPlay then proceeds to call 'ProcessTables' */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** @brief Ensures the that the world is not a client world, only server or single player worlds allowed */
	bool HasCompleteAuthority() const;

	/** @brief @todo Update the fog of war, read positions and write to store */
	virtual void Tick(float DeltaTime) override;	
	
	/** @brief Checks with WorldCellStates if we've already been to the cell
	 * and calls 'ProcessChangeForCellVisibilityState' if the cell is new or it's visibility state has changed */
    UFUNCTION(BlueprintCallable, Category = "Fog of War")
	void UpdateWorldDatum(AController* RequestedController, const FPDVisitedWorldDatum& WorldLocation);

	/** @brief Convenience function: 
	 * @details Converts the location into a hashgrid cell (FPDGridCell) and constructs a FPDVisitedWorldDatum,
	 * to then call the overloaded 'VisitedWorldLocation' with the constructed type */
    UFUNCTION(BlueprintCallable, Category = "Fog of War")
	void UpdateWorldLocation(AController* RequestedController, const FVector& WorldLocation);

	/** @brief Sets the given mode and loads the settings for that mode */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Camera)
    void SetCustomMode(FGameplayTag Tag);

	/** @brief Boilerplate for unreals stat system, declares and returns a cycle stat for profiling purposes */
	virtual TStatId GetStatId() const override;


protected:
	/** @brief Process all camera settings tables */
    void ProcessTables();
    
public:
	/** @brief 'Fog of War settings' handle. Points to the settings entry we want to apply to this manager. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog of War", Meta = (RowType = "/Script/PDRTSBase.PDFogOfWarSettings"))
    FDataTableRowHandle CurrentSettingsHandle;
	/** @brief Pointer directly to the actual settings entry after it has been resolved by 'CurrentSettingsHandle' */
    FPDFogOfWarSettings* SettingPtr = nullptr;

	/** @brief The world coordinates we want the subsystem to have a view over */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDWorldDescriptor WorldDescriptor;	

	/** @brief Fog of War State: Requested*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog of War")
    FGameplayTag RequestedFOWState{};

	/** @brief Fog of War State: Previous */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog of War")
    FGameplayTag OldFOWState{};

	/** @brief Controllers we are tracking FOW state for */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Fog of War")
	TMap<AController*, FPDWorldData> TrackedControllers;

    /* Associative maps */
	/** @brief Map to associate a type tag with the manager settings it points to in the table it was sourced from */
    TMap<FGameplayTag /*Type tag*/, FPDFogOfWarSettings* /*SettingsRow*/> TagToSettings;
	/** @brief Map to associate a type tag with the table it was sourced from */
    TMap<FGameplayTag /*Type tag*/, const UDataTable* /*Table*/> TagToTable;
	/** @brief Map to associate a type tag with the rowname of the entry it was sourced from */
    TMap<FGameplayTag /*Type tag*/, FName /*Rowname*/> TagToRowname;
	
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