/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Actors/Interactables/ConversationHandlers/RTSOInteractableConversationActor.h"

#include "Kismet/KismetSystemLibrary.h"

#include "ConversationContext.h"
#include "ConversationInstance.h"
#include "ConversationChoiceNode.h"
#include "ConversationSettings.h"
#include "ConversationParticipantComponent.h"

#include "Camera/CameraComponent.h"

#include "MassEntitySubsystem.h"
#include "PDRTSBaseSubsystem.h"
#include "AI/Mass/PDMassFragments.h"
#include "PDConversationCommons.h"
#include "Actors/RTSOController.h"

#include "Core/RTSOBaseGM.h"
#include "Interfaces/RTSOConversationInterface.h"
#include "Widgets/Slate/SRTSOActionLog.h"

/** Declaring the "Conversation.Entry" gameplay tags. to be defined in an object-file
 * @todo move to a 'conversation commons' file which I need to create */
UE_DEFINE_GAMEPLAY_TAG(TAG_Conversation_Entry_Intro, "Conversation.Entry.Intro");
UE_DEFINE_GAMEPLAY_TAG(TAG_Conversation_Entry_00, "Conversation.Entry.00");
UE_DEFINE_GAMEPLAY_TAG(TAG_Conversation_Entry_01, "Conversation.Entry.01");


/** Declaring the "Conversation.Participant" gameplay tags. to be defined in an object-file
 * @todo move to a 'conversation commons' file which I need to create */
UE_DEFINE_GAMEPLAY_TAG(TAG_Conversation_Participant_Speaker, "Conversation.Participant.Speaker");
UE_DEFINE_GAMEPLAY_TAG(TAG_Conversation_Participant_Listener, "Conversation.Participant.Listener");

void FRTSOConversationMetaState::ApplyValuesFromProgressionTable(FRTSOConversationMetaProgressionDatum& ConversationProgressionEntry)
{
	// ProgressionPerPlayer = ConversationProgressionEntry.BaseProgression;
	MissionTag = ConversationProgressionEntry.MissionTag;
	PhaseRequiredTags = ConversationProgressionEntry.PhaseRequiredTags;
	
}

ARTSOInteractableConversationActor::ARTSOInteractableConversationActor()
{
	ConversationCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ConversationCamera"));
}

void ARTSOInteractableConversationActor::ConversationStarted()
{
}
void ARTSOInteractableConversationActor::ConversationTaskChoiceDataUpdated(const FConversationNodeHandle& NodeHandle, const FClientConversationOptionEntry& OptionEntry)
{
}
void ARTSOInteractableConversationActor::ConversationUpdated(const FClientConversationMessagePayload& Payload)
{
}
void ARTSOInteractableConversationActor::ConversationStatusChanged(bool bDidStatusChange)
{
}

void URTSOConversationInstance::PauseConversationAndSendClientChoices(
	const FConversationContext& Context,
	const FClientConversationMessage& ClientMessage)
{
	Super::PauseConversationAndSendClientChoices(Context, ClientMessage);

	AActor* ListenerActor = Context.GetParticipantActor(TAG_Conversation_Participant_Listener);

	const APawn* ListenerAsPawn = Cast<APawn>(ListenerActor);
	const ARTSOController* ListenerAsController = ListenerAsPawn != nullptr ?
		ListenerAsPawn->GetController<ARTSOController>() : Cast<ARTSOController>(ListenerActor);
	
	const int32 ActorID =  UPDRTSBaseSubsystem::Get()->SharedOwnerIDBackMappings.FindRef(ListenerAsController);
	if (ChoiceWaitingStateMap.Contains(ActorID) == false || OnBegin_WaitChoicesDelegateMap.Contains(ActorID) == false)
	{
		UE_LOG(PDLog_RTSOInteract, Warning, TEXT("URTSOConversationInstance::PauseConversationAndSendClientChoices -- fail"));
		return;
	}
	
	UE_LOG(PDLog_RTSOInteract, Warning, TEXT("URTSOConversationInstance::PauseConversationAndSendClientChoices -- Broadcasting"));
	*ChoiceWaitingStateMap.Find(ActorID) = true;
	OnBegin_WaitChoicesDelegateMap.Find(ActorID)->Broadcast(ActorID);
}

void URTSOConversationInstance::OnChoiceNodePickedByUser(const FConversationContext& Context, const UConversationChoiceNode* ChoiceNode,
	const TArray<FConversationBranchPoint>& ValidDestinations)
{
	Super::OnChoiceNodePickedByUser(Context, ChoiceNode, ValidDestinations);

	ARTSOController* ListenerAsController = Cast<ARTSOController>(Context.GetParticipant(TAG_Conversation_Participant_Listener)->Actor);

	// @note Store selected tags on server
	ARTSOBaseGM* GM = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARTSOBaseGM>() : nullptr;
	if (GM == nullptr || ListenerAsController == nullptr) { return; }
	GM->GameSave->Data.PlayersAndConversationTags.FindOrAdd(ListenerAsController->GetActorID()).AppendTags(ChoiceNode->ChoiceTags);
	
	IRTSOConversationInterface::Execute_AddProgressionTagContainer(ListenerAsController->GetPawn(), ChoiceNode->ChoiceTags);
}

//
// Conversation function lib
void URTSOConversationBPFL::RequestToAdvance(UPDConversationInstance* Conversation, UConversationParticipantComponent* ConversationParticipantComponent, const FAdvanceConversationRequest& InChoicePicked)
{
	if(Conversation == nullptr || ConversationParticipantComponent == nullptr) { return; }

	const int32 ActorID = UPDRTSBaseSubsystem::Get()->SharedOwnerIDBackMappings
		.FindRef(ConversationParticipantComponent->GetParticipantActor(TAG_Conversation_Participant_Listener));

	if (Conversation->ChoiceWaitingStateMap.Contains(ActorID))
	{
		*Conversation->ChoiceWaitingStateMap.Find(ActorID) = false;
	}

	// @todo Store choice tags here on client
	ConversationParticipantComponent->RequestServerAdvanceConversation(InChoicePicked);
}

URTSOConversationInstance* URTSOConversationBPFL::StartConversation(FGameplayTag ConversationEntryTag, AActor* Instigator,
	FGameplayTag InstigatorTag, AActor* Target, FGameplayTag TargetTag, const TSubclassOf<UConversationInstance> ConversationInstanceClass)
{
#if WITH_SERVER_CODE
	if (Instigator == nullptr || Target == nullptr)
	{
		return nullptr;
	}

	if (UWorld* World = GEngine->GetWorldFromContextObject(Instigator, EGetWorldErrorMode::LogAndReturnNull))
	{
		UClass* InstanceClass = ConversationInstanceClass;
		if (!InstanceClass)
		{
			InstanceClass = GetDefault<UConversationSettings>()->GetConversationInstanceClass();
			if (InstanceClass == nullptr)
			{
				InstanceClass = UConversationInstance::StaticClass();
			}
		}
		URTSOConversationInstance* ConversationInstance = NewObject<URTSOConversationInstance>(World, InstanceClass);
		if (ensure(ConversationInstance))
		{
			FConversationContext Context = FConversationContext::CreateServerContext(ConversationInstance, nullptr);

			UConversationContextHelpers::MakeConversationParticipant(Context, Target, TargetTag);
			UConversationContextHelpers::MakeConversationParticipant(Context, Instigator, InstigatorTag);

			// @todo Generalize and move out of the game module
			const int32 ActorID = UPDRTSBaseSubsystem::Get()->SharedOwnerIDBackMappings.FindRef(Target);
			if (ConversationInstance->ChoiceWaitingStateMap.Contains(ActorID) == false)
			{
				ConversationInstance->ChoiceWaitingStateMap.Add(ActorID) = false;
			}
			
			ConversationInstance->OnBegin_WaitChoicesDelegateMap.FindOrAdd(ActorID).AddDynamic(Cast<IRTSOConversationSpeakerInterface>(Instigator), &IRTSOConversationSpeakerInterface::BeginWaitingForChoices);
			ConversationInstance->ServerStartConversation(ConversationEntryTag);
		}

		return ConversationInstance;
	}
#endif

	return nullptr;	
}


void ARTSOInteractableConversationActor::BeginPlay()
{
	Super::BeginPlay();

	URTSOConversationActorTrackerSubsystem& ConversationTrackerSubsystem = *GetWorld()->GetSubsystem<URTSOConversationActorTrackerSubsystem>();
	ConversationTrackerSubsystem.TrackedConversationActors.Emplace(this);
	
	ParticipantComponent =
		Cast<UConversationParticipantComponent>(AddComponentByClass(UConversationParticipantComponent::StaticClass(), false, FTransform::Identity, false));

	ParticipantComponent->ConversationStarted.AddUObject(this, &ARTSOInteractableConversationActor::ConversationStarted);
	ParticipantComponent->ConversationTaskChoiceDataUpdated.AddUObject(this, &ARTSOInteractableConversationActor::ConversationTaskChoiceDataUpdated);
	ParticipantComponent->ConversationUpdated.AddUObject(this, &ARTSOInteractableConversationActor::ConversationUpdated);
	ParticipantComponent->ConversationStatusChanged.AddUObject(this, &ARTSOInteractableConversationActor::ConversationStatusChanged);
	
	UPDAsyncAction_ActivateFeature* ActiveFeature = UPDAsyncAction_ActivateFeature::CreateActionInstance(this, GameFeatureName);
	ActiveFeature->Activate();
	
	JobTag = TAG_AI_Job_GenericInteract;


	for (const FDataTableRowHandle& ConversationSettingsHandle : ConversationSettingsHandles)
	{
		if (ConversationSettingsHandle.IsNull()) { continue; }
		const FString Context = FString::Printf(TEXT("ARTSOInteractableConversationActor(%s)::OnConstruction -- Attempting to access ConversationSettingsHandle.GetRow<FRTSOConversationMetaProgressionDatum>() "), *GetName());

		FRTSOConversationMetaProgressionDatum* BaseMetaProgressDatum = ConversationSettingsHandle.GetRow<FRTSOConversationMetaProgressionDatum>(Context);
		FRTSOConversationMetaState& MetaProgressState = InstanceDataPerMission.FindOrAdd(BaseMetaProgressDatum->MissionTag);
		MetaProgressState.MissionTag = BaseMetaProgressDatum->MissionTag;
		MetaProgressState.PhaseRequiredTags = BaseMetaProgressDatum->PhaseRequiredTags;

		AttemptInitializeFirstPlayerMission(INDEX_NONE, BaseMetaProgressDatum->MissionTag);
		
		FGameplayTag& BaseState = CurrentMissionPerPlayer.FindOrAdd(INDEX_NONE);
		if (BaseState.IsValid() == false)
		{
			BaseState = BaseMetaProgressDatum->MissionTag;
		}		
	}
	InstanceDataPerMissionPtr = &InstanceDataPerMission;
}

void ARTSOInteractableConversationActor::BeginDestroy()
{
	Super::BeginDestroy();

	if (GetWorld() == nullptr
		|| GetWorld()->bIsWorldInitialized == false
		|| GetWorld()->GetSubsystem<URTSOConversationActorTrackerSubsystem>() == nullptr)
	{
		return;
	}
	URTSOConversationActorTrackerSubsystem* ConversationTrackerSubsystem = GetWorld()->GetSubsystem<URTSOConversationActorTrackerSubsystem>();
	ConversationTrackerSubsystem->TrackedConversationActors.Remove(this);	
}

void ARTSOInteractableConversationActor::AttemptInitializeFirstPlayerMission(int32 UserID, const FGameplayTag& MissionTag)
{
	FGameplayTag& State = CurrentMissionPerPlayer.FindOrAdd(UserID);
	if (State.IsValid()) { return; } // Exit early if already has a valid tag		

	State = MissionTag;
}

void ARTSOInteractableConversationActor::SetCurrentPlayerMission(int32 UserID, const FGameplayTag& MissionTag)
{
	FGameplayTag& State = CurrentMissionPerPlayer.FindOrAdd(UserID);
	State = MissionTag;	// overwrite last mission tag of the associated user state
}

FGameplayTagContainer ARTSOInteractableConversationActor::GetGenericTagContainer_Implementation() const
{
	FGameplayTagContainer GeneratedTags;
	GeneratedTags.AddTag(JobTag);
	return GeneratedTags;
}

void ARTSOInteractableConversationActor::OnInteract_Implementation(
	const FPDInteractionParamsWithCustomHandling& InteractionParams,
	EPDInteractResult& InteractResult) const
{
	Super::OnInteract_Implementation(InteractionParams, InteractResult);
	
	switch (InteractResult)
	{
	case EPDInteractResult::INTERACT_SUCCESS:
	case EPDInteractResult::INTERACT_FAIL:
	case EPDInteractResult::INTERACT_DELAYED:
		return; // was handled by the delegate in InteractionParams
	case EPDInteractResult::INTERACT_UNHANDLED:
		break; // was not handled by a supplied delegate
	}

	// Nasty, I know, but right now I can't mark the function as const and still be able to bind it to the dynamic delegate 
	ARTSOInteractableConversationActor* MutableSelf = const_cast<ARTSOInteractableConversationActor*>(this);

	const TMap<int32, AActor*>& IDToActorMap =  UPDRTSBaseSubsystem::Get()->SharedOwnerIDMappings;
	const UMassEntitySubsystem& EntitySubsystem = *GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	const FPDMFragment_RTSEntityBase* EntityBase = EntitySubsystem.GetEntityManager().GetFragmentDataPtr<FPDMFragment_RTSEntityBase>(InteractionParams.InstigatorEntity);
	MutableSelf->AttemptInitializeFirstPlayerMission(EntityBase->OwnerID, MutableSelf->CurrentMissionPerPlayer[INDEX_NONE]);
	const FGameplayTag& CurrentMission = MutableSelf->CurrentMissionPerPlayer[EntityBase->OwnerID];
	
	if (InstanceDataPerMissionPtr->Contains(CurrentMission) == false || InteractionParams.InteractionPercent <= .85) { return; }
	FRTSOConversationMetaState& MetaProgressState = *InstanceDataPerMissionPtr->Find(CurrentMission);	

	AActor* SelectedActor = EntityBase != nullptr && IDToActorMap.Contains(EntityBase->OwnerID) ?
		IDToActorMap.FindRef(EntityBase->OwnerID) : InteractionParams.InstigatorActor;
	
	APawn* AsCallingPawn = Cast<APawn>(SelectedActor);
	ARTSOController* AsCallingController = AsCallingPawn != nullptr ?
		AsCallingPawn->GetController<ARTSOController>() : Cast<ARTSOController>(SelectedActor);
	if (AsCallingController == nullptr) { return; }
	AsCallingPawn = AsCallingPawn == nullptr ? AsCallingController->GetPawn() : AsCallingPawn;
	
	const int32 OwnerID = AsCallingController->GetActorID();
	TryInitializeOwnerProgression(OwnerID);

	static int32 Dummy = INDEX_NONE;
	int32& CurrentProgressionRef = MetaProgressState.ProgressionPerPlayer.Contains(OwnerID) ? *MetaProgressState.ProgressionPerPlayer.Find(OwnerID) : Dummy;

	UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ARTSOInteractableConversationActor::OnInteract"));
	const ERTSOConversationState ConversationState =
		ValidateRequiredTags(ResolveTagForProgressionLevel(OwnerID, CurrentProgressionRef), AsCallingPawn);
	switch (ConversationState)
	{
	case CurrentStateCompleted:
		// handle based on conversation rules
		{
			// @todo need a way to display repeatable Conversations even after incrementing CurrentProgressionRef, as CurrentProgressionRef is used further down the list
			const int CopyProgression = CurrentProgressionRef;
			CurrentProgressionRef++;

			const FRTSOActionLogEvent NewActionEvent{ TAG_ActionLog_Styling_Entry_T1, TAG_ActionLog_Styling_Timestamp_T1,
				FString::Printf(TEXT(
					"OwnerID(%i) -- Successfully Finished Phase{%i} of Mission %s "),
					OwnerID, CopyProgression, *CurrentMission.GetTagName().ToString())}; 
			URTSActionLogSubsystem::DispatchEvent(OwnerID, NewActionEvent);
			
			if (CanRepeatConversation(OwnerID, CopyProgression) == false) { return; }
		}
		break;
	case Invalid:
		UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ARTSOInteractableConversationActor::OnInteract -- Invalid State"));
		return;
	case Valid:
		// Continue on as normal 
		break;
	default: return; // ValidateRequiredTags Never returns anything outside of the three above states
	}

	UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ARTSOInteractableConversationActor::OnInteract - 2"));

	// open conversation here with instigator
	MetaProgressState.ActiveConversationInstance =
		Cast<URTSOConversationInstance>(
			URTSOConversationBPFL::StartConversation(
			MetaProgressState.PhaseRequiredTags[CurrentProgressionRef].EntryTag,
			(ARTSOInteractableConversationActor*)this, 
			TAG_Conversation_Participant_Speaker, 
			AsCallingController,
			TAG_Conversation_Participant_Listener,
			URTSOConversationInstance::StaticClass()));
	if (MetaProgressState.ActiveConversationInstance == nullptr) { return; }
	
	const int32 ActorID =  UPDRTSBaseSubsystem::Get()->SharedOwnerIDBackMappings.FindRef(AsCallingController);
	MetaProgressState.ActiveConversationInstance->ServerRefreshConversationChoices();
	if (MetaProgressState.ActiveConversationInstance->ChoiceWaitingStateMap.FindRef(ActorID))
	{
		ARTSOInteractableConversationActor::Execute_BeginWaitingForChoices(MutableSelf, ActorID);
	}
	
	UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ARTSOInteractableConversationActor::OnInteract - 3"));
}

void ARTSOInteractableConversationActor::BeginWaitingForChoices_Implementation(int32 ActorID) 
{
	const FClientConversationMessagePayload& Payload = ParticipantComponent->GetLastMessage();
	UPDConversationLibrary::Debug_PrintConversationMessageToScreen(this, Payload.Message, FLinearColor::Blue);
	
	UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ARTSOInteractableConversationActor::BeginWaitingForChoices"));
	for (const FClientConversationOptionEntry& Opt : Payload.Options)
	{
		FName Participant{};
		switch (Opt.ChoiceType)
		{
		case EConversationChoiceType::ServerOnly:
			break;
		case EConversationChoiceType::UserChoiceAvailable:
			break;
		case EConversationChoiceType::UserChoiceUnavailable:
			Participant = FName("(Can't select)");
			break;
		}

		UPDConversationLibrary::Debug_PrintConversationTextToScreen(this, Participant, Opt.ChoiceText,FLinearColor::Blue);
	}

	// bug: some engine bug causing participant component to always fail returning TAG_Conversation_Participant_Listener even though it was set in the same frame
	// workaround, use the PDRTSBaseSubsystem id mappings and add an ID parameter to find the proper controller 
	//ParticipantComponent->GetParticipantActor(TAG_Conversation_Participant_Listener));
	const TMap<int32, AActor*>& IDToActorMap =  UPDRTSBaseSubsystem::Get()->SharedOwnerIDMappings;
	ARTSOController* ListenerAsController = Cast<ARTSOController>(IDToActorMap.FindRef(ActorID));

	if (ListenerAsController->IsMappingContextActive(TAG_CTRL_Ctxt_ConversationMode) == false)
	{
		ListenerAsController->OnBeginConversation(Payload, this);
		return;
	}
	ListenerAsController->OnAdvanceConversation(Payload, this);
	
}

void ARTSOInteractableConversationActor::ReplyChoice_Implementation(AActor* Caller, int32 Choice)
{
	UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ARTSOInteractableConversationActor::ReplyChoice - Choice: %i"), Choice);

	const ARTSOController* AsCallingController = Cast<ARTSOController>(Caller);
	const int32 OwnerID = AsCallingController->GetActorID();	
	const FGameplayTag& CurrentMission = CurrentMissionPerPlayer[OwnerID];
	
	if (InstanceDataPerMissionPtr->Contains(CurrentMission) == false ) { return; }
	const FRTSOConversationMetaState& MetaProgressState = *InstanceDataPerMissionPtr->Find(CurrentMission);
	
	const FClientConversationMessagePayload& Payload = ParticipantComponent->GetLastMessage();
	if (Payload.Options.IsValidIndex(Choice) == false)
	{
		const FAdvanceConversationRequest AdvanceRequest{FConversationChoiceReference::Empty};
		URTSOConversationBPFL::RequestToAdvance(MetaProgressState.ActiveConversationInstance, ParticipantComponent, AdvanceRequest);		
		return;
	}

	const FClientConversationOptionEntry& OptionEntry = Payload.Options[Choice];
	
	FAdvanceConversationRequest AdvanceRequest{OptionEntry.ChoiceReference};
	AdvanceRequest.UserParameters = OptionEntry.ExtraData;
	
	URTSOConversationBPFL::RequestToAdvance(MetaProgressState.ActiveConversationInstance, ParticipantComponent, AdvanceRequest);

	const FString BuildString = "Selected{" + FString::FromInt(Choice) + "}";
	UKismetSystemLibrary::PrintString(this, BuildString, true, true, FLinearColor::Blue, 50);
}

void ARTSOInteractableConversationActor::TryInitializeOwnerProgression(const int32 OwnerID) const
{
	const FGameplayTag& CurrentMission = CurrentMissionPerPlayer[OwnerID];
	if (InstanceDataPerMissionPtr->Contains(CurrentMission) == false )
	{
		UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ConversationActor(%s)::TryInitializeOwnerProgression -- InstanceData for mission(%s) invalid"), *GetName(), *CurrentMission.GetTagName().ToString())
		return;
	}

	CurrentMissionPerPlayer[INDEX_NONE];

	FRTSOConversationMetaState& MetaProgressState = *InstanceDataPerMissionPtr->Find(CurrentMission);

	if (MetaProgressState.ProgressionPerPlayer.Contains(OwnerID) == false)
	{
		MetaProgressState.ProgressionPerPlayer.Add(OwnerID) = 0;
	}
}

ERTSOConversationState ARTSOInteractableConversationActor::ValidateRequiredTags(const FGameplayTag& EntryTag, AActor* CallingActor) const
{
	ERTSOConversationState ValidatedTagState = ERTSOConversationState::Invalid;
	APawn* AsCallingPawn = Cast<APawn>(CallingActor);
	ARTSOController* AsCallingController = AsCallingPawn != nullptr ?
		AsCallingPawn->GetController<ARTSOController>() : Cast<ARTSOController>(CallingActor);
	if (AsCallingController == nullptr)
	{
		UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ConversationActor(%s)::ValidateRequiredTags -- No valid calling controller"), *GetName())
		return ValidatedTagState; 
	}
	
	if (CallingActor == nullptr || AsCallingPawn->GetClass()->ImplementsInterface(URTSOConversationInterface::StaticClass()) == false)
	{
		UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ConversationActor(%s)::ValidateRequiredTags -- No valid calling controller"), *GetName())
		return ValidatedTagState;
	}

	const int32 OwnerID = AsCallingController->GetActorID();	
	const FGameplayTag& CurrentMission = CurrentMissionPerPlayer[OwnerID];
	
	if (InstanceDataPerMissionPtr->Contains(CurrentMission) == false )
	{
		UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ConversationActor(%s)::ValidateRequiredTags -- InstanceData for mission(%s) invalid"), *GetName(), *CurrentMission.GetTagName().ToString())
		return ValidatedTagState;
	}
	FRTSOConversationMetaState& MetaProgressState = *InstanceDataPerMissionPtr->Find(CurrentMission);
	
	const FGameplayTagContainer& ActorTagSet = Cast<IRTSOConversationInterface>(AsCallingPawn)->GetProgressionTags();
	const int32 LastConversationProgressionIndex = MetaProgressState.PhaseRequiredTags.Num();
	int32 ConversationProgressionLevel = 0;

	TryInitializeOwnerProgression(OwnerID);
	for (; ConversationProgressionLevel < LastConversationProgressionIndex ;)
	{
		if (ConversationProgressionLevel > MetaProgressState.ProgressionPerPlayer.FindOrAdd(OwnerID))
			{
				UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ConversationActor(%s)::ValidateRequiredTags -- progression index is invalid"), *GetName())
				break;
			}
		
		const FRTSOConversationRules& RequiredRules = MetaProgressState.PhaseRequiredTags[ConversationProgressionLevel];
		ConversationProgressionLevel++;

		// Invalid entry tag, might be too early in that conversation 'tree'
		if (RequiredRules.EntryTag != EntryTag)
		{
			UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ConversationActor(%s)::ValidateRequiredTags -- Fail not equal tags"), *GetName())
			continue;
		}

		// Already completed
		if(RequiredRules.State == ERTSOConversationState::CurrentStateCompleted)
		{
			UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ConversationActor(%s)::ValidateRequiredTags -- Already Completed"), *GetName())
			return ERTSOConversationState::CurrentStateCompleted;
		}
		
		// Wish epic had implemented some form of set comparator function, they might have need to look at the TSet implementation
		// but in-case it would be an mmo with lets say 5-10 thousand players at most we can expect that many conversations to be held as a max limit,
		// and should be easy for the servers to handle.
		// Realistically at a given moment the amount of validation calls will likely by much fewer
		bool bFoundAllRequiredTags = true; // Default to true if there are no requirements
		for (const FGameplayTag& RequiredTag : RequiredRules.RequiredTags)
		{
			bFoundAllRequiredTags &= ActorTagSet.HasTag(RequiredTag); // Needs them all to exist in the actor tag set
		}
		ValidatedTagState = bFoundAllRequiredTags ?
			ERTSOConversationState::Valid : ERTSOConversationState::Invalid;

		UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ConversationActor(%s)::ValidateRequiredTags -- bFoundAllRequiredTags: %i"), *GetName(), bFoundAllRequiredTags)
	}

	return ValidatedTagState;
}

bool ARTSOInteractableConversationActor::CanRepeatConversation(int32 UserID, const int32 ConversationProgression) const
{
	const FGameplayTag& CurrentMission = CurrentMissionPerPlayer[UserID];
	
	if (InstanceDataPerMissionPtr->Contains(CurrentMission) == false )
	{
		UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ConversationActor(%s)::ValidateRequiredTags -- InstanceData for mission(%s) invalid"), *GetName(), *CurrentMission.GetTagName().ToString())
		return false;
	}
	FRTSOConversationMetaState& MetaProgressState = *InstanceDataPerMissionPtr->Find(CurrentMission);
	return MetaProgressState.PhaseRequiredTags[ConversationProgression].bCanRepeatConversation;
}

const FGameplayTag& ARTSOInteractableConversationActor::ResolveTagForProgressionLevel(int32 UserID, const int32 ConversationProgression) const
{
	const FGameplayTag& CurrentMission = CurrentMissionPerPlayer[UserID];
	
	if (InstanceDataPerMissionPtr->Contains(CurrentMission) == false )
	{
		UE_LOG(PDLog_RTSOInteract, Warning, TEXT("ConversationActor(%s)::ValidateRequiredTags -- InstanceData for mission(%s) invalid"), *GetName(), *CurrentMission.GetTagName().ToString())
		return FGameplayTag::EmptyTag;
	}
	FRTSOConversationMetaState& MetaProgressState = *InstanceDataPerMissionPtr->Find(CurrentMission);
	return MetaProgressState.PhaseRequiredTags[ConversationProgression].EntryTag;
}

void ARTSOInteractableConversationActor::OnConversationPhaseStateChanged(const FGameplayTag& EntryTag, ERTSOConversationState NewState)
{
}

URTSOConversationActorTrackerSubsystem* URTSOConversationActorTrackerSubsystem::Get(UWorld* World)
{
	check(World && "World ins static URTSOConversationActorTrackerSubsystem::Get is not valid")
	return World->GetSubsystem<URTSOConversationActorTrackerSubsystem>();
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
