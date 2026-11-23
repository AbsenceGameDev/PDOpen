/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "MassEntityTypes.h"
#include "Containers/Deque.h"
#include "PDInteractCommon.generated.h"

DECLARE_LOG_CATEGORY_CLASS(PDLog_Interact, Log, All);

/* Max distances */
constexpr float DEFAULT_PEROBJECT_MAX_INTERACTION_DISTANCE = 150;
constexpr float DEFAULT_TRACER_MAX_INTERACTION_DISTANCE = 1500;
constexpr float DEFAULT_TRACER_MAX_RADIAL_DISTANCE = 500;


namespace PD::Interact::Constants
{
	namespace Limits
	{
		constexpr double Double = DBL_MAX;
	}

	inline constexpr FVector INVALID_WORLD_LOC = FVector(DBL_MAX, UE::Math::TVectorConstInit{});
}


/* Interaction (trace) channels */
constexpr ECollisionChannel DEDICATED_INTERACT_CHANNEL_ALT18 = ECollisionChannel::ECC_GameTraceChannel18;
constexpr ECollisionChannel DEDICATED_INTERACT_CHANNEL_ALT17 = ECollisionChannel::ECC_GameTraceChannel17;
constexpr ECollisionChannel DEDICATED_INTERACT_CHANNEL_ALT16 = ECollisionChannel::ECC_GameTraceChannel16;
constexpr ECollisionChannel DEDICATED_INTERACT_CHANNEL_ALT15 = ECollisionChannel::ECC_GameTraceChannel15;
constexpr ECollisionChannel DEDICATED_INTERACT_CHANNEL_ALT14 = ECollisionChannel::ECC_GameTraceChannel14;


/** @brief Tick trace types, Radial, Radial-trace, Shape-trace along lines,(more to  etc.  */
UENUM(Blueprintable, BlueprintType)
enum class EPDTickTraceType : uint8
{
	TRACE_RADIAL     UMETA(DisplayName="Radial trace"),           /**< @brief Radial-trace */
	TRACE_LINESHAPE  UMETA(DisplayName="Shape trace along line"), /**< @brief Shape-line-trace */ 
	TRACE_RESERVED0  UMETA(DisplayName="RESERVED0"),              /**< @brief Reserved type */ 
	TRACE_RESERVED1  UMETA(DisplayName="RESERVED1"),              /**< @brief Reserved type */ 
	TRACE_RESERVED2  UMETA(DisplayName="RESERVED2"),              /**< @brief Reserved type */ 
	TRACE_MAX        UMETA(DisplayName="All trace types"),        /**< @brief All trace types */
};

/** @brief Interaction results enum: INTERACT_[SUCCESS, FAIL, DELAYED, UNHANDLED] */
UENUM(Blueprintable, BlueprintType)
enum class EPDInteractResult : uint8
{
	INTERACT_SUCCESS   UMETA(DisplayName="Succeeded"),  /**< @brief Interaction attempt succeeded */
	INTERACT_FAIL      UMETA(DisplayName="Failed"),     /**< @brief Interaction attempt failed */ 
	INTERACT_DELAYED   UMETA(DisplayName="Delayed"),    /**< @brief Interaction attempt delayed. Possibly networked call */
	INTERACT_UNHANDLED UMETA(DisplayName="Unhandled"),  /**< @brief Interaction attempt delayed. Possibly networked call */
};

/** @brief Trace results enum: TRACE_[SUCCESS, FAIL] */
UENUM(Blueprintable, BlueprintType)
enum class EPDTraceResult : uint8 
{
	TRACE_SUCCESS UMETA(DisplayName="Succeeded"),  /**< @brief Trace succeeded at finding an interactable */
	TRACE_FAIL    UMETA(DisplayName="Failed"),     /**< @brief Trace failed at finding an interactable  */ 
};

/** @brief Key actions enum: [KTAP, KHOLD]. Used for interaction messages  */
UENUM(Blueprintable, BlueprintType)
enum class EPDKeyAction : uint8 
{
	KTAP UMETA(DisplayName="Tap"),   /**< @brief Tapping key action */    
	KHOLD UMETA(DisplayName="Hold"), /**< @brief Holding key action */ 
};

/** @brief Key modifier enum: [KCTRL L/R, KALT L/R, KSHIFT L/R]. Used for interaction messages  */
UENUM(Blueprintable, BlueprintType)
enum class EPDKeyModifiers : uint8 
{
	KCTRLR UMETA(DisplayName="ControlR"),  /**< @brief Modifier key: CTRL R */
	KCTRLL UMETA(DisplayName="ControlL"),  /**< @brief Modifier key: CTRL L */
	
	KALTR UMETA(DisplayName="AltR"),   /**< @brief Modifier key: ALT R */
	KALTL UMETA(DisplayName="AltL"),   /**< @brief Modifier key: ALT L */
	
	KSHIFTR UMETA(DisplayName="ShiftR"), /**< @brief Modifier key: SHIFT R */
	KSHIFTL UMETA(DisplayName="ShiftL"), /**< @brief Modifier key: SHIFT L */
};

/** @brief Key string builder. Used for interaction messages */
namespace EPDKeys
{
	/** @brief Translate Key action enum to string */
	static inline FString ToString(EPDKeyAction InAction)
	{
		switch (InAction)
		{
		case EPDKeyAction::KTAP:
			return "Press";
		case EPDKeyAction::KHOLD:
			return "Hold";
		}
		return "";
	}

	/** @brief Translate Key modifier enum to string */
	static inline FString ToString(EPDKeyModifiers InMod)
	{
		switch (InMod) {
		case EPDKeyModifiers::KCTRLR:
			return "Ctrl(R)";
		case EPDKeyModifiers::KCTRLL:
			return "Ctrl(L)";
		case EPDKeyModifiers::KALTR:
			return "Alt(R)";
		case EPDKeyModifiers::KALTL:
			return "Alt(L)";
		case EPDKeyModifiers::KSHIFTR:
			return "Shift(R)";
		case EPDKeyModifiers::KSHIFTL:
			return "Shift(L)";
		}
		return "";
	}	
};

/** @brief Key combination compound. Used for interaction messages */
USTRUCT(Blueprintable, BlueprintType)
struct PDINTERACTION_API FPDKeyCombination
{
	GENERATED_BODY()

	/** @brief Translate Key combination data to string format */
	FString ConvertToString();
	
	/** @brief Main key used for the interaction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKey MainKey{};
	/** @brief Optional key modifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EPDKeyModifiers> OptionalModifiers{};
	/** @brief Related key action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDKeyAction KeyAction = EPDKeyAction::KTAP;
	/** @brief String that accumulates the strings of translated modifiers */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FString AccumulatedString{};
	
};

/** @brief Interaction message compound. For use with user interfaces */
USTRUCT(Blueprintable, BlueprintType)
struct PDINTERACTION_API FPDInteractMessage
{
	GENERATED_BODY()

	/** @brief Interactable actor display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ActorName;
	/** @brief Key combination (meta) data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPDKeyCombination KeyActionMetadata;
	/** @brief Game action to describe the interaction itself */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString GameAction;
};

/**
 * @brief This is the structure used to dispatch interactions, the parameters passed into an OnInteract.
 * @note This structure is constructed by the interaction component and passed into an 'interactable' item at the point of interaction
 */
USTRUCT(Blueprintable, BlueprintType)
struct PDINTERACTION_API FPDInteractionParams
{
	GENERATED_BODY()

	FPDInteractionParams() = default;
	explicit FPDInteractionParams(double _InteractionPercent, AActor* _InstigatorActor, TSubclassOf<UActorComponent> _InstigatorComponentClass, const TSet<FGameplayTag>& _OptionalInteractionTags)
		: InteractionPercent(_InteractionPercent), InstigatorActor(_InstigatorActor), InstigatorComponentClass(_InstigatorComponentClass),  OptionalInteractionTags(_OptionalInteractionTags) {}

	/** @brief Will contain a normalized value representing progress [0.0, 1.0]*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Params")
	double InteractionPercent = 0.0;
	
	/** @brief Passing in the instigator, for some interactables this will be needed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Params")
	AActor* InstigatorActor = nullptr;

	/** @brief Passing in the instigator, for some interactables this will be needed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Params")
	TSubclassOf<UActorComponent> InstigatorComponentClass = nullptr;

	/** @brief Passing in the instigator fragment */
    FMassEntityHandle InstigatorEntity{}; 
	
	/** @brief A set of optional tags to be handled as seen fit by game module implementations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Params")
	TSet<FGameplayTag> OptionalInteractionTags{};
};

/**
 * @brief A subclass of interaction parameters which adds a custom processing delegate for allowing processing logic to be overriden for any interactables, use with care 
 */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FPDCustomProcessInteraction, const FPDInteractionParams&, InteractionParams, EPDInteractResult&, InteractResult);
USTRUCT(Blueprintable, BlueprintType)
struct FPDInteractionParamsWithCustomHandling : public FPDInteractionParams
{
	GENERATED_BODY()
	
	// @todo Handle callback on delayed events soon
	/** @brief Custom processing delegate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Params")
	FPDCustomProcessInteraction CustomInteractionProcessor;
};

/**
 * @brief This structure is meant to be placed in a interactable datatable or in an interactable base-actor
 */
USTRUCT(Blueprintable, BlueprintType) 
struct FPDInteractionSettings : public FTableRowBase
{
	GENERATED_BODY()

	FPDInteractionSettings() = default;
	explicit FPDInteractionSettings(double _InteractionTimeInSeconds) : InteractionTimeInSeconds(_InteractionTimeInSeconds){}
	
	/** @brief Represents the full interaction time 
	 * @details How much time does it take to interact with this resource. defaults to instant interaction */	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings")
	double InteractionTimeInSeconds = 0.0;

	/** @brief Time (in seconds) the interact cooldown persísts after successfully interacting with the object */		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (AllowPrivateAccess="true"))
	double RefreshInterval = 20.0;
	
	// @todo might need more settings
};

/**
 * @brief This structure encapsulated some settings related to interaction tracing
 */
USTRUCT(BlueprintType, Blueprintable)
struct FPDTraceTickSettings
{
	GENERATED_BODY()

public:
	void Setup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName TraceNameID;

	/** @brief Which type of trace will we perform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPDTickTraceType TickTraceType = EPDTickTraceType::TRACE_RADIAL;

	/** @brief Needs to be large enough to hit objects of differing 'per-object' interaction distance limit*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxTraceDistanceInUnrealUnits = DEFAULT_TRACER_MAX_INTERACTION_DISTANCE; 	

	/** @brief Value is interpreted as per seconds. 0.0 will defaults to 'per-frame' for traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double TickInterval  = 0.0;

	/** @brief THe collision channel we want to trace against */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ECollisionChannel> TraceChannel = DEDICATED_INTERACT_CHANNEL_ALT18;

	/** @brief Is generated from the collision channel using 'UEngineTypes::ConvertToObjectType' */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TEnumAsByte<EObjectTypeQuery> GeneratedObjectType = EObjectTypeQuery::ObjectTypeQuery_MAX;
};


/**
 * @brief This structure encapsulated some settings related to interaction tracing
 */
USTRUCT(BlueprintType, Blueprintable)
struct FPDTraceSettings : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** @brief Which type of trace will we perform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText TickSetName = FText::AsCultureInvariant(TEXT("NA"));

	/** @brief This contains settings for different trace types, as 'TickTraceType' can change we just change which settings we are reading from here */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPDTraceTickSettings> TickTraceTypeSettings;
};

/**
 * @brief This structure contains trace results for the interaction system. 
 */
USTRUCT(BlueprintType)
struct FPDTraceResult
{
	GENERATED_USTRUCT_BODY()

public:
	/** @brief Result enum flag  @note Not needed in c++ but for representing code paths in blueprints it is a bit cleaner */
	UPROPERTY(BlueprintReadOnly)
	EPDTraceResult ResultFlag = EPDTraceResult::TRACE_FAIL;

	/** @brief Result hit result structure */
	UPROPERTY(BlueprintReadOnly)
	FHitResult HitResult;

	/** @brief The trace type that this result was created for */
	UPROPERTY(BlueprintReadOnly)
	EPDTickTraceType TraceType;
	
	/** @brief Equality comparison, compares the resultflags and the hitresults */
	bool operator==(const FPDTraceResult& Other) const
	{
		return ResultFlag == Other.ResultFlag
		&& HitResult.GetActor() == Other.HitResult.GetActor()
		&& HitResult.Location.Equals(Other.HitResult.Location)
		&& TraceType == Other.TraceType;
	}
};

/** @brief How many trace frames the interaction component can accumulate it its trace buffer */
constexpr int32 FRAMERESET_LIMIT = 10;

/**
 * @brief The Trace buffer structure. Keeps buffers + controls for managing the trace buffer
 * @details Keeps buffers for both radial and shape traces + controls for managing the trace buffer
 */
USTRUCT(BlueprintType)
struct FPDTraceBuffer
{
	GENERATED_USTRUCT_BODY()

public:
	/** @brief Adds an initial invalid trace frame. Used as a state check downstream */
	void Setup();
	/** @brief Get last trace result */
	const FPDTraceResult& GetLastTraceResult() const;
	/** @brief Get last trace result with a valid hit result, if any exists in the buffer */
	const FPDTraceResult& GetLastValidResult() const;
	/** @brief Check if there are any valid trace results */
	bool HasValidResults() const;
	/** @brief Clear all trace results */
	void ClearTraceResults();
	/** @brief Adds a new trace frame to the buffer */
	void AddTraceFrame(EPDTraceResult TraceResult, const FHitResult& HitResult, EPDTickTraceType TraceType);
	
	/** @brief Radially traced actors */
	UPROPERTY()
	TArray<AActor*> RadialTraceActors{};

	/** @brief Tick time interval for the radial trace */
	double RadialTraceTickTime = 0.0;
	/** @brief Tick time interval for the shaped-line trace  */
	double LineTraceTickTime = 0.0;

	/** @brief Frame results */
	TDeque<FPDTraceResult> Frames{};
	/** @brief Iteration step, to help keep track when we should wrapping the buffer */
	int32 ValidFrameResetIt = 1;
	/** @brief Cached valid frame */
	FPDTraceResult CachedValidFrame{};
};

/**
 * @brief Structure used by the save-game structure. Holds data regarding world interactables.
 */
USTRUCT(BlueprintType, Blueprintable)
struct PDINTERACTION_API FRTSSavedInteractable
{
	GENERATED_BODY()

	/** @brief Actor world representation of this interactable */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite)
	AActor* ActorInWorld = nullptr; 
	
	/** @brief Class of interactable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<AActor> ActorClass; 
	
	/** @brief Location of the interactable at the moment of saving */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location;

	/** @brief Reserved Usability 'stat'. @todo consider removing and replacing in gamemodule class, where a progression system could be hooked into here to give an extended stat-list */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance|Widgets")
	double Usability = 0.0;

	/** @brief Instance ID is used to compare old vs new data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance|Widgets")
	int32 InstanceIndex = INDEX_NONE;

	/** @brief Hidden uclass pointer so the save editor may update the softclass field when we need it to */
	UPROPERTY()
	UClass* _HiddenInstantiatedClass = nullptr;

	/** @brief Function to update the softclass field with _HiddenInstantiatedClass,
	 * @note if _HiddenInstantiatedClass is nullptr we may optinoally override it as-well */
	void CopySelectedToSoftClass(UClass* PotentialOverrideHiddenClass = nullptr)
	{
		if (PotentialOverrideHiddenClass != nullptr || _HiddenInstantiatedClass == nullptr)
		{
			_HiddenInstantiatedClass = PotentialOverrideHiddenClass;
		}
		
		if (_HiddenInstantiatedClass != nullptr)
		{
			if (TSoftClassPtr<AActor>(_HiddenInstantiatedClass) != ActorClass)
			{
				ActorClass = TSoftClassPtr<AActor>(_HiddenInstantiatedClass);
			}
			_HiddenInstantiatedClass = nullptr; // Clear after attempting to copy to a softclass ptr
		}		
	}
	
	/** @brief Equality comparison, compares the actor class, the instanced ID/index
	 * alongside a slacked comparison of usability and location */
	bool operator==(const FRTSSavedInteractable& Other) const
	{
		return 
			this->ActorClass == Other.ActorClass
			&& this->InstanceIndex == Other.InstanceIndex
			&& FMath::IsNearlyEqual(this->Usability, Other.Usability, 0.5f) 
			&& (this->Location - Other.Location).IsNearlyZero(5.f);
	}

	/** @brief Inequality comparison, negates the result of the Equality comparison */
	bool operator!=(const FRTSSavedInteractable& Other) const
	{
		return (*this == Other) == false;
	}
};

/** @brief System Behaviours - Resource Availability */
UENUM()
enum class ERTSResourceAvailability : uint8
{
	EInfinitelyAvailable,
	EInventoryFragment,
	EUndefined,
};

/** @brief System Behaviours - Resource Action Requirements, for interactable resource containers */
UENUM()
enum class ERTSResourceRequirement : uint8
{
	EMustHaveMinimumCount,
	EAlwaysAllowTrade,
	EUndefined,
};

/** @brief System Behaviours - namespace aliases */
namespace PD::Interactable::Behaviour
{
	using Availability = ERTSResourceAvailability;
	using Requirements = ERTSResourceRequirement;
}

/** @brief Global/Fallback resource settings */
UCLASS(Config = "Game", DefaultConfig)
class PDINTERACTION_API URTSInteractableResourceSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	
	/** @brief Default Resource Availability setting, resource interactables will fall back to this if no override value is set */
	UPROPERTY(Config, EditAnywhere, Category = "Interactable Resources Developer Settings")
	ERTSResourceAvailability DefaultAvailability = PD::Interactable::Behaviour::Availability::EInfinitelyAvailable;

	/** @brief Default Resource Action Requirements setting, resource interactables will fall back to this if no override value is set */
	UPROPERTY(Config, EditAnywhere, Category = "Interactable Resources Developer Settings")
	ERTSResourceRequirement DefaultRequirements = PD::Interactable::Behaviour::Requirements::EAlwaysAllowTrade;	
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