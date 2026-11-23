/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "PDRTSSharedHashGrid.generated.h"

/** @brief Dynamic hash-grid developer settings, default to 200.0 cell size, modify in .ini config or in editor project settings */
UCLASS(Config = "Game", DefaultConfig)
class PDRTSBASE_API UPDHashGridDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPDHashGridDeveloperSettings(){}
	
	/** @brief Default grid data */
	UPROPERTY(Config, EditAnywhere, Category = "HashGrid")
	double UniformCellSize = 200.0f; 
	
};

/** @brief Intvector that we are using a floored and then truncated gridcell
 *  @note has some functions to check if a given cell is a neighbour in 2d, and/or in 3d, to convert to a floatvector alongside some basic operators overloads */
USTRUCT()
struct PDRTSBASE_API FPDGridCell : public FIntVector 
{
	GENERATED_BODY()
	
	/** @brief Converts cell to float-vector  */
	inline FVector ToFloatVector(const double ScalingFactor = 1.0) const
	{
		FVector RetVector;
		RetVector.X = this->X;
		RetVector.Y = this->Y;
		RetVector.Z = this->Z;
		return RetVector * ScalingFactor;
	}

	static FPDGridCell Construct(int32 StartValue) { return FPDGridCell{{StartValue, StartValue, StartValue}};}
	static FPDGridCell Construct(int32 InX, int32 InY, int32 InZ) { return FPDGridCell{{InX, InY, InZ}}; }

	/** @brief  Compares if cell is neighbour in X and Y */
	bool IsNeighbour2D(const FPDGridCell& OtherCell) const
	{
		const FPDGridCell Delta = OtherCell - *this; 
		return Delta.X >= -1 && Delta.X <= 1 && Delta.Y >= -1 && Delta.Y <= 1;
	}
	
	/** @brief  Compares if cell is neighbour in X, Y and Z */
	bool IsNeighbour3D(const FPDGridCell& OtherCell) const
	{
		const FPDGridCell Delta = OtherCell - *this; 
		return IsNeighbour2D(OtherCell) && Delta.Z >= -1 && Delta.Z <= 1 ;
	}

	/** @brief this->XY must be equal to Other.XY */
	bool IsSame2D(const FPDGridCell & OtherCell) const
	{
		return this->X == OtherCell.X
			&& this->Y == OtherCell.Y;
	}	

	/** @brief this->XYZ must be equal to Other.XYZ */
	bool operator==(const FPDGridCell & OtherCell) const
	{
		return this->X == OtherCell.X
		    && this->Y == OtherCell.Y
		    && this->Z == OtherCell.Z;
	}
	/** @brief this->XYZ must be equal to Other.XYZ */
	bool operator!=(const FPDGridCell & OtherCell) const
	{
		return false == (*this == OtherCell);
	}

	/** @brief this->XYZ must be lower to Other.XYZ */
	bool operator<(const FPDGridCell & OtherCell) const
	{
		return this->X <= OtherCell.X
		    && this->Y <= OtherCell.Y
		    && this->Z <= OtherCell.Z
			&& *this != OtherCell;
	}
	/** @brief this->XYZ must be higher to Other.XYZ */
	bool operator>(const FPDGridCell & OtherCell) const
	{
		return this->X >= OtherCell.X
		    && this->Y >= OtherCell.Y
		    && this->Z >= OtherCell.Z
			&& *this != OtherCell;
	}


	FPDGridCell& operator+=(const FPDGridCell& OtherCell) 
	{
		*this = *this + OtherCell;
		return *this;
	}
	FPDGridCell& operator-=(const FPDGridCell & OtherCell) 
	{
		*this = *this - OtherCell;
		return *this;
	}
	FPDGridCell operator+(const FPDGridCell & OtherCell) const
	{
		FPDGridCell ThisCopy = *this;
		ThisCopy.X += OtherCell.X;
		ThisCopy.Y += OtherCell.Y;
		ThisCopy.Z += OtherCell.Z;
		return ThisCopy;
	}
	FPDGridCell operator-(const FPDGridCell& OtherCell) const
	{
		FPDGridCell ThisCopy = *this;
		ThisCopy.X -= OtherCell.X;
		ThisCopy.Y -= OtherCell.Y;
		ThisCopy.Z -= OtherCell.Z;
		return ThisCopy;
	}


	FPDGridCell& operator+=(const int32& OtherVal) 
	{
		*this = *this + OtherVal;
		return *this;
	}
	FPDGridCell& operator-=(const int32& OtherVal) 
	{
		*this = *this - OtherVal;
		return *this;
	}	
	FPDGridCell operator+(const int32& OtherVal) const
	{
		FPDGridCell ThisCopy = *this;
		ThisCopy.X += OtherVal;
		ThisCopy.Y += OtherVal;
		ThisCopy.Z += OtherVal;
		return ThisCopy;
	}	
	FPDGridCell operator-(const int32& OtherVal) const
	{
		FPDGridCell ThisCopy = *this;
		ThisCopy.X -= OtherVal;
		ThisCopy.Y -= OtherVal;
		ThisCopy.Z -= OtherVal;
		return ThisCopy;
	}

};


struct PDRTSBASE_API FRTSOFindResourcesParameters 
{
	FPDGridCell GridCell;
	const FGameplayTag ResourceType{}; 
	int32 TargetResourceAmount = 1;
	int32 SearchDepth = INDEX_NONE;
};	

/** @brief Dynamic hash-grid sub-system, Manages grid functionality.
 * @note Loads settings from UPDHashGridDeveloperSettings, and keeps it synced.
 * @note Has functions generate cell index constructs and apply them, transfer between vector and gridcell. Allows for easy snapping to grid  */
UCLASS()
class PDRTSBASE_API UPDHashGridSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	/** @brief Shorthand to get the subsystem,
	 * @note as the engine will instantiate these subsystem earlier than anything will reasonably call Get()  */
	static UPDHashGridSubsystem* Get();
	
	/** @brief Loads hashgrid config from developer settings and binds to said developer settings value changes */
	virtual void Initialize(FSubsystemCollectionBase& Collection) final override;

#if WITH_EDITOR
	/** @brief Updates 'UniformCellSize'. Function is bound to default UPDHashGridDeveloperSettings OnSettingChanged */
	void OnDeveloperSettingsChanged(UObject* SettingsToChange, const FPropertyChangedEvent& PropertyEvent);
#endif

	inline static FVector StepLocation(const FVector& TrueLocation)
	{
		return UPDHashGridSubsystem::Get()->GetCellVector(TrueLocation);
	}
	inline FVector StepLocationConst(const FVector& TrueLocation) const
	{
		return UPDHashGridSubsystem::Get()->GetCellVector(TrueLocation);
	}

	/** @brief  Floor vector and return as gridcell index */
    inline static struct FPDGridCell FloorVectorC(const FVector&& LocationToCell)
    {
        return FPDGridCell
        {
            {
				static_cast<int32_t>(FMath::Floor(LocationToCell.X)),
				static_cast<int32_t>(FMath::Floor(LocationToCell.Y)),
				static_cast<int32_t>(FMath::Floor(LocationToCell.Z))
            }
        };
    }

	/** @brief  Floor vector and return as FVector */
	inline static FVector FloorVectorV(const FVector&& LocationToCell)
    {
    	return FVector
		{
		    FMath::Floor(LocationToCell.X),
			FMath::Floor(LocationToCell.Y),
			FMath::Floor(LocationToCell.Z)
		};
    }	
  
	/** @brief  Calculate cell index */
    inline FPDGridCell GetCellIndex(const FVector& LocationToCell) const
    {
        return StaticCell(LocationToCell, UniformCellSize);
    }

	/** @brief  Calculate cell-clamped vector */
	inline FVector GetCellVector(const FVector& LocationToCell) const
    {
    	return StaticVector(LocationToCell, UniformCellSize);
    }	

    /** @brief Slow if used often, instead cache a pointer to the subsystem and call it 'GetCellIndex' instead  */
	static inline FPDGridCell GetCellIndexStatic(const FVector& LocationToCell)
    {
    	return StaticCell(LocationToCell, UPDHashGridSubsystem::Get()->UniformCellSize);
    }

	/** @brief Slow if used often, instead cache a pointer to the subsystem and call it 'GetCellIndex' instead  */
	static inline FVector GetCellVectorStatic(const FVector& LocationToCell)
    {
    	return StaticVector(LocationToCell, UPDHashGridSubsystem::Get()->UniformCellSize);
    }

	static inline FPDGridCell StaticCell(const FVector& LocationToCell, const double CellSize)
    {
    	return FloorVectorC(LocationToCell / CellSize); 
    }
	static inline FVector StaticVector(const FVector& LocationToCell, const double CellSize)
    {
    	return FloorVectorV(LocationToCell / CellSize) * CellSize; // Bring it back to proper world space dims by 
    }

	/** @brief Uniform cell size instance data, value is dictated by the value in the developer settings */
	UPROPERTY()
    double UniformCellSize = 200.0f; 
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