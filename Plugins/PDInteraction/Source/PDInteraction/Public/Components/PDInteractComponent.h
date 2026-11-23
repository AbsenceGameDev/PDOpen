/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "PDInteractCommon.h"
#include "Components/ActorComponent.h"
#include "PDInteractComponent.generated.h"

class UCameraComponent;
/**
 * @brief This component has two main functions: \n 1. Handling interaction logic. \n 2. Performing traces per frame, for downstream purposes
 */
UCLASS(ClassGroup=(Custom), Meta=(BlueprintSpawnableComponent))
class PDINTERACTION_API UPDInteractComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY() 

public:
	/** @brief Calls Super::BeginPlay() and Prerequisites()*/
	virtual void BeginPlay() override; 
	/** @brief Calls trace functions. Which trace functions depends on which trace type has been assigned to 'TraceSettings.TickTraceType'*/
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	/** @brief Assign the camera we will be using for our tracing view/rot transform */
	void SetCamera(UCameraComponent* InCamera) { Camera = InCamera; }; 

	/** @brief Finds the closest interactable of the actors in the radial trace  */
	void FindClosestRadialTraceActor(const FVector& OwnerActorLocation, const AActor*& ClosestInteractable);

	/** @brief Registers the start interaction time and sets the interaction timer that accumulates the held time */
	UFUNCTION(BlueprintCallable)
	virtual void BeginInteraction(FName TraceID);
	/** @brief Runs on a timer from the start of an interaction until the end of one */
	UFUNCTION()
	virtual void TimerInteraction(FName TraceID);
	/** @brief Called when an interaction ends, either by completing the interaction or stopping early 
	 * - Registers the current interaction time
	 * - calculates total interaction time
	 * - clears the interaction timer */
	UFUNCTION(BlueprintCallable)
	virtual void EndInteraction(FName TraceID);

	
	/** @brief Returns latest trace-result, or enforcing the latest valid in the buffer if one exists */
	UFUNCTION(BlueprintCallable)
	const FPDTraceResult& GetTraceResult(bool bSearchForValidCachedResults, const FName TraceID) const;	
	/** @brief Returns latest trace-result, or enforcing the latest valid in the buffer if one exists 
	 * @note this function is unsafe to call directly, assumes Idx is known to be valid. */
	UFUNCTION(BlueprintCallable)
	const FPDTraceResult& GetTraceResultByIdx(bool bSearchForValidCachedResults, const int32 TraceIndex) const; 
	/** @brief Returns the active trace settings for this component. This contains the tick-type, tick behaviours etc. */
	UFUNCTION(BlueprintCallable)
	const FPDTraceSettings& GetTraceSettings() const;
	/** @brief Overwrites the current trace settings */
	UFUNCTION(BlueprintCallable)
	void SetTraceSettings(const FPDTraceSettings& NewSettings);
	/** @brief Checks if 'TraceBuffer' contains valid trace results */
	UFUNCTION(BlueprintCallable)
	bool ContainsValidTraceResults(FName TraceID) const;

	/** @brief Returns a list of all interactables within the given radius */
	UFUNCTION(BlueprintCallable)
	TArray<AActor*> GetAllInteractablesInRadius(double Radius = 500.0, bool bIgnorePerObjectInteractionDistance = false);
	
	/** @brief Meant to be used on the server for one-off for comparisons/validations */
	UFUNCTION(BlueprintCallable)
	void TraceToTarget(const FVector& TraceEnd);
	/** @brief Meant to be used on the server for one-off for comparisons/validations */
	void TraceToTarget(const FVector& TraceStart, const FVector& TraceEnd);
	/** @brief Meant to be used on the server for one-off for comparisons/validations */
	void TraceToTarget(const FVector& TraceEnd, FCollisionQueryParams& TraceParams);
	/** @brief Meant to be used on the server for one-off for comparisons/validations */
	void TraceToTarget(const FVector& TraceStart, const FVector& TraceEnd, FCollisionQueryParams& TraceParams);
	/** @brief Meant to be used on the server for one-off for comparisons/validations */
	UFUNCTION(BlueprintCallable)
	/** @brief Meant to be used on the server for one-off for comparisons/validations */
	FPDTraceResult TraceToTargetAndReset(const FVector& TraceEnd);
	/** @brief Meant to be used on the server for one-off for comparisons/validations */
	FPDTraceResult TraceToTargetAndReset(const FVector& TraceStart, const FVector& TraceEnd);
	/** @brief Meant to be used on the server for one-off for comparisons/validations */
	FPDTraceResult TraceToTargetAndReset(const FVector& TraceEnd, FCollisionQueryParams& TraceParams);
	/** @brief Meant to be used on the server for one-off for comparisons/validations */
	FPDTraceResult TraceToTargetAndReset(const FVector& TraceStart, const FVector& TraceEnd, FCollisionQueryParams& TraceParams);

	/** @brief Returns the max trace distance allowed for the trace component no matter which object.
	 * Object can set per interaction distances within the range [0, GetMaxTraceDistance()] */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE double GetMaxTraceDistance(const FName TraceID) const
	{
		const bool bTraceIdxExists = TraceNameToIndexMappings.Contains(TraceID);
		return bTraceIdxExists 
			? TraceSettings.TickTraceTypeSettings[TraceNameToIndexMappings[TraceID]].MaxTraceDistanceInUnrealUnits
			: 0.0;
	}
	UFUNCTION(BlueprintCallable)
	FORCEINLINE double GetMaxTraceDistanceByIdx(const int32 TraceIdx) const
	{
		const bool bTraceIdxExists = TraceSettings.TickTraceTypeSettings.IsValidIndex(TraceIdx);
		return bTraceIdxExists ? TraceSettings.TickTraceTypeSettings[TraceIdx].MaxTraceDistanceInUnrealUnits : 0.0;
	}

	/** @brief Return the radially traced actor.
	 * @todo integrate some mass entity tracing into this? Or a subclass of the trace component
	 * @todo and rewrite the base class slightly to allow smooth integration between the systems on the game module level */
	UFUNCTION(BlueprintCallable)
	const TArray<AActor*>& GetRadialTraceResults(const FName TraceID) const 
	{ 
		static const TArray<AActor*> DummyReturn;
		const bool bTraceIdxExists = TraceNameToIndexMappings.Contains(TraceID);
		return bTraceIdxExists ? TraceBuffer[TraceNameToIndexMappings[TraceID]].RadialTraceActors : DummyReturn;
	}
	UFUNCTION(BlueprintCallable)
	const TArray<AActor*>& GetRadialTraceResultsByIdx(const int32 TraceIdx) const 
	{ 
		static const TArray<AActor*> DummyReturn;
		const bool bTraceIdxExists = TraceBuffer.IsValidIndex(TraceIdx);
		return bTraceIdxExists ? TraceBuffer[TraceIdx].RadialTraceActors : DummyReturn;
	}
	/** @brief Gets the result from the first valid tracer of Radial type */
	UFUNCTION(BlueprintCallable)
	const TArray<AActor*>& GetFirstRadialTraceResults() const;



	/** @brief Returns a const reference of the trace buffer */
	UFUNCTION(BlueprintCallable)
	const FPDTraceBuffer& GetTraceBuffer(const FName TraceID) const 
	{ 
		static const FPDTraceBuffer DummyReturn;
		const bool bTraceIdxExists = TraceNameToIndexMappings.Contains(TraceID);
		return bTraceIdxExists ? TraceBuffer[TraceNameToIndexMappings[TraceID]] : DummyReturn; 
	}
	UFUNCTION(BlueprintCallable)
	const FPDTraceBuffer& GetTraceBufferByIdx(const int32 TraceIdx) const 
	{ 
		static const FPDTraceBuffer DummyReturn;
		const bool bTraceIdxExists = TraceBuffer.IsValidIndex(TraceIdx);
		return bTraceIdxExists ? TraceBuffer[TraceIdx] : DummyReturn; 
	}	

	/** @brief Set override trace position. This is a temporary override which modifies
	 * the trace behaviour by translating the trace start position  */
	UFUNCTION(BlueprintCallable)
	void SetOverrideTracePosition(FVector NewPosition, const bool bInResetOverrideNextFrame = true)
	{
		OverridePosition = NewPosition;
		bResetOverrideNextFrame = bInResetOverrideNextFrame;
	};
	
protected:
	/** @brief Sets up TraceSettings & TraceBuffer and then enabling the component tick */
	UFUNCTION()
	virtual void Prerequisites();
	
	/** @brief Clear the trace buffer */
	UFUNCTION(BlueprintCallable)
	void ClearCurrentTraceResult();
	UFUNCTION(BlueprintCallable)
	void ClearTraceResult(const int32 TraceIdx);	
	UFUNCTION(BlueprintCallable)
	void ClearAllTraceResults();
	

	/** @brief Performs a shape trace along a line/direction */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void PerformLineShapeTrace(double DeltaSeconds, const FPDTraceTickSettings& PerTickTraceSettings);

	/** @brief Performs a radial trace, outwards radially from the start position */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void PerformRadialTrace(double DeltaSeconds, const FPDTraceTickSettings& PerTickTraceSettings);
	
	
	void PerformCameraTrace(FVector& TraceStart, FVector& TraceEnd, const FPDTraceTickSettings& PerTickTraceSettings, FCollisionQueryParams& TraceParams, FHitResult& TraceHitResult, EPDTraceResult& TraceResultFlag) const;
	/** @brief Calls TracePass. Performs a comparative trace between the input start/end trace and a trace towards the same hit location with a
	 * start position on the owning entities location adjusted in z by eye height. Then making sure they both hit the same target */
	void PerformComparativeTraces(FVector& TraceStart, FVector& TraceEnd, const FPDTraceTickSettings& PerTickTraceSettings, FCollisionQueryParams& TraceParams, FHitResult& TraceHitResult, EPDTraceResult& TraceResultFlag) const;
	/** @brief Calls TracePass. Performs a simple trace from the given start/end position and parameters. */
	void PerformSimpleTrace(const FVector& TraceStart, const FVector& TraceEnd, ECollisionChannel TraceChannel, FCollisionQueryParams& TraceParams, FHitResult& TraceHitResult, bool& bTraceResultFlag) const;
	/** @brief Actual Shape trace-pass. Sweeps the shape along the direction  */
	void TracePass(const FVector& TraceFromLocation, const FVector& TraceEnd, ECollisionChannel TraceChannel, FCollisionQueryParams& TraceParams, FHitResult& TraceHitResult, bool& bTraceResultFlag) const;
	
public:
	/** @brief Shape extent for shape trace */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double BoxTraceExtent = 10.0;

	/** @brief The assigned settings. Assigned from tablerow struct of type FPDTraceSettings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (RowType = "/Script/PDInteraction.PDTraceSettings"))
	FDataTableRowHandle TraceSettingsHandle;	
	
protected:
	/** @brief Actually loaded trace settings. Loaded from value pointed to by 'TraceSettingsHandle' */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite)
	FPDTraceSettings TraceSettings{};
	/** @brief Actor list of actors to ignore, assignable at runtime*/
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite)
	TArray<AActor*> RuntimeIgnoreList{};
	/** @brief The actual trace buffer which holds trace frame data*/
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TArray<FPDTraceBuffer> TraceBuffer{};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TMap<FName /* TraceID*/, const AActor*> ActiveInteractionTargets;
	
	/** @brief THe timer handle for an active interaction, will be cleared after the interaction finishes */
	UPROPERTY()
	TMap<FName, FTimerHandle> ActiveInteractionTimers{};

	/** @brief The current held interaction time, flushes after an interaction has finished */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TMap<FName, double> CurrentInteractionTimes;
	/** @brief The start held interaction time, flushes after an interaction has finished */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TMap<FName, double> StartInteractionTimes;
	
	/** @brief Dont access directly and Call 'SetOverrideTracePosition' instead. An override position to start the trace from. */
	FVector OverridePosition;
	/** @brief Dont access directly and Call 'SetOverrideTracePosition' instead. An override position to start the trace from. */
	bool bResetOverrideNextFrame = false;


private:
	/** @brief The index of the most recent trace type. Updates each frame and indicates which index is currently being processed */
	int32 CurrentTraceIndex = 0;
	/** @brief Map ID Names to indices in the array */
	TMap<FName, int32> TraceNameToIndexMappings{};

	/** @brief Camera used for view loc/rot data */
	UPROPERTY()
	UCameraComponent* Camera = nullptr;
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