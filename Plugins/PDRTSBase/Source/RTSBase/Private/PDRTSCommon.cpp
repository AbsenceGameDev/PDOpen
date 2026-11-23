/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "PDRTSCommon.h"

/** Define the gameplay "AI.Jobs." tags */
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Job_Idle, "AI.Jobs.Idle");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Job_WalkToTarget, "AI.Jobs.WalkToTarget");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Job_GenericInteract, "AI.Jobs.GenericInteract");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Job_GatherResource, "AI.Jobs.GatherResource");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Job_BringBackResource, "AI.Jobs.BringBackResource");

/** Define the gameplay "CTRL.Ctxt." tags */
UE_DEFINE_GAMEPLAY_TAG(TAG_CTRL_Ctxt_BaseInput, "CTRL.Ctxt.BaseInput");
UE_DEFINE_GAMEPLAY_TAG(TAG_CTRL_Ctxt_DragMove, "CTRL.Ctxt.DragMove");
UE_DEFINE_GAMEPLAY_TAG(TAG_CTRL_Ctxt_WorkerUnitMode, "CTRL.Ctxt.WorkerUnitMode");
UE_DEFINE_GAMEPLAY_TAG(TAG_CTRL_Ctxt_BuildMode, "CTRL.Ctxt.BuildMode");
UE_DEFINE_GAMEPLAY_TAG(TAG_CTRL_Ctxt_ConversationMode, "CTRL.Ctxt.ConversationMode");



template<typename TBucketType>
MOpaquePImpl(TBucketTickHandler<TBucketType>)
{
	/** @brief Adds or removes our bucket to our tick array */
	template<bool bRemove>
	void HandleBucketFromTicks(TBucketType* BucketData, float StartTimeIfRegistering);
	
	/** @brief Max Limit of a tick interval : 100.0 (seconds)
	 * @note 100 seconds is excessive but doesn't matter, just something to clamp our timestamp against to offset the tick group properly
	 * @note 100.0 second ping limit puts our max index at 10 000 (Limit * IntervalScalar),
	 * @note meaning we have at max 10k ping groups every 100 seconds, we get a resolution of 1 ping group every hundredth of a second */
	static constexpr double HandlerTimeStepLimit = 100.0;

	double TimeAccumulation = 0;

	/** @brief Max Limit of a tick interval : (1.0 / (1.e-2f) */	
	static constexpr int32 IntervalScalar = (1.0 / (1.e-2f));
	/** @brief Max bucket index : HandlerTimeStepLimit * IntervalScalar */	
	static constexpr int32 MaxBucketIndex = HandlerTimeStepLimit * IntervalScalar;

	/** @brief List of tickgroup buckets */
	TArray<TArray<TBucketType*>> BucketTicks;
	/** @brief Special bucket that gets ticked every frame */
	TArray<TBucketType*> TickEveryFrame; 
	
	/** @brief List of interval groups 
	 * @note  tops out at 10k indices (this is MaxBucketIndex) */
	TArray<int32> InverseIntervals;

	/** @brief Cached value of last processed tick index */	
	int32 LastProcessedTickIndex = INDEX_NONE; 
	
};

template <typename TBucketType>
TBucketTickHandler<TBucketType>* TBucketTickHandler<TBucketType>::CreateHandler()
{
	PImpl* Handler = new PImpl;
	// Handler->Initialize();
	return Handler;
}

template <typename TBucketType>
void TBucketTickHandler<TBucketType>::DestroyHandler()
{
	// Self->Terminate();	
	delete Self.Ptr();	
}


template<typename TBucketType>
void TBucketTickHandler<TBucketType>::AddToAllTickgroups(TBucketType* BucketData)
{
	Self->TickEveryFrame.AddUnique(BucketData);
}

template<typename TBucketType>
void TBucketTickHandler<TBucketType>::RemoveFromAllTickgroups(TBucketType* BucketData)
{
	Self->TickEveryFrame.Remove(BucketData);	
}

template<typename TBucketType>
void TBucketTickHandler<TBucketType>::RegisterNewBucketForTicks(TBucketType* BucketData, double StartTime)
{
	Self->HandleBucketFromTicks<false>(BucketData, StartTime);
}

template<typename TBucketType>
void TBucketTickHandler<TBucketType>::DeregisterBucketFromTicks(TBucketType* BucketData)
{
	Self->HandleBucketFromTicks<true>(BucketData, 0.0);
}

template<typename TBucketType>
void TBucketTickHandler<TBucketType>::TickBuckets(float DeltaTime)
{
	Self->TimeAccumulation += DeltaTime;
	
	// Find offset from HandlerTimeStepLimit with StartTime
	const double WholeAndFraction = Self->TimeAccumulation / TBucketTickHandler::PImpl::HandlerTimeStepLimit;
	const double OnlyFraction = WholeAndFraction - static_cast<int32>(WholeAndFraction);

	// the fraction is our offset
	const int32 IntervalEntry = OnlyFraction * TBucketTickHandler::PImpl::IntervalScalar; 
	const int32 LastIdx       = Self->InverseIntervals.Last();

	if (Self->InverseIntervals.Contains(IntervalEntry)) 
	{
		// Found exising bucket
		const int32 TickIdx = Self->InverseIntervals.Find(IntervalEntry);
		const int32 IdxDelta = Self->LastProcessedTickIndex == INDEX_NONE ? TickIdx : TickIdx - Self->LastProcessedTickIndex;
			
		// @todo if true the nwe have missed 'IdxDelta - 1' amount of tick groups 
		// @todo handle wrapping when TickIndex has wrapped around but LastProcessesTickIndex is still at the end

		TArray<double> AccumulatedTimings;
		TArray<TBucketType*> FoundEntries;
		TArray<bool> SkipAccumulation; // if only found in the first entry, skip accumulating
			
		for (int32 Step = 0; Step < IdxDelta;)
		{
			// We step Backwards from previous and find all unique bucket entries, @note this might get hairy fast
			const int32 ReversingTickIndex = TickIdx - Step;
			for(const TBucketType& BucketData : Self->BucketTicks[ReversingTickIndex])
			{
				const int32 EntryIdx = FoundEntries.AddUnique(&BucketData); // this will be slow on cramped buckets, think on how to avoid this
				if (AccumulatedTimings.IsValidIndex(EntryIdx))
				{
					AccumulatedTimings[EntryIdx] += BucketData.UserInterval;
					SkipAccumulation[EntryIdx] = false; // Don't skip accumulation of missed frames if we have any missed frames 
				}
				else
				{
					// find the missing time here, at our first encountered missing step of any given bucketentry
					const double MissingTime =
						static_cast<double>(IntervalEntry - Self->InverseIntervals[ReversingTickIndex]) / static_cast<double>(TBucketTickHandler::PImpl::IntervalScalar);
					
					
					AccumulatedTimings.EmplaceAt(EntryIdx, BucketData.UserInterval + MissingTime);

					bool bShouldSkip = ReversingTickIndex == TickIdx; 
					BucketData.LastTickOffset =
						bShouldSkip ?
							(FMath::IsNearlyZero(BucketData.LastTickOffset) == false ?
								BucketData.LastTickOffset
								: 0.0)
							: MissingTime;
					
					SkipAccumulation.EmplaceAt(EntryIdx, bShouldSkip); 
				}
			
			}
			++Step;
		}

		// Steps our bucket (and potentially our missed bucket entries from previous buckets) and fires their tick with the proper accumulated (missed time) 
		const int32 FoundEntryLimit = FoundEntries.Num();
		for (int32 Step = 0; Step < FoundEntryLimit; )
		{
			TBucketType* CurrentBucketEntry = FoundEntries[Step];
			double AccumulatedTick = SkipAccumulation[Step]
				? CurrentBucketEntry->UserInterval - CurrentBucketEntry->LastTickOffset // This is our potential offset when we have fastforwarded a missing tick group
				: AccumulatedTimings[Step];
			CurrentBucketEntry->LastTickOffset = 0.0; // always clear after using it, this 
			
			CurrentBucketEntry->Execute(AccumulatedTick); 
			Step++;
		}
		Self->LastProcessedTickIndex = TickIdx;
	}

	// Special bucket for things that tick every frame
	for(TBucketType* ChunkData : Self->TickEveryFrame)
	{
		ChunkData->Execute(DeltaTime);
	}
}
template <typename TBucketType>
template <bool bRemove>
void TBucketTickHandler<TBucketType>::PImpl::HandleBucketFromTicks(TBucketType* BucketData, float StartTimeIfRegistering)
{
	const double ClampedInterval = FMath::Min(BucketData->UserInterval, TBucketTickHandler::PImpl::HandlerTimeStepLimit);

	// If ClampedInterval is zero, put it in ALL tickgroups
	if (ClampedInterval <= (1.e-4f))
	{
		if constexpr (bRemove) { TBucketTickHandler<TBucketType>::RemoveFromAllTickgroups(BucketData); }
		else { TBucketTickHandler<TBucketType>::AddToAllTickgroups(BucketData); }
		
		return;
	}

	// Find offset from HandlerTimeStepLimit with StartTime
	const double WholeAndFraction = StartTimeIfRegistering / TBucketTickHandler::PImpl::HandlerTimeStepLimit;
	const double OnlyFraction = bRemove ?  BucketData->StoredFraction : (WholeAndFraction) - static_cast<int32>(WholeAndFraction);

	// How many times do we need to apply this chunk, how many buckets does it apply to
	const int32 TotalSteps = TBucketTickHandler::PImpl::HandlerTimeStepLimit / ClampedInterval;
	for (int32 Step = 0; Step <= TotalSteps; Step++)
	{
		// the fraction is our offset
		const int32 IntervalOffset = (OnlyFraction + (OnlyFraction * Step)) * TBucketTickHandler::PImpl::IntervalScalar;

		// 
		// Convert to inverse integer, to avoid floating point comparisons when Contains() runs and to avoid removing our ability to match against the timestep properly
		int32 TruncatedInverseInterval = (ClampedInterval * TBucketTickHandler::PImpl::IntervalScalar) + IntervalOffset;


		if constexpr (bRemove)
		{
			// 
			// Remove from bucket if found
			// Sort into buckets, first find where they are on the timeline by inversing them with epsilon
			if (InverseIntervals.Contains(TruncatedInverseInterval) == false) { return; }

			// Found exising bucket
			const int32 TickIdx = InverseIntervals.Find(BucketData->TruncatedInverseTickInterval);
			BucketTicks[TickIdx].Remove(BucketData);
			BucketData->TruncatedInverseTickInterval = INDEX_NONE;
			
			return;
		}
		else
		{
			// 
			// Sort into buckets, first find where they are on the timeline by inversing them with epsilon
			if (InverseIntervals.Contains(TruncatedInverseInterval))
			{
				// Found exising bucket
				const int32 TickIdx = InverseIntervals.Find(TruncatedInverseInterval);
				BucketTicks[TickIdx].Emplace(BucketData);
				return;
			}
			
			// Create new bucket
			TArray<TBucketType*> NewBucket = {BucketData};
			const int32 NewBucketIdx = BucketTicks.Emplace(NewBucket);
			InverseIntervals.EmplaceAt(NewBucketIdx, TruncatedInverseInterval);
		}
	}

	if (bRemove == false) { BucketData->StoredFraction = OnlyFraction; }
}

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
