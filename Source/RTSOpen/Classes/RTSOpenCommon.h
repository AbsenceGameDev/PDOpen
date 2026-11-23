/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "PDInteractCommon.h"
#include "PDItemCommon.h"
#include "PDRTSCommon.h"
#include "Net/PDItemNetDatum.h"

#include "Math/Range.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/SaveGame.h"
#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"
#include "AI/Mass/PDMassFragments.h"
#include "MassEntityConfigAsset.h"

#if WITH_EDITOR
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#endif // WITH_EDITOR

#include "RTSOpenCommon.generated.h"


DECLARE_LOG_CATEGORY_CLASS(PDLog_RTSOConversation, Log, All);
DECLARE_LOG_CATEGORY_CLASS(PDLog_RTSOInteract, Log, All);
DECLARE_LOG_CATEGORY_CLASS(PDLog_RTSO, Log, All);

class APDInteractActor;
class UPDRTSBaseUnit;
class ARTSOInteractableConversationActor;

UENUM()
enum class EPDSaveDataThreadSelector : int8
{
	// Things we want to handle on different threads
	EPlayers = 0,
	EInteractables,
	EEntities,
	EInventories,
	EConversationActors,
	EPlayerConversationProgress,
	EEnd,

	// Needed for other work that does not involve threads, felt unnecessary to keep in another enum when this enum ha all the other fields we are looking for
	EWorldBaseData = -1
};



/**
 * @todo @note: If I have time then write a small mission system, but avoid putting to much time into it
 * @todo @cont: as I am working on a separate repo as-well with a full fledged mission system,
 * @todo @cont: so not worth making two fully fledged ones,
 * @todo @cont: this one well have to be made with some shortcuts in design and implementation
 */
UENUM()
enum ERTSOConversationState
{
	CurrentStateActive    UMETA(DisplayName="Conversation: Current State Active"),
	CurrentStateInactive  UMETA(DisplayName="Conversation: Current State Inactive"),
	CurrentStateCompleted UMETA(DisplayName="Conversation: Current State Completed"),
	Invalid               UMETA(DisplayName="Conversation: Current State INVALID"),
	Valid                 UMETA(DisplayName="Conversation: Current State VALID"),
};

/** @brief Conversation rules, used for mission-progression */
USTRUCT(Blueprintable)
struct FRTSOConversationRules
{
	GENERATED_BODY()
	
	/** @brief Tag for entry data that these rules pertain */
	UPROPERTY(EditAnywhere)
	FGameplayTag EntryTag;
	
	/** @brief StartingState and ActiveState for this conversation instance */
	UPROPERTY(EditAnywhere)
	TEnumAsByte<ERTSOConversationState> State = ERTSOConversationState::CurrentStateInactive;

	/** @brief Tags required for this instance to be loaded */
	UPROPERTY(EditAnywhere)
	TArray<FGameplayTag> RequiredTags;

	/** @brief Flag to tell if this conversation is repeatable*/
	UPROPERTY(EditAnywhere)
	bool bCanRepeatConversation = true;

	bool operator==(const FRTSOConversationRules& Other) const
	{
		// we really only care about the entry tag and required tags in comparisons 
		return
			this->EntryTag == Other.EntryTag
			&& this->RequiredTags == Other.RequiredTags;
	}

	bool operator!=(const FRTSOConversationRules& Other) const
	{
		return (*this == Other) == false;
	}
};

/** @brief Conversation base Settings.  */
USTRUCT(Blueprintable)
struct FRTSOConversationMetaProgressionDatum : public FTableRowBase
{
	GENERATED_BODY()

	/**< @brief Starting progression */
	UPROPERTY(EditAnywhere)
	FGameplayTag MissionTag = FGameplayTag{};	
	
	/**< @brief Starting progression */
	UPROPERTY(EditAnywhere)
	int32 BaseProgression = 0;
	
	/** @brief Required tags to progress through the mission phases */
	UPROPERTY(EditAnywhere)
	TArray<FRTSOConversationRules> PhaseRequiredTags;

	bool operator==(const FRTSOConversationMetaProgressionDatum& Other) const
	{
		return 
			this->MissionTag == Other.MissionTag
			&& this->BaseProgression == Other.BaseProgression
			&& this->PhaseRequiredTags == Other.PhaseRequiredTags;
	}

	bool operator!=(const FRTSOConversationMetaProgressionDatum& Other) const
	{
		return (*this == Other) == false;
	}
};


/** @brief Conversation meta progression wrapper, @note this is nasty @todo actually plan design for this after having the initial prototypes finished. */
USTRUCT(Blueprintable)
struct FRTSOConversationMetaProgressionListWrapper
{
	GENERATED_BODY()

	/** @brief Map of progression datum per mission tag */
	UPROPERTY(EditAnywhere)
	TMap<FGameplayTag, FRTSOConversationMetaProgressionDatum> ProgressionDataMap;
	
	/** @brief Equality comparison, slow */
	bool operator==(const FRTSOConversationMetaProgressionListWrapper& Other) const
	{
		TArray<FGameplayTag> ThisMetaProgressionKeyArray;
		TArray<FGameplayTag> OtherMetaProgressionKeyArray;
		this->ProgressionDataMap.GenerateKeyArray(ThisMetaProgressionKeyArray);
		Other.ProgressionDataMap.GenerateKeyArray(OtherMetaProgressionKeyArray);

		TArray<FRTSOConversationMetaProgressionDatum> ThisMetaProgressionValueArray;
		TArray<FRTSOConversationMetaProgressionDatum> OtherMetaProgressionValueArray;
		this->ProgressionDataMap.GenerateValueArray(ThisMetaProgressionValueArray);
		Other.ProgressionDataMap.GenerateValueArray(OtherMetaProgressionValueArray);	
		
		return 
			ThisMetaProgressionKeyArray == OtherMetaProgressionKeyArray
			&& ThisMetaProgressionValueArray == OtherMetaProgressionValueArray;		
	}

	/** @brief Inequality comparison, slow */
	bool operator!=(const FRTSOConversationMetaProgressionListWrapper& Other) const
	{
		return (*this == Other) == false;
	}
};


/** @brief Save data for inventory items */
USTRUCT(BlueprintType, Blueprintable)
struct FRTSSavedItems
{
	GENERATED_BODY()

	/** @brief Saved items, @todo need to sort this by user/ownerID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPDItemNetDatum> Items;
};

/** @brief Save data for conversation progression actors and players conversation progressions  */
USTRUCT(BlueprintType, Blueprintable)
struct FRTSSavedConversationActorData
{
	GENERATED_BODY()
	
	/** @brief Missions to load from table*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<FName> Missions{};

	/** @brief Location of conversation progression actor it belongs to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|SaveGame|Unit")
	FVector Location{}; 

	/** @brief Actor class of the interactable conversation actor*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|SaveGame|Unit")
	TSoftClassPtr<ARTSOInteractableConversationActor> ActorClassType{};

	/** @brief Progression map, keyed by user (ID) and valued by progression level for the conversation.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<int32/*ActorID*/, FRTSOConversationMetaProgressionListWrapper/*ProgressionLevel*/> ProgressionPerPlayer; 

	/** @brief Unused. assume it will always be 1.0 for now	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|SaveGame|Unit")
	double Health = 1.0;

	UPROPERTY()
	UClass* _HiddenInstantiatedClass = nullptr;

	// Copy selected class (_HiddenInstantiatedClass) to softobject if not already copied, clear _HiddenInstantiatedClass no matter if it was copied or not 
	void CopySelectedToSoftClass(UClass* PotentialOverrideHiddenClass = nullptr)
	{
		if (PotentialOverrideHiddenClass != nullptr || _HiddenInstantiatedClass == nullptr)
		{
			_HiddenInstantiatedClass = PotentialOverrideHiddenClass;
		}
		
		if (_HiddenInstantiatedClass != nullptr)
		{
			if (TSoftClassPtr<ARTSOInteractableConversationActor>(_HiddenInstantiatedClass) != ActorClassType)
			{
				ActorClassType = TSoftClassPtr<ARTSOInteractableConversationActor>(_HiddenInstantiatedClass);
			}
			_HiddenInstantiatedClass = nullptr;
		}		
	}
	
	/** @brief Equality comparison, slow */
	bool operator==(const FRTSSavedConversationActorData& Other) const
	{
		TArray<int32> ThisProgressionKeyArray;
		TArray<int32> OtherProgressionKeyArray;
		this->ProgressionPerPlayer.GenerateKeyArray(ThisProgressionKeyArray);
		Other.ProgressionPerPlayer.GenerateKeyArray(OtherProgressionKeyArray);

		TArray<FRTSOConversationMetaProgressionListWrapper> ThisProgressionValueArray;
		TArray<FRTSOConversationMetaProgressionListWrapper> OtherProgressionValueArray;
		this->ProgressionPerPlayer.GenerateValueArray(ThisProgressionValueArray);
		Other.ProgressionPerPlayer.GenerateValueArray(OtherProgressionValueArray);		

		// Give a fair amount of slack on the floating points
		return 
			this->ActorClassType == Other.ActorClassType
			&& ThisProgressionKeyArray == OtherProgressionKeyArray
			&& ThisProgressionValueArray == OtherProgressionValueArray
			&& this->Missions.Array() == Other.Missions.Array()
			&& FMath::IsNearlyEqual(this->Health, Other.Health, 0.5f) 
			&& (this->Location - Other.Location).IsNearlyZero(5.f);
	}

	/** @brief Inequality comparison, slow */
	bool operator!=(const FRTSSavedConversationActorData& Other) const
	{
		return (*this == Other) == false;
	}	
	
};

/** @brief Save data for world units/entities*/
USTRUCT(BlueprintType, Blueprintable)
struct FRTSSavedWorldUnits
{
	GENERATED_BODY()

	/** @brief Saved current action for the unit this relates to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|SaveGame|Unit")
	FPDMFragment_Action CurrentAction{};

	/** @brief Saved current location for the unit this relates to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|SaveGame|Unit")
	FVector Location{};

	/** @brief Saved current health for the unit this relates to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|SaveGame|Unit")
	double Health = 0.0;

	/** @brief Saved OwnerID for the unit this relates to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|SaveGame|Unit")
	int32 OwnerID = INDEX_NONE;

	/** @brief Saved selection index the unit this relates to is contained within */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|SaveGame|Unit")
	int32 SelectionIndex = INDEX_NONE;

	/** @brief Tag related to this entity, @done use to load entity config from table  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data|SaveGame|Unit")
	FGameplayTag EntityUnitTag; //

	/** @brief Instance ID is used to compare old vs new data */
	UPROPERTY(EditAnywhere, Category = "GameInstance|Widgets")
	FMassEntityHandle InstanceIndex = {INDEX_NONE, INDEX_NONE};

	/** @brief Equality comparison, has certain slack in the health and location comparisons */
	bool operator==(const FRTSSavedWorldUnits& Other) const
	{
		return 
			this->CurrentAction.Reward == Other.CurrentAction.Reward
			&& this->CurrentAction.RewardAmount == Other.CurrentAction.RewardAmount
			&& this->CurrentAction.ActionTag == Other.CurrentAction.ActionTag
			&& this->CurrentAction.OptTargets.IsValidCompound() == Other.CurrentAction.OptTargets.IsValidCompound()
			&& this->InstanceIndex == Other.InstanceIndex
			&& this->EntityUnitTag == Other.EntityUnitTag
			&& FMath::IsNearlyEqual(this->Health, Other.Health, 0.5f) 
			&& (this->Location - Other.Location).IsNearlyZero(5.f);
	}

	/** @brief Inequality comparison, negates the result of the equality comparison */
	bool operator!=(const FRTSSavedWorldUnits& Other) const
	{
		return (*this == Other) == false;
	}	
};

USTRUCT(BlueprintType, Blueprintable)
struct RTSOPEN_API FRTSSaveData
{
	GENERATED_BODY()
	/** @brief Random stream */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance|Widgets")
	FRandomStream Seeder{0};
	
	/** @brief Total elapsed game-time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance|Widgets")
	double GameTime = 0.0;

	/** @brief Players locations, keyed by players persistent IDs*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance|Widgets")
	TMap<int32 /*Player Persistent ID*/, FVector> PlayerLocations{}; 

	/** @brief World interactables and their parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance|Widgets")
	TArray<FRTSSavedInteractable> Interactables{};

	/** @brief World entities and their parameters, such as ownerID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance|Widgets")
	TArray<FRTSSavedWorldUnits> EntityUnits{};
	
	/** @brief userid tied to some account id?, in singleplayer keep only one entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance|Widgets")
	TMap<int32, FRTSSavedItems> Inventories{};

	/** @brief Save data for the existing conversation actors */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance|Widgets")
	TMap<int32 /*ConversationActorID*/, FRTSSavedConversationActorData> ConversationActorState{};

	/** @brief userid tied to some account id?, in singleplayer keep only one entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int32 /*PlayerID*/, FGameplayTagContainer> PlayersAndConversationTags{};

	/** @brief Timer Handle, save is dispatched on a timer to throttle any saves */
	FTimerHandle SaveThrottleHandle;
	
};


/** @brief Main save data structure, @todo associate account ID to the persistent ID */
UCLASS(BlueprintType, Blueprintable)
class RTSOPEN_API URTSOpenSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	/** @brief Data struct */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance|Widgets")
	FRTSSaveData Data;
};

/** @brief @todo move to new file in UI folder */
UCLASS(BlueprintType, Blueprintable)
class UPDTransitionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** @brief Starts a transition animation */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void StartTransition();
	/** @brief Ends the transition animation */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void EndTransition();
	
public:
	/** @brief The given transition animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget|BaseTransition", Meta = (BindWidget))
	UWidgetAnimation* TransitionAnimation = nullptr;
};

/** @brief @todo move to new file in UI folder */
UCLASS(BlueprintType, Blueprintable)
class URTSHudWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
};

/** @brief @todo remove, redundant as there are already widgets in the interaction plugin afaik */
UCLASS(BlueprintType, Blueprintable)
class URTSInteractWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/** @brief @todo see note above */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget|Base", Meta = (BindWidget))
	UCommonButtonBase* InteractButton = nullptr;
};

/** @brief Data-asset that holds any uasset we want to default to a given godhand settings entry */
UCLASS(BlueprintType, Blueprintable)
class URTSGodhandDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** @brief Curve to source magnification value from. is none exist it reverts to a expression based logarithmic 'curve' */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Pawn|Cursor|Settings")
	UCurveFloat* MagnificationCurve = nullptr;
};

/** @brief Settings table row for the godhand pawn */
USTRUCT(Blueprintable, BlueprintType)
struct FRTSGodhandSettings : public FTableRowBase
{
	GENERATED_BODY()

	/** @brief Dataasset that holds uassets we want to default to this godhand settings entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Godhand|Settings")
	URTSGodhandDataAsset* AssetData = nullptr;
	/** @brief Padding to offset a scalar range the is fed to a sine. This is used when the cursor hovers an actor or entity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Godhand|Settings")
	double TargetPadding = 40.0;
	/** @brief Scalar phase speed, how fast does it ride the sine-wave? This is used when the cursor hovers an actor or entity  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Godhand|Settings")
	double CursorSelectedScalePhaseSpeed = 4.0;	
	/** @brief  Sine intensity. This is used when the cursor hovers an actor or entity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Godhand|Settings")
	double CursorSelectedSinScaleIntensity = 0.3;
	/** @brief This controls how fast the cursor rescales when snapping to a hovered actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Godhand|Settings")
	double SelectionRescaleSpeed = 8.0;
	/** @brief This controls how fast the godhand rotates when using the rotate keys */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Godhand|Settings")
	double RotationRateModifier = 75.0;
	/** @brief This controls how fast the godhand moves to a new buildable camera target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Godhand|Settings")
	double CameraTargetInterpSpeed = 3.0;	
};

/** @brief State struct for the godhand pawn */
USTRUCT(BlueprintType, Blueprintable)
struct FRTSGodhandState
{
	GENERATED_BODY()
	
	/** @brief The currently hovered actor, it looks for objects that inherit from FPDInteractInterface */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Godhand|State")
	AActor* HoveredActor = nullptr;

	/** @brief The worlds agent handler gets assigned to this. This ISM component has logic to handle spawning mass entities, dispatching jobs and such  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Godhand|State")
	UPDRTSBaseUnit* ISMAgentComponent = nullptr;	

	/** @brief This value is calculated based on the current MagnificationStrength */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Godhand|State")
	double MagnificationValue = 0.0;

	/** @brief This value is assigned from the inputvalue of the 'ActionMagnify' input action */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Godhand|State")
	double MagnificationStrength = 0.01;	

	/** @brief This value is assigned from the inputvalue of the 'ActionMagnify' input action, in case we are rotating a held object */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Godhand|State")
	double ScrollAccumulator = 0.0;	

	/* State(s) - Cursor */	
	/** @brief Accumulates the current time elapsed and feeds a sinewave with it */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Godhand|State")
	double AccumulatedPlayTime = 0.0;
	/** @brief The calculated target transform for the godhand,  be it a hovered entity/actor or the mouse projection location */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Godhand|State")
	FTransform TargetTransform{};
	
	/* State(s) - Target */	
	/** @brief Virtually unused @todo Remove within a commit or two, this has lost it's purpose at this point  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Godhand|State")
	FVector WorkUnitTargetLocation{};

	/** @brief Copies the pointer from 'HoveredActor' at the point of triggering the 'ActionWorkerUnit' input action  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Godhand|State")
	AActor* WorkerUnitActionTarget;
	
	/** @brief Currently selected, singular, mass entity. In case we have multiple selected this is often set to an invalid handle as that is handled via selection groups instead */
	UPROPERTY()
	FMassEntityHandle SelectedWorkerUnitHandle{0, 0};
	
	/* State(s) - Pathing */	
	/** @brief This flag is set when we start drawing our niagara path, and is unset when we stop. Used to know if it's safe to overwrite certain properties or not */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Godhand|State")
	bool bUpdatePathOnTick = false;
	
	/** @brief The generated path points for the niagara effect */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Godhand|State")
	TArray<FVector> PathPoints;
	
	/** @brief Rotation direction deque, it increases the head is the current ratation operation,
	 * @note rotate ccw if Element == 1
	 * @note rotate cw  if Element == -1
	 * 
	 * @note A. Get if positive or negative,
	 * @note B. Add Direction to queue
	 * @note C. In tick: rotate +-90 degree in yaw with interpolation
	 * @note D. If rotating direction Positive, then pressing positive again then negative, , final two should cancel eachother out
	 */
	TDeque<TPair<int8, double /*ActualRotationTarget*/>> RotationDeque{};

	/** @brief Amount degrees left of rotation,
	 * @note this has a tendency to 'slide' into an offset when tick is not reliable,
	 * @note fixed it by snapping it to cardinal directions so it never accumulates this offset */
	double CurrentRotationLeft = 0.0;
	
	/** @brief Flag used to avoid overwriting the rotation states for a active rotation */
	bool bIsInRotation = false;
};

//
// Move everything below this to a file called RTSOpenSettingsCommon

/** @brief Value types we should allow for settings  */
namespace PD::Settings
{
	enum class BaseType : uint8
	{
		Boolean = 0, // 'bool' or 'uint8 : 1'
		FloatSelector,   // 'double' or 'float'
		FloatSlider,   // 'double' or 'float'
		IntegerSelector, // 'int32' or 'int64'
		IntegerSlider, // 'int32' or 'int64'
		BaseTypeLim,
	};
	enum class VectorType : uint8
	{
		Vector2 = BaseType::BaseTypeLim,  // 'FVector2D' 
		Vector3,  // 'FVector'
		Colour,  // 'FColor'
		VectorTypeLim,		
	};
	enum class OtherType : uint8
	{
		String = VectorType::VectorTypeLim, // 'FString'
		EnumAsByte, // 'Byte'
		Key,     // 'FKey'
		OtherTypeLim,		
	};
}

UENUM()
enum class ERTSOSettingsType : uint8
{
	Boolean, // 'bool' or 'uint8 : 1'
	FloatSelector,   // 'double' or 'float'
	FloatSlider,   // 'double' or 'float'
	IntegerSelector, // 'int32' or 'int64'
	IntegerSlider, // 'int32' or 'int64'
	Vector2,  // 'FVector2D' 
	Vector3,  // 'FVector'
	Colour,  // 'FColor'
	String,  // 'FString'
	EnumAsByte, // 'Byte'
	Key,     // 'FKey'
	None,     // 'N/A'
};

UENUM()
enum class ERTSOResolutionMode : uint8
{
	Fullscreen,
	Windowed,
	Borderless,
};

UENUM()
enum class ERTSOSettingsKeySource : uint8
{
	KBM,
	GGP,
	DS45,
	XSX,
};

USTRUCT(Blueprintable)
struct RTSOPEN_API FRTSOSettingsKeyData
{
	GENERATED_BODY()

	FRTSOSettingsKeyData(){}
	FRTSOSettingsKeyData(FGuid IdToMatchAgainst, FKey InMain_KeyBoardMouse);
	FRTSOSettingsKeyData(FGuid IdToMatchAgainst, FKey InMain_KeyBoardMouse, FKey InMain_GenericGamepad, FKey InMain_DS45, FKey InMain_XSX);	
	FRTSOSettingsKeyData(FGuid IdToMatchAgainst, FKey InMain_KeyBoardMouse, FKey InMain_GenericGamepad, FKey InMain_DS45, FKey InMain_XSX, FKey InAlt_KeyBoardMouse, FKey InAlt_GenericGamepad, FKey InAlt_DS45, FKey InAlt_XSX);
	FRTSOSettingsKeyData(FKey InMain_KeyBoardMouse);
	FRTSOSettingsKeyData(FKey InMain_KeyBoardMouse, FKey InMain_GenericGamepad, FKey InMain_DS45, FKey InMain_XSX);	
	FRTSOSettingsKeyData(FKey InMain_KeyBoardMouse, FKey InMain_GenericGamepad, FKey InMain_DS45, FKey InMain_XSX, FKey InAlt_KeyBoardMouse, FKey InAlt_GenericGamepad, FKey InAlt_DS45, FKey InAlt_XSX);
	template<ERTSOSettingsKeySource TKeySource = ERTSOSettingsKeySource::KBM>
	FRTSOSettingsKeyData(FKey InMain, FKey InAlt)
	{
		if constexpr (ERTSOSettingsKeySource::KBM == TKeySource)
		{
			Main_KeyBoardMouse = InMain;
			Alt_KeyBoardMouse = InAlt;
		}
		else if constexpr (ERTSOSettingsKeySource::GGP == TKeySource)
		{
			Main_GenericGamepad = InMain;
			Alt_GenericGamepad = InAlt;
		}
		else if constexpr (ERTSOSettingsKeySource::DS45 == TKeySource)
		{
			Main_DS45 = InMain;
			Alt_DS45 = InAlt;
		}
		else if constexpr (ERTSOSettingsKeySource::XSX == TKeySource)
		{
			Main_XSX = InMain;
			Alt_XSX = InAlt;
		}
	}

	// Main keys
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKey Main_KeyBoardMouse = {};
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKey Main_GenericGamepad = {};
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKey Main_DS45 = {};
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKey Main_XSX = {};

	// Alt keys
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKey Alt_KeyBoardMouse  = {};
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKey Alt_GenericGamepad = {};
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKey Alt_DS45 = {};
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKey Alt_XSX = {};

	UPROPERTY()
	FGuid HiddenSettingIdToMatchAgainst = FGuid{};

	bool operator==(const FRTSOSettingsKeyData& Other) const
	{
		return this->HiddenSettingIdToMatchAgainst == Other.HiddenSettingIdToMatchAgainst;
	}

	bool Serialize(FArchive& Ar);
	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FRTSOSettingsKeyData& Value)
	{
		Value.Serialize(Ar);
		return Ar;		
	}
};

UCLASS(Blueprintable)
class URTSOSettingsQualityTextLabelSettings : public UDeveloperSettings 
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText LowQuality;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText MidQuality;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText HighQuality;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText EpicQuality;
};

USTRUCT()
struct RTSOPEN_API FPDSettingStatics
{
	GENERATED_BODY()
	
	static TArray<TSharedPtr<FString>> GenerateQualityStringPtrArray();
	static TArray<TSharedPtr<FString>> GenerateStringPtrArrayFromDataSelector(const FRTSOSettingsDataSelector& DataSelector);
	static bool IsUFunctionViableForBinding(UFunction* Function, FString PropertyCompareTypeName);

	static FString SelectTargetFunctionParamType(ERTSOSettingsType SettingsValueType);
	static FName GetFirstInputPropertyName(UFunction* Function);

	static FORCEINLINE PD::Settings::VectorType ToVectorType(ERTSOSettingsType ReflectedEnum)
	{
		return static_cast<PD::Settings::VectorType>(ReflectedEnum);
	}	
};

USTRUCT(Blueprintable) struct FRTSOIntegerRange { GENERATED_BODY()  UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Min = {0}; UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Max = {0};};
USTRUCT(Blueprintable) struct FRTSOFloatRange { GENERATED_BODY()  UPROPERTY(EditAnywhere, BlueprintReadWrite) double Min = {0}; UPROPERTY(EditAnywhere, BlueprintReadWrite) double  Max = {0};};
USTRUCT(Blueprintable) struct FRTSOVec2Range { GENERATED_BODY()  UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Min{0}; UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Max{0};};
USTRUCT(Blueprintable) struct FRTSOVec3Range { GENERATED_BODY()  UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Min{0}; UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Max{0};};
USTRUCT(Blueprintable) struct FRTSOColourRange { GENERATED_BODY()  UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor Min{0}; UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor Max{0};};
USTRUCT(Blueprintable) struct FRTSOEnumRange { GENERATED_BODY()  UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Min = {0}; UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Max = {0};};

template<typename TType>
using FRTSOMinMax = TTuple<TType, TType>;

USTRUCT(Blueprintable)
struct FRTSOSettingsDataSelector
{
	GENERATED_BODY()

	FRTSOSettingsDataSelector() {}

	template<typename TValueType, typename TSecondaryValueType = TValueType>
	FRTSOSettingsDataSelector(ERTSOSettingsType InValueType, TValueType ActualValue, FRTSOMinMax<TValueType> MinMaxRange = {}, TArray<TValueType> PotentialList = {}, TArray<TSecondaryValueType> AssociatedList = {})
	{
		ValueType = InValueType;

		if constexpr (TIsDerivedFrom<TValueType, bool>::Value)
		{
			AsBool = ActualValue;
		}
		else if constexpr (TIsDerivedFrom<TValueType, double>::Value || TIsDerivedFrom<TValueType, float>::Value)
		{
			AsDouble = ActualValue;
			DoubleRange.Min = MinMaxRange.Key;
			DoubleRange.Max = MinMaxRange.Value;
			DoubleList = PotentialList;
		}
		else if constexpr (TIsDerivedFrom<TValueType, int32>::Value)
		{
			AsInteger = ActualValue;
			IntegerRange.Min = MinMaxRange.Key;
			IntegerRange.Max = MinMaxRange.Value;
			IntegerList = PotentialList;
		}		
		else if constexpr (TIsDerivedFrom<TValueType, FVector2D>::Value)
		{
			AsVector2 = ActualValue;
			if (MinMaxRange.IsEmpty() == false)
			{
				Vector2Range.Min = MinMaxRange.Key;
				Vector2Range.Max = MinMaxRange.Value;
			}
			Vector2dList = PotentialList;
		}
		else if constexpr (TIsDerivedFrom<TValueType, FVector>::Value)
		{
			AsVector3 = ActualValue;
			Vector3Range.Min = MinMaxRange.Key;
			Vector3Range.Max = MinMaxRange.Value;
			Vector3dList = PotentialList;
		}
		else if constexpr (TIsDerivedFrom<TValueType, FColor>::Value)
		{
			AsColour = ActualValue;
			ColourRange.Min = MinMaxRange.Key;
			ColourRange.Max = MinMaxRange.Value;
			ColourList = PotentialList;
		}
		else if constexpr (TIsDerivedFrom<TValueType, FString>::Value)
		{
			AsString = ActualValue;
			StringList = PotentialList;
			if constexpr (TIsDerivedFrom<TValueType, TSecondaryValueType>::Value == false)
			{
				AssociatedStringDataType = GetSettingsType<TSecondaryValueType>();
				if constexpr (TIsDerivedFrom<TSecondaryValueType, double>::Value) { DoubleList = AssociatedList;}
				else if constexpr (TIsDerivedFrom<TSecondaryValueType, int32>::Value) { IntegerList = AssociatedList;}
				else if constexpr (TIsDerivedFrom<TSecondaryValueType, FVector2D>::Value) { Vector2dList = AssociatedList;}
				else if constexpr (TIsDerivedFrom<TSecondaryValueType, FVector>::Value) { Vector3dList = AssociatedList;}
				else if constexpr (TIsDerivedFrom<TSecondaryValueType, FColor>::Value) { ColourList = AssociatedList;}
				else if constexpr (TIsDerivedFrom<TSecondaryValueType, uint8>::Value) { EnumList = AssociatedList;}
			}
		}
		else if constexpr (TIsDerivedFrom<TValueType, uint8>::Value)
		{
			AsEnumByte = ActualValue;
			EnumRange.Min = MinMaxRange.Key;
			EnumRange.Max = MinMaxRange.Value;
			EnumList = PotentialList;
		}
		else if constexpr (TIsDerivedFrom<TValueType, FRTSOSettingsKeyData>::Value) { AsKey = ActualValue; }
	}

	template<typename TValueType>
	TValueType& GetRef()
	{
		if constexpr (TIsDerivedFrom<TValueType, bool>::Value) { return AsBool; }
		else if constexpr (TIsDerivedFrom<TValueType, double>::Value) { return AsDouble; }
		else if constexpr (TIsDerivedFrom<TValueType, float>::Value) { return AsDouble; }
		else if constexpr (TIsDerivedFrom<TValueType, int32>::Value) { return AsInteger; }		
		else if constexpr (TIsDerivedFrom<TValueType, FVector2D>::Value) { return AsVector2; }
		else if constexpr (TIsDerivedFrom<TValueType, FVector>::Value) { return AsVector3; }
		else if constexpr (TIsDerivedFrom<TValueType, FColor>::Value) { return AsColour; }
		else if constexpr (TIsDerivedFrom<TValueType, FString>::Value) { return AsString; }
		else if constexpr (TIsDerivedFrom<TValueType, uint8>::Value) { return AsEnumByte; }
		else if constexpr (TIsDerivedFrom<TValueType, FRTSOSettingsKeyData>::Value) { return AsKey; }
		else { static TValueType Dummy; return Dummy; }
	}


	template<typename TValueType>
	static ERTSOSettingsType GetSettingsType()
	{
		if constexpr (TIsDerivedFrom<TValueType, bool>::Value) { return ERTSOSettingsType::Boolean; }
		else if constexpr (TIsDerivedFrom<TValueType, double>::Value) { return ERTSOSettingsType::FloatSlider; }
		else if constexpr (TIsDerivedFrom<TValueType, float>::Value) { return ERTSOSettingsType::FloatSlider; }
		else if constexpr (TIsDerivedFrom<TValueType, int32>::Value) { return ERTSOSettingsType::IntegerSlider; }		
		else if constexpr (TIsDerivedFrom<TValueType, FVector2D>::Value) { return ERTSOSettingsType::Vector2; }
		else if constexpr (TIsDerivedFrom<TValueType, FVector>::Value) { return ERTSOSettingsType::Vector3; }
		else if constexpr (TIsDerivedFrom<TValueType, FColor>::Value) { return ERTSOSettingsType::Colour; }
		else if constexpr (TIsDerivedFrom<TValueType, FString>::Value) { return ERTSOSettingsType::String; }
		else if constexpr (TIsDerivedFrom<TValueType, uint8>::Value) { return ERTSOSettingsType::EnumAsByte; }
		else if constexpr (TIsDerivedFrom<TValueType, FRTSOSettingsKeyData>::Value) { return ERTSOSettingsType::Key; }
		else { return ERTSOSettingsType::None; }
	}

	template<typename TValueType>
	const TArray<TValueType>& GetOptionsList() const
	{
		if constexpr (TIsDerivedFrom<TValueType, double>::Value) { return DoubleList; }
		else if constexpr (TIsDerivedFrom<TValueType, int32>::Value) { return IntegerList; }		
		else if constexpr (TIsDerivedFrom<TValueType, FString>::Value) { return StringList; }
		else if constexpr (TIsDerivedFrom<TValueType, uint8>::Value) { return EnumList; }
		else { static TArray<TValueType> Dummy; return Dummy; }
	}	

	template<typename TValueType>
	const TArray<TValueType>* GetAssociatedStringDataList() const
	{
		return static_cast<const TArray<TValueType>*>(GetAssociatedStringDataList()); 		
	}		

	void const* GetAssociatedStringDataList() const
	{
		switch (AssociatedStringDataType)
		{
		case ERTSOSettingsType::FloatSlider: return &DoubleList;
		case ERTSOSettingsType::IntegerSlider: return &IntegerList;
		case ERTSOSettingsType::Vector2: return &Vector2dList;
		case ERTSOSettingsType::Vector3: return &Vector3dList;
		case ERTSOSettingsType::Colour: return &ColourList;
		case ERTSOSettingsType::String: return &StringList;
		case ERTSOSettingsType::EnumAsByte: return &EnumList;
		// case ERTSOSettingsType::Boolean: return nullptr;
		// case ERTSOSettingsType::Key: return nullptr;
		default: return nullptr;
		}
	}			
	
	void* GetRawAdress()
	{
		switch (ValueType)
		{
		case ERTSOSettingsType::Boolean: return &AsBool;
		case ERTSOSettingsType::FloatSelector: return &AsDouble;
		case ERTSOSettingsType::FloatSlider: return &AsDouble;
		case ERTSOSettingsType::IntegerSelector: return &AsInteger;
		case ERTSOSettingsType::IntegerSlider: return &AsInteger;
		case ERTSOSettingsType::Vector2: return &AsVector2;
		case ERTSOSettingsType::Vector3: return &AsVector3;
		case ERTSOSettingsType::Colour: return &AsColour;
		case ERTSOSettingsType::String: return &AsString;
		case ERTSOSettingsType::EnumAsByte: return &AsEnumByte;
		case ERTSOSettingsType::Key: return &AsKey;
		default: return nullptr;
		}
	}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERTSOSettingsType ValueType = ERTSOSettingsType::Boolean;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Boolean", EditConditionHides))
	bool AsBool;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Double",EditConditionHides))
	double AsDouble;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Double", EditConditionHides))
	FRTSOFloatRange DoubleRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Double", EditConditionHides))
	TArray<double> DoubleList;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Integer",EditConditionHides))
	int32 AsInteger;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Integer", EditConditionHides))
	FRTSOIntegerRange IntegerRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Integer", EditConditionHides))
	TArray<int32> IntegerList;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Vector2",EditConditionHides))
	FVector2D AsVector2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Vector2", EditConditionHides))
	FRTSOVec2Range Vector2Range;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Integer", EditConditionHides))
	TArray<FVector2D> Vector2dList;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Vector3",EditConditionHides))
	FVector AsVector3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Vector3", EditConditionHides))
	FRTSOVec3Range Vector3Range;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Integer", EditConditionHides))
	TArray<FVector> Vector3dList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Colour",EditConditionHides))
	FColor AsColour;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Colour", EditConditionHides))
	FRTSOColourRange ColourRange;	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::Integer", EditConditionHides))
	TArray<FColor> ColourList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::String",EditConditionHides))
	FString AsString;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::String",EditConditionHides))
	TArray<FString> StringList;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERTSOSettingsType AssociatedStringDataType = ERTSOSettingsType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==EERTSOSettingsType::EnumAsByte",EditConditionHides))
	uint8 AsEnumByte;	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::EnumAsByte", EditConditionHides))
	FRTSOEnumRange EnumRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==ERTSOSettingsType::String",EditConditionHides))
	TArray<uint8> EnumList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition="ValueType==EERTSOSettingsType::Key",EditConditionHides))
	FRTSOSettingsKeyData AsKey;
};

namespace PD::Settings
{
	using FDataSelector = FRTSOSettingsDataSelector;

}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTSOSettingsDataDelegate, FRTSOSettingsDataSelector, SettingsDataSelector);

/** @brief Settings data struct for game settings we want to display in the settings menu */
USTRUCT(Blueprintable)
struct FRTSOSettingsData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag SettingTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRTSOSettingsDataSelector ValueSelector;
	
	// todo write function to allow binding funcitons from BP as-well, will need to ue an alternate delegate type then and not 'FRTSOSettingsDataDelegate'
	UPROPERTY(BlueprintAssignable)
	FRTSOSettingsDataDelegate OnValueUpdated;
};

UENUM()
enum class ERTSOSettings : uint8
{
	Gameplay = 0,
	Video,
	Audio,
	Controls,
	Interface,
	Num,
};


namespace PD::Settings
{
	namespace Signatures
	{
		template<typename TSettingValue> 
		class FSettingsDelegate : TDelegate<TSettingValue(const TSettingValue&)>
		{};

		using FProcessFloatValue = FSettingsDelegate<float>;
		using FProcessDoubleValue = FSettingsDelegate<double>;
		using FProcessIntValue = FSettingsDelegate<int32>;
		using FProcessBoolValue = FSettingsDelegate<bool>;
		using FProcessByteValue = FSettingsDelegate<uint8>;
		using FProcessVec2Value = FSettingsDelegate<FVector2D>;
		using FProcessVec3Value = FSettingsDelegate<FVector>;
		using FProcessColourValue = FSettingsDelegate<FColor>;
		using FProcessStringValue = FSettingsDelegate<FString>;
		using FProcessKeyValue = FSettingsDelegate<FKey>;
	}

	using ERTSOSettingsGroups = ERTSOSettings;

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_SettingsBase)
	//
	// Settings - Gameplay categories
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ActionLog)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Difficulty)

	// Settings - Video categories
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Display)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Effects) 
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Graphics)

	// Settings - Audio categories
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Audio)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Audio_Volume)

	// Settings - Control categories
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_UI)

	// Settings - Interface categories
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Interface) 

	enum class EType : uint8
	{
		Camera,
		ActionLog,
		Difficulty,
		Display,
		Effects,
		Graphics,
		Audio,
		Audio_Volume,
		Controls,
		Controls_UI,
		Interface,
		Num,
	};

	namespace Gameplay::Camera
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_RotationRateModifier) // (Slider, Limit to range [40, 100])
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_TargetInterpSpeed) // (Slider, Limit to range [1, 10])
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_ScrollSpeed) // (Slider, Limit to range [1, 10])
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_DoF) // - Depth of Field (Slider)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Camera_FoV) // - Field of View (Slider)
	}
	namespace Gameplay::ActionLog
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ActionLog_Show) // Checkbox
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ActionLog_ColorHightlight0) // FColor picker needed
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ActionLog_ColorHightlight1) // FColor picker needed
	}
	namespace Gameplay::Difficulty
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Difficulty_Combat) // - Combat difficulty (AI Behaviour, Overall damage, NO health increased)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Difficulty_World) // - World difficulty (FogOfWar, Puzzles?, Resource costs)
		// NOTE: Difficulty modifiers should affect AI behavior and overall damage, but never increase AI or player health.
		// NOTE: We want to avoid bullet sponges. Make the player die fast but the enemies die faster.
		// NOTE: Difficulty modifiers should also affect things such a fog of war clearing distance and such.		
	}

	
	namespace Video::Display
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Display_Mode) // Mode (List of options (Windowed, Borderless, Fullscreen))
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Display_Resolution) // Resolution (List of available resolutions)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Display_ResolutionScale) // ResolutionScale (Set resolution scale)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Display_FPSLimit) // FPS Limit (30, 60, 120, 144)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Display_VSyncEnabled) // V-Sync (Checkbox)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Display_Gamma) // Gamma (Slider)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Display_AntiAliasing) // - Anti-aliasing (List of AA options)
	}
	namespace Video::Effects
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Effects_AmbientOcclusion) // - Ambient Occlusion (CheckBox)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Effects_MotionBlur) // - Motion Blur (CheckBox)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Effects_FilmGrain) // - Film Grain (CheckBox)
	}
	namespace Video::Graphics
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Graphics_TextureQuality) // Texture quality (List of Quality options)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Graphics_ShadowQuality) // Shadow quality (List of Quality options)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Graphics_ReflectionQuality) // Reflection quality ? (List of Quality options)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Graphics_GIQuality) // Foliage Quality (List of Quality options)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Graphics_ParticleEffect) // Particle Effect Quality (List of Quality options)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Graphics_FoliageQuality) // Foliage Quality (List of Quality options)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Graphics_ResolutionQuality) // Animation Quality (List of Quality options)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Graphics_PostProcessQuality) // Post process Quality (List of Quality options)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Graphics_ViewDistance) // View Distance (List of Quality options)
	}
	
	namespace Audio
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Audio_MasterVolume) // . Master Volume (Slider)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Audio_SFXVolume)    // - SFX volume (Slider)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Audio_MusicVolume)  // - Music Volume (Slider)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Audio_AmbientVolume)  // - Ambient Volume (Slider)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Audio_VoiceVolume)  // - Voice volume (Slider)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Audio_Subtitles)    // -- Subtitles (List of available language options)		
	}

	namespace Controls::Game
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_MoveForward) //  - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_MoveBackward) //  - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_MoveLeftward) //  - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_MoveRightward) //  - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_RotateLeft) //  - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_RotateRight) //  - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_Interact) //  - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_PauseMenu) //  - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_GameMenu) //  - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_GameMenu_Inventory) //  - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_GameMenu_Map) //  - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_GameMenu_QuestLog) //  - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_Zoom) //  - Analog key selector (or two buttoned key selector), with two alts for each input type (explicit tags for each alt is not needed)
				
	}
	namespace Controls::UI
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_UI_AcceptOrEnter) // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_UI_CancelOrBack) // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_UI_NextTab) // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_UI_PreviousTab) // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_UI_InnerNextTab) // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_UI_InnerPreviousTab) // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_UI_NavigateUp) // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_UI_NavigateDown) // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_UI_NavigateLeft) // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Controls_UI_NavigateRight) // - Discrete Key selector, with two alts for each input type (explicit tags for each alt is not needed)		
	}
	
	namespace Interface
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Interface_UIScaling) // - UIScaling slider (Range [0.5 , 4])
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Interface_ShowObjectiveMarkers) // - UIScaling slider (Range [0.5 , 4])
	}
	
	struct FSettingsDefaultsBase { ERTSOSettingsGroups SettingsGroup; };
	struct FGameplaySettingsDefaults : FSettingsDefaultsBase
	{
		static TMap<FGameplayTag, FRTSOSettingsDataSelector> Camera;
		static TMap<FGameplayTag, FRTSOSettingsDataSelector> ActionLog;
		static TMap<FGameplayTag, FRTSOSettingsDataSelector> Difficulty;
	};
	struct FVideoSettingsDefaults : FSettingsDefaultsBase
	{
		static TMap<FGameplayTag, FRTSOSettingsDataSelector> Display;
		static TMap<FGameplayTag, FRTSOSettingsDataSelector> Effects;
		static TMap<FGameplayTag, FRTSOSettingsDataSelector> Graphics;
	};
	struct FAudioSettingsDefaults : FSettingsDefaultsBase
	{
		static TMap<FGameplayTag, FRTSOSettingsDataSelector> Base;
	};
	struct FControlsSettingsDefaults : FSettingsDefaultsBase
	{
		static TMap<FGameplayTag, FRTSOSettingsDataSelector> Game;
		static TMap<FGameplayTag, FRTSOSettingsDataSelector> UI;
	};	
	struct FInterfaceSettingsDefaults : FSettingsDefaultsBase
	{
		static TMap<FGameplayTag, FRTSOSettingsDataSelector> Base;
	};

	struct FAllSettingsDefaults
	{
		static inline TArray<ERTSOSettingsGroups> TopLevelSettingTypes
		{
			ERTSOSettingsGroups::Gameplay,
			ERTSOSettingsGroups::Video,
			ERTSOSettingsGroups::Audio,
			ERTSOSettingsGroups::Controls,
			ERTSOSettingsGroups::Interface,
		};

		static FName GetTabName(ERTSOSettingsGroups Selector)
		{
			static FName GameplayLabel(*FString("Gameplay"));
			static FName VideoLabel(*FString("Video"));
			static FName AudioLabel(*FString("Audio"));
			static FName ControlsLabel(*FString("Controls"));
			static FName InterfaceLabel(*FString("Interface"));
			switch (Selector)
			{
			case ERTSOSettingsGroups::Gameplay: return GameplayLabel;
			case ERTSOSettingsGroups::Video: return VideoLabel;
			case ERTSOSettingsGroups::Audio: return AudioLabel;
			case ERTSOSettingsGroups::Controls: return ControlsLabel;
			case ERTSOSettingsGroups::Interface: return InterfaceLabel;
			default: break;
			}
			return FName("");
		}

	};

	/** @brief TODO */
	static inline TArray<FString> QualityDefaultStrings{"Low", "Medium", "High", "Epic"};
	static inline TArray<FString> DifficultyDefaultStrings{"Neophyte", "Acolyte", "Adept", "Challenger", "Conduit"};

	//
	// MinMax when fstrings could be where we store another second list laong with it, so we can refer back to values
	static inline TArray<FString> Resolution_Strings{
		"{16:9}  : 2560x1440", "{16:9}  : 1920x1080", "{16:9}  : 1366x768", "{16:9}  : 1280x720",
		"{16:10} : 1920x1200", "{16:10} : 1680x1050", "{16:10} : 1440x900", "{16:10} : 1280x800",
		"{4:3}   : 1024x768",  "{4:3}   : 800x600",   "{4:3}   : 640x480"
		};
	static inline TArray<FVector2D> Resolution_Values{
		/* 16:9  */ FVector2D{2560,1440}, FVector2D{1920,1080}, FVector2D{1366,768}, FVector2D{1280,720},
		/* 16:10 */ FVector2D{1920,1200}, FVector2D{1680,1050}, FVector2D{1440,900}, FVector2D{1280,800},
		/* 4:3   */ FVector2D{1024,768}, FVector2D{800,600}, FVector2D{640,480}
		};


	constexpr double FloatUIMultiplier = 10000; 
};

class RTSOPEN_API FRTSOValueBinderDetails : public IPropertyTypeCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
};

#if WITH_EDITOR
/** @brief 
 * @todo Remember to document this class */
class FRTSOBinderDetailRowBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FRTSOBinderDetailRowBuilder>
{
public:
	FRTSOBinderDetailRowBuilder() {}
	virtual ~FRTSOBinderDetailRowBuilder(){}

    static TSharedRef<SVerticalBox> GenerateSectionTitle(const FText& SectionText);
	//~ Begin IDetailCustomNodeBuilder interface
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override;
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& HeaderRow) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual bool RequiresTick() const override { return false; }
	virtual bool InitiallyCollapsed() const override { return false; }
	virtual FName GetName() const override;
	//~ End IDetailCustomNodeBuilder interface

	TSharedRef<ITableRow> MakeListViewWidget_QualifiedPreprocessFunctions(TSharedPtr<FString> InQualifiedFunctionPath, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnOptionSelected_QualifiedPreprocessFunctions(TSharedPtr<FString> InQualifiedFunctionPath, ESelectInfo::Type InSelectInfo);
    
	void PreprocessPropertyType();
	void PopulateTagArray();
	TSharedRef<class SExpandableArea> SpawnExpandableOptionsArea();
	TSharedRef<class SGameplayTagPicker> SpawnFilteredOptionsPicker();
	TSharedRef<class SVerticalBox> SpawnFilteredOptionsPickerWithSignatureBinder();
	void OnTagSelectorPressed();
	void OnTagMenuDismissed(TSharedRef<IMenu> ClosingMenu);
	void OnCheckboxStateChanged(ECheckBoxState NewState);
	FText GetTagText() const;

    ERTSOSettingsType PropertyType = ERTSOSettingsType::None;
    TSharedPtr<IPropertyHandle> PropertyHandle = nullptr;
	TSharedPtr<IMenu> PopupMenuPtr = nullptr;

	FGameplayTag SelectedSettingsTag;

protected:
    /** @brief */
	FSimpleDelegate OnRegenerateChildren;
	FDelegateHandle FilterHandle;
	FString PropertyCompareTypeName;
	TArray<FGameplayTag> TagsToShow;
	TSharedPtr<class SExpandableArea> ExpandableAreaPtr = nullptr;
	TSharedPtr<class SGameplayTagPicker> TagPickerPtr = nullptr;
	TSharedPtr<SVerticalBox> CombinedBinderListPtr;
	TArray<TSharedPtr<FString>> QualifiedFunctionPaths;
};
#endif // WITH_EDITOR

//
// messy to just wrap these, but worsk for now
USTRUCT(Blueprintable)
struct RTSOPEN_API FVectorWrapper
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Inner;
};

USTRUCT(Blueprintable)
struct RTSOPEN_API FVector2DWrapper
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Inner;
};

USTRUCT(Blueprintable)
struct RTSOPEN_API FColorWrapper
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FColor Inner;
};

USTRUCT(Blueprintable)
struct RTSOPEN_API FLinearColorWrapper
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Inner;
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