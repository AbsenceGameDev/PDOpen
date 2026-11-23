/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/EngineSubsystem.h"
#include "PDRTSPingerSubsystem.generated.h"

class UPDRTSBaseUnit;
class UMassEntitySubsystem;


/** @brief Settings to fire off a ping into rts subsystem entities.
 * @note Must be constructed for a valid target actor with a valid ownerID and job-tag, otherwise it will fire off ensures and checks. Will be impossible to miss the potential error */
USTRUCT(Blueprintable)
struct PDRTSBASE_API FPDEntityPingDatum
{
	GENERATED_BODY()

	/** @brief Base ctor needed by the engine to resolve the generated code properly */
	FPDEntityPingDatum() : FPDEntityPingDatum(0) {};
private:
	/** @brief private ctor, only accessible by friends. I.e. the ping subsystem may access it to create a fake datum to get typehash from */
	FPDEntityPingDatum(uint32 InInstanceID);
public:
	/** @brief Ctor with world actor we want to ping from and a JobTag to actually ping */
	FPDEntityPingDatum(AActor* InWorldActor, const FGameplayTag& InJobTag);

	/** @brief Ctor with world actor we want to ping from and a JobTag to actually ping;
	 * @param InWorldActor - Requires valid actor
	 * @param InJobTag - Requires valid job-tag
	 * @param InAltOwnerID - Optional OwnerID override, if the actor is known to be missing one or we want to target a job on another players buildable
	 * @param InInterval - Optional ping-interval override, Defaults to 10 second intervals.
	 * @param InMaxCountIntervals - Optional max ping count, Defaults to 15 times
	 */
	FPDEntityPingDatum(AActor* InWorldActor, const FGameplayTag& InJobTag, int32 InAltOwnerID, double InInterval = 10.0, int32 InMaxCountIntervals = 15);
	~FPDEntityPingDatum();

	/** @brief Converts the uint32 to a BP friendly uint8 array */
	TArray<uint8> ToBytes() const;
	/** @brief Converts a BP friendly uint8 array to our inner uint32 */
	void FromBytes(const TArray<uint8>&& Bytes);

	/** @brief Unsafe and non-checked function to set the ownerID from the owner of 'WorldActor' */
	void Unsafe_SetOwnerIDFromOwner();

	bool operator==(const FPDEntityPingDatum& OtherDatum) const { return this->InstanceID == OtherDatum.InstanceID; }
	
	/** @brief  Instance ID,only visible in instance, get from world actor*/
	uint32 InstanceID = INDEX_NONE;
	
	/** @brief The world actor this ping is calling entities for */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build|Procedures")
	AActor* WorldActor = nullptr;
	
	/** @brief  job tag associated with this ping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build|Procedures")
	FGameplayTag JobTag{};
	
	/** @brief  Owner ID, error if neither supplied and can't be found on WorldActors owner controller*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build|Procedures")
	int32 OwnerID = INDEX_NONE;

	/** @brief default ping at 10 seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build|Procedures")
	double Interval = 10.0;

	/** @brief default ping 15 times at most. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build|Procedures")
	int32 MaxCountIntervals = 15;	

	friend class UPDEntityPinger;
};

/** @brief Hash the ping data, really just pass through the world-actors UID*/
inline uint32 GetTypeHash(const FPDEntityPingDatum& PingDatum)
{
	/** @brief If out instance ID is set to 0, hash our inners to lessen likelyhood we get collisions */
	if (PingDatum.InstanceID == 0)
	{
		uint32 Hash = 0;
		Hash = HashCombine(Hash, GetTypeHash(PingDatum.JobTag));
		Hash = HashCombine(Hash, GetTypeHash(PingDatum.WorldActor));
		return Hash;
	}
	
	return PingDatum.InstanceID;
}

/** @brief EntityPinger subsystem. Handles managing our pings, running async tasks and such */
UCLASS()
class PDRTSBASE_API UPDEntityPinger : public UEngineSubsystem
{
	GENERATED_BODY()
	
	UPDEntityPinger() = default;
	/** @brief Init with a starting PingDatum if wanted. likely not needed */
	explicit UPDEntityPinger(const FPDEntityPingDatum& PingDatum);
public:
	/** @brief Shorthand to get the subsystem,
	 * @note as the engine will instantiate these subsystem earlier than anything will reasonably call Get()  */	
	static UPDEntityPinger* Get();

	/** @brief Add a ping-datum to PingDataAndHandles */
	UFUNCTION(BlueprintCallable)
	TArray<uint8> AddPingDatum(const FPDEntityPingDatum& PingDatum);
	/** @brief Removes a ping-datum from PingDataAndHandles */
	UFUNCTION(BlueprintCallable)
	void RemovePingDatum(const FPDEntityPingDatum& PingDatum);
	
	/** @brief Removes a ping-datum from PingDataAndHandles */
	UFUNCTION(BlueprintCallable)
	void RemovePingDatumWithHash(const FPDEntityPingDatum& PingDatum);

	/** @brief Gets a timer handle related to a ping-datum, found using BP friendly unsigned byte array */
	UFUNCTION(BlueprintCallable)
	FTimerHandle GetPingHandleCopy(const TArray<uint8>& PingHashBytes);
	/** @brief Gets a timer handle related to a ping-datum, found using unsigned int32, not usable in BP  */
	FTimerHandle GetPingHandleCopy(uint32 PingHash);
	
	/** @brief Adds or overwrites a ping datum and enables the pinging */
	UFUNCTION(BlueprintCallable)
	TArray<uint8> EnablePing(const FPDEntityPingDatum& PingDatum);
	UFUNCTION(BlueprintCallable)

	/** @brief Adds or overwrites a ping datum and enables the pinging, static call */
	static TArray<uint8> EnablePingStatic(const FPDEntityPingDatum& PingDatum);
	
	/** @brief Actual pinging logic, requests new action on idle and eligible entities */
	UFUNCTION(BlueprintNativeEvent)
	void Ping(UWorld* World, const FPDEntityPingDatum& PingDatum);

	/** @brief Disables and active ping via its ping datum */
	UFUNCTION(BlueprintCallable)
	void DisablePing(const FPDEntityPingDatum& PingDatum);
	
	/** @brief Disables and active ping via its ping datum, static call */
	UFUNCTION(BlueprintCallable)
	static void DisablePingStatic(const FPDEntityPingDatum& PingDatum);

	/** @brief The pings we are managing, at 10k actors to ping we'll still only take up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build|Procedures")
	TMap<FPDEntityPingDatum, FTimerHandle> PingDataAndHandles{};
	TMap<FTimerHandle, FPDEntityPingDatum> PingDataAndHandlesReverse{};
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