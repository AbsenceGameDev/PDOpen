/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDPersistenceInterface.generated.h"

//
// Imagine a static set of all possible server IDs, it would contain 65535 values, * 2 bytes = approx. 130kb

/** @brief PersistenID subsystem. @todo add functions to pipe connections into */
UCLASS()
class UPDPersistentIDSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	/** @brief Shorthand to get the subsystem,
	 * @note as the engine will instantiate these subsystem earlier than anything will reasonably call Get()  */
	static UPDPersistentIDSubsystem* Get();
	
	/** @brief Persistent IDs tracked by the subsystem */
	UPROPERTY()
	TSet<int32> TrackedPersistentIDs;
	
};


USTRUCT()
struct FPDPersistentID
{
	GENERATED_BODY()

public:
	/** @brief Default empty ctor defines an invalid persistent ID*/
	FPDPersistentID() : BitSet_ID(INVALID_ID) {}

	/** @brief Supplied ID might cause conflicts, only meant to be used internally when iterating comparisons */
	FPDPersistentID(int32 InID) : BitSet_ID(InID) {}
	
	/** @brief Gets the internal ID integer */
	int32 GetID() const { return BitSet_ID; }
	/** @brief Checks that the internal ID integer is not equals to INVALID_ID. Dos not check with the subsystem it is actually generated and tracked  */
	bool IsValidID() const { return BitSet_ID != INVALID_ID; }
	
	/** @brief Generates a new persistent ID. Compares with the tracked persistent ID list in 'UPDPersistentIDSubsystem' to ensure no collisions */
	static FPDPersistentID GenerateNewPersistentID()
	{
		const TSet<int32>& ExistingIDs = UPDPersistentIDSubsystem::Get()->TrackedPersistentIDs;
		if (ExistingIDs.Num() > INT16_MAX)
		{
			// Too many in the set, log as a warning and return invalid ID
			return FPDPersistentID{INVALID_ID};
		}

		// simple iteration algorithm which is reliable and in most cases should be faster than a regular iteration
		// 1. Generate random Search start point, and pick random search direction
		const int64 RandomStartingPoint = FMath::RandRange(0, INT16_MAX);
		const bool  RandomDirectionForward = FMath::RandBool();

		
		// 2. Search first in randomly selected direction.
		// If first search fails, reverse directions and start over from the selected point
		FPDPersistentID FoundIdx;
		if (RandomDirectionForward)
		{
			if (ForwardSearch(ExistingIDs, RandomStartingPoint, FoundIdx)) { return FoundIdx; }
			if (ReverseSearch(ExistingIDs, RandomStartingPoint, FoundIdx)) { return FoundIdx; }
		}
		else
		{
			if (ReverseSearch(ExistingIDs, RandomStartingPoint, FoundIdx)) { return FoundIdx; }
			if (ForwardSearch(ExistingIDs, RandomStartingPoint, FoundIdx)) { return FoundIdx; }
		}

		// Search failed,
		UE_LOG(LogTemp, Warning, TEXT("FPDPersistentID::GenerateNewPersistentID -- failed search, meaning no spot left for generated IDs"))
		return FPDPersistentID{INVALID_ID};
	}
	

	/* Comparator operators - Start */
	bool operator==(const FPDPersistentID& Other) const { return this->BitSet_ID == Other.BitSet_ID; } 
	bool operator!=(const FPDPersistentID& Other) const { return (*this == Other) == false; }
	bool operator> (const FPDPersistentID& Other) const { return this->BitSet_ID > Other.BitSet_ID; }
	bool operator< (const FPDPersistentID& Other) const { return this->BitSet_ID < Other.BitSet_ID; }
	bool operator>=(const FPDPersistentID& Other) const { return this->BitSet_ID >= Other.BitSet_ID; }
	bool operator<=(const FPDPersistentID& Other) const { return this->BitSet_ID <= Other.BitSet_ID; }

	bool operator==(const int32& OtherID) const { return this->BitSet_ID == OtherID; }
	bool operator!=(const int32& OtherID) const { return (*this == OtherID) == false; }
	bool operator> (const int32 OtherID) const { return this->BitSet_ID > OtherID; }
	bool operator< (const int32 OtherID) const { return this->BitSet_ID < OtherID; }	
	bool operator>=(const int32 OtherID) const { return this->BitSet_ID >= OtherID; }
	bool operator<=(const int32 OtherID) const { return this->BitSet_ID <= OtherID; }	
	FPDPersistentID operator+(const FPDPersistentID& Other) const { return this->BitSet_ID + Other.BitSet_ID;}
	FPDPersistentID operator+=(const FPDPersistentID& Other){ return this->BitSet_ID = this->BitSet_ID + Other.BitSet_ID;}
	FPDPersistentID& operator++(){ BitSet_ID++; return *this;}
	FPDPersistentID& operator--(){ BitSet_ID--; return *this;}
	FPDPersistentID& operator++(int){ FPDPersistentID Old = *this; BitSet_ID++; return Old;}
	/* Comparator operators - End */
	
private:
	
	/** @brief Searches the list in reverse, from the given starting point, and only returns true if it finds an unused ID*/
	static bool ReverseSearch(const TSet<int32>& ExistingIDs, const int64 RandomStartingPoint, FPDPersistentID& Value)
	{
		if (ExistingIDs.IsEmpty()) { Value = RandomStartingPoint; return true; }
		TSet<int32>& MutableExistingIDs = UPDPersistentIDSubsystem::Get()->TrackedPersistentIDs;

		// relies on uintwrapping
		for (FPDPersistentID Step = RandomStartingPoint; Step != UINT32_MAX ;)
		{
			if (ExistingIDs.Contains(Step.GetID())) { --Step; }
			else
			{
				MutableExistingIDs.Emplace(Step.GetID());
				Value = Step;
				return true;
			}
		}
		return false;
	}

	/** @brief Searches the list in forward direction, from the given starting point, and only returns true if it finds an unused ID*/
	static bool ForwardSearch(const TSet<int32>& ExistingIDs, const int64 RandomStartingPoint, FPDPersistentID& Value)
	{
		if (ExistingIDs.IsEmpty()) { Value = RandomStartingPoint; return true; }
		TSet<int32>& MutableExistingIDs = UPDPersistentIDSubsystem::Get()->TrackedPersistentIDs;
		
		for (FPDPersistentID Step = RandomStartingPoint; Step < UINT32_MAX;)
		{
			if (ExistingIDs.Contains(Step.GetID())) { ++Step; }
			else
			{
				MutableExistingIDs.Emplace(Step.GetID());
				Value = Step;
				return true;
			}
		}
		return false;
	}

public:
	inline static constexpr int32 INVALID_ID = 0; // is our INVALID_ID

private:
	/** @brief Wrapped data (Persistent ID) */
	int32 BitSet_ID;	
};

/** @brief Hash the persistent ID*/
inline uint32 GetTypeHash(const FPDPersistentID& PersistentID)
{
	return FCrc::MemCrc32(&PersistentID,sizeof(FPDPersistentID));
}

/** @brief Engine boilerplate */
UINTERFACE() class UPDPersistenceInterface : public UInterface { GENERATED_BODY() };

/** @brief Persistence interface */
class RTSOPEN_API IPDPersistenceInterface
{
	GENERATED_BODY()

public:
	/** @brief default impl returns an invalid ID */
	virtual FPDPersistentID GetPersistentID() { return FPDPersistentID(); };
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