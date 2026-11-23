/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "PDRTSSharedUI.h"
#include "PDRTSCommon.h"
#include "PDBuildCommon.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TileView.h"
#include "Components/Slider.h"
#include "Interfaces/PDRTSBuilderInterface.h"

#include "Components/EditableTextBox.h"
#include "UI/PDNumberBoxes.h"

//
// build system widgets
void UPDBuildableEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UPDBuildableWrapper* Item = Cast<UPDBuildableWrapper>(ListItemObject);
	if (Item == nullptr) { return; }
	
	// 1. Read data
	BuildableTitle->SetText(Item->GetBuildableTitle());
	bCanBuild             = Item->GetCanBuild();
	DirectParentReference = Item->GetDirectParentReference();
	ParentMenuContextTag  = Item->GetParentContextTag();
	BuildableTag          = Item->GetBuildableTag();
	SetOwningPlayer(DirectParentReference->GetOwningPlayer());
	
	// 2. Set data-asset variable
	TArray<FDataTableRowHandle>& BuildContexts = DirectParentReference->GetCurrentBuildContexts();
	for (const FDataTableRowHandle& Context : BuildContexts)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildableEntry(%s)::NativeOnListItemObjectSet(ContextRow: %s)"), *GetName(), *Context.RowName.ToString());
		const FPDBuildContext* LoadedContext = Context.GetRow<FPDBuildContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag.IsValid() == false || LoadedContext->ContextTag != ParentMenuContextTag)
		{
			/** @todo Output to log with warning or error level verbosity if the context or the tag was invalid */ 
			continue;
		}

		const UPDBuildableDataAsset* BuildDataAsset = nullptr;
		const TArray<FDataTableRowHandle>& BuildablesData = LoadedContext->BuildablesData;
		for (const FDataTableRowHandle& BuildableData : BuildablesData)
		{
			const FString BuildDatumCtxtStr = CtxtStr + FString::Printf(TEXT("(BuildableData: %s)"), *BuildableData.RowName.ToString());
			const FPDBuildable* Buildable = BuildableData.GetRow<FPDBuildable>(BuildDatumCtxtStr);
			if (Buildable == nullptr || Buildable->BuildableTag != Item->GetBuildableTag())
			{
				continue;
			}
			
			BuildDataAsset = Buildable->BuildableData.DABuildAsset;
			break;
		}
		
		if (BuildDataAsset != nullptr)
		{
			UMaterialInstance* MatInst = BuildDataAsset->Buildable_MaterialInstance;
			UTexture2D* Tex = BuildDataAsset->Buildable_Texture;
			if (MatInst != nullptr)
			{
				TextContentBorder->SetBrushFromMaterial(MatInst);
			}
			else if (Tex != nullptr) // Fallback to texture
			{
				TextContentBorder->SetBrushFromTexture(Tex);
			}
		}
	}
	
	// 3. Clear and Bind delegates
	Hitbox->OnClicked.Clear();
	Hitbox->OnClicked.AddDynamic(this, &UPDBuildableEntry::OnClicked);

	Hitbox->OnReleased.Clear();
	Hitbox->OnReleased.AddDynamic(this, &UPDBuildableEntry::OnReleased);
	
	Hitbox->OnHovered.Clear();
	Hitbox->OnHovered.AddDynamic(this, &UPDBuildableEntry::OnHovered);
	
	Hitbox->OnUnhovered.Clear();
	Hitbox->OnUnhovered.AddDynamic(this, &UPDBuildableEntry::OnUnhovered);

	Hitbox->OnPressed.Clear();
	Hitbox->OnPressed.AddDynamic(this, &UPDBuildableEntry::OnPressed);
}

void UPDBuildableEntry::OnHovered()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildableEntry::OnHovered"))
	/** @todo: write impl.*/
}

void UPDBuildableEntry::OnUnhovered()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildableEntry::OnUnhovered"))
	/** @todo: write impl.*/
}

void UPDBuildableEntry::OnClicked()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildableEntry::OnClicked"))
	/** @todo: write impl.*/
}

void UPDBuildableEntry::OnReleased()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildableEntry::OnReleased"))
	/** @todo: write impl.*/
}

void UPDBuildableEntry::OnPressed()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildableEntry::OnPressed"))

	const APawn* CachedOwner = GetOwningPlayer()->GetPawnOrSpectator();
	if (CachedOwner == nullptr)
	{
		/** @todo @test also write tests for this */
		UE_LOG(PDLog_RTSBase, Error, TEXT("UPDBuildableEntry::OnPressed - Player owning pawn is not set!"))
		return;
	}
	
	if (CachedOwner->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()) == false)
	{
		/** @todo @test also write tests for this */
		UE_LOG(PDLog_RTSBase, Error, TEXT("UPDBuildableEntry::OnPressed - Player pawn class does not implement 'PDRTSBuilderInterface' or object has been sliced."))
		
		return;
	}

	// tell DirectParentReference about the clicked button 
	DirectParentReference->SelectBuildable(BuildableTag);		
}

void UPDBuildContextEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UPDBuildContextWrapper* Item = Cast<UPDBuildContextWrapper>(ListItemObject);
	if (Item == nullptr) { return; }

	
	// 1. Read data
	ContextTitle->SetText(Item->GetBuildContextTitle());
	SelfContextTag        = Item->GetContextTag();
	DirectParentReference = Item->GetDirectParentReference();
	SetOwningPlayer(DirectParentReference->GetOwningPlayer());

	// 2. Set data-asset variable
	TArray<FDataTableRowHandle>& BuildContexts = DirectParentReference->GetCurrentBuildContexts();
	for (const FDataTableRowHandle& Context : BuildContexts)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildContextEntry(%s)::NativeOnListItemObjectSet(Context: %s)"), *GetName(), *Context.RowName.ToString());
		const FPDBuildContext* LoadedContext = Context.GetRow<FPDBuildContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag.IsValid() == false)
		{
			/** @todo Output to log with warning or error level verbosity if the context or the tag was invalid */ 
			continue;
		}

		const UPDBuildContextDataAsset* BuildDataAsset = LoadedContext->ContextData.DABuildContextAsset;
		if (BuildDataAsset != nullptr)
		{
			UMaterialInstance* MatInst = BuildDataAsset->BuildContext_MaterialInstance;
			UTexture2D* Tex = BuildDataAsset->BuildContext_Texture;
			if (MatInst != nullptr)
			{
				TextContentBorder->SetBrushFromMaterial(MatInst);
			}
			else if (Tex != nullptr) // Fallback to texture
			{
				TextContentBorder->SetBrushFromTexture(Tex);
			}
		}
	}

	// 3 Clear and Bind delegates
	Hitbox->OnClicked.Clear();
	Hitbox->OnClicked.AddDynamic(this, &UPDBuildContextEntry::OnClicked);

	Hitbox->OnReleased.Clear();
	Hitbox->OnReleased.AddDynamic(this, &UPDBuildContextEntry::OnReleased);

	Hitbox->OnHovered.Clear();
	Hitbox->OnHovered.AddDynamic(this, &UPDBuildContextEntry::OnHovered);

	Hitbox->OnUnhovered.Clear();
	Hitbox->OnUnhovered.AddDynamic(this, &UPDBuildContextEntry::OnUnhovered);

	Hitbox->OnPressed.Clear();
	Hitbox->OnPressed.AddDynamic(this, &UPDBuildContextEntry::OnPressed);
}

void UPDBuildContextEntry::OnHovered()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildContextEntry::OnHovered"))
	/** @todo: write impl.*/
}
void UPDBuildContextEntry::OnUnhovered()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildContextEntry::OnUnhovered"))
	
	/** @todo: write impl.*/
}

void UPDBuildContextEntry::OnClicked()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildContextEntry::OnClicked"))
	/** @todo: write impl.*/
}

void UPDBuildContextEntry::OnReleased()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildContextEntry::OnReleased"))
	
	/** @todo: write impl.*/
}

void UPDBuildContextEntry::OnPressed()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildContextEntry::OnPressed"))

	APawn* CachedOwner = GetOwningPlayer()->GetPawnOrSpectator();
	if (CachedOwner == nullptr)
	{
		/** @todo @test also write tests for this */
		UE_LOG(PDLog_RTSBase, Error, TEXT("UPDBuildContextEntry::OnPressed - Player owning pawn is not set!"))
		
		return;
	}
	
	if (CachedOwner->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()) == false)
	{
		/** @todo @test also write tests for this */
		UE_LOG(PDLog_RTSBase, Error, TEXT("UPDBuildContextEntry::OnPressed - Player pawn class does not implement 'PDRTSBuilderInterface' or object has been sliced."))
		
		return;
	}

	IPDRTSBuilderInterface::Execute_SelectBuildMenuEntry(CachedOwner, ERTSBuildMenuModules::SelectContext, SelfContextTag);
	
	// if you hit this ensure´then you've forgotten to add an appropriate parent reference upon spawning the widget 
	ensure(DirectParentReference != nullptr);

	if (DirectParentReference->SelectedWidgetFlair == nullptr)
	{
		/** @todo: log about widget flair not being set.*/
	}

	// tell DirectParentReference about the clicked button 
	DirectParentReference->UpdateSelectedContext(SelfContextTag);	
}

void UPDBuildWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	CurrentBuildableContexts = DefaultBuildableContexts;
	
	SelectedWidgetFlair = BuildWidgetFlairHandle.GetRow<FPDSharedBuildWidgetFlair>("");
}

void UPDBuildWidgetBase::OverwriteBuildContext(const FPDBuildContextParam& NewRowParams)
{
	CurrentBuildableContexts = NewRowParams.NewBuildableContext;
}

void UPDBuildWidgetBase::LoadBuildContexts()
{
	// Build the context menu entries (Category)
	BuildContexts->ClearListItems();

	FGameplayTag SelectedContextTag = FGameplayTag::EmptyTag;
	bool bLastSelectedContextStillValid = false;
	for (const FDataTableRowHandle& Context : CurrentBuildableContexts)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildWidgetBase(%s)::LoadBuildContexts(Context: %s)"), *GetName(), *Context.RowName.ToString());
		FPDBuildContext* LoadedContext = Context.GetRow<FPDBuildContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag.IsValid() == false)
		{
			/** @todo Output to log with warning or error level verbosity if the context or the tag was invalid */ 
			continue;
		}
		
		UPDBuildContextWrapper* DataWrapper = NewObject<UPDBuildContextWrapper>(this, UPDBuildContextWrapper::StaticClass());		

		FText SelectedName = LoadedContext->ContextReadableName.IsEmpty() ?
			FText::FromName(LoadedContext->ContextTag.GetTagName()) : LoadedContext->ContextReadableName;

		TArray<FDataTableRowHandle>& InnerBuildableHandles = LoadedContext->BuildablesData; 
		
		DataWrapper->AssignData(SelectedName, LoadedContext->ContextTag, InnerBuildableHandles, this);
		BuildContexts->AddItem(DataWrapper);
		
		SelectedContextTag = SelectedContextTag.IsValid() == false ? LoadedContext->ContextTag : SelectedContextTag;
		bLastSelectedContextStillValid &= LastSelectedContextTag == LoadedContext->ContextTag;
	}
	
	SelectedContextTag = bLastSelectedContextStillValid ? LastSelectedContextTag : SelectedContextTag;
	SelectBuildContext(SelectedContextTag, true);
}

void UPDBuildWidgetBase::SelectBuildContext(const FGameplayTag& NewSelectedContext, const bool bWasDeselected)
{
	bool bRequestedContextWasValid = false;
	for (const FDataTableRowHandle& Context : CurrentBuildableContexts)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildWidgetBase(%s)::SelectBuildContext(ContextRow: %s)"), *GetName(), *Context.RowName.ToString());
		FPDBuildContext* LoadedContext = Context.GetRow<FPDBuildContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag != NewSelectedContext) { continue; }

		bRequestedContextWasValid = true;
		
		Buildables->ClearListItems();
		if (bWasDeselected) // deselecting a category should intuitively deselect any selected entry
		{
			APawn* CachedOwner = GetOwningPlayer()->GetPawnOrSpectator();
			if (ensure(CachedOwner != nullptr && CachedOwner->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass())))
			{
				IPDRTSBuilderInterface::Execute_SelectBuildMenuEntry(CachedOwner, ERTSBuildMenuModules::DeselectBuildable, FGameplayTag::EmptyTag);
			}
			
			return;
		}
		
		for (const FDataTableRowHandle& BuildData : LoadedContext->BuildablesData)
		{
			const FString BuildableCtxtStr = FString::Printf(TEXT("UPDBuildWidgetBase(%s)::SelectBuildContext(BuildableData: %s)"), *GetName(), *BuildData.RowName.ToString());
			
			const FPDBuildable* Buildable = BuildData.GetRow<FPDBuildable>(BuildableCtxtStr);
			if (Buildable == nullptr) { continue; }

			UPDBuildableWrapper* DataWrapper = NewObject<UPDBuildableWrapper>(this, UPDBuildableWrapper::StaticClass());
			
			FGameplayTag BuildableTag = Buildable->BuildableTag;
			const FPDBuildableData& DABuild = Buildable->BuildableData;

			constexpr bool bCanBuild = true; // @todo extrapolate bCanBuild based on available resources and such, possibly control via a virtual function and define in game module
			DataWrapper->AssignData(DABuild.ReadableName, bCanBuild, BuildableTag, LoadedContext->ContextTag, this);
			Buildables->AddItem(DataWrapper);
		}
		break;
	}
	
	LastSelectedContextTag = bRequestedContextWasValid ? NewSelectedContext : LastSelectedContextTag;
}

void UPDBuildWidgetBase::UpdateSelectedContext(const FGameplayTag& RequestToSelectTag)
{
	// Is it already selected?
	bool bWasContextSelected = false;
	for (const FDataTableRowHandle& Context : CurrentBuildableContexts)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildWidgetBase(%s)::UpdateSelectedContext(ContextRow: %s)"), *GetName(), *Context.RowName.ToString());
		const FPDBuildContext* LoadedContext = Context.GetRow<FPDBuildContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag.IsValid() == false)
		{
			/** @todo Output to log with warning or error level verbosity if the context or the tag was invalid */ 
			continue;
		}
		
		// Resets any previous selected tints
		const TArray<UObject*>& ListItems = BuildContexts->GetListItems();
		for (UObject* const& Item : ListItems)
		{
			UPDBuildContextEntry* AsContextEntry = BuildContexts->GetEntryWidgetFromItem<UPDBuildContextEntry>(Item);
			if (AsContextEntry == nullptr || AsContextEntry->IsValidLowLevelFast() == false)
			{
				continue;
			}
			
			// Selected new tag, @todo clean things up here
			FSlateBrush& Brush = AsContextEntry->TextContentBorder->Background;
			const FString& ContextEntryString = AsContextEntry->SelfContextTag.GetTagName().ToString();
			const bool bShouldSelect = (RequestToSelectTag == AsContextEntry->SelfContextTag) && AsContextEntry->bIsSelected == false;
			if (bShouldSelect)
			{
				UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildWidgetBase::UpdateSelectedContext -- Selecting context(%s)"), *ContextEntryString)
				Brush.TintColor = FSlateColor(SelectedWidgetFlair->SelectedContextTint);
				bWasContextSelected = AsContextEntry->bIsSelected = true;
			}
			else // Deselecting any entries we had selected before // @todo redesign this slightly ,as we only need to know the previously selected entry, we should cache it so we can skip this loop
			{
				UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildWidgetBase::UpdateSelectedContext -- Deselecting context(%s)"), *ContextEntryString)				
				Brush.TintColor = FSlateColor(SelectedWidgetFlair->NotSelectedContextTint);
				bWasContextSelected = AsContextEntry->bIsSelected = false;
			}
			AsContextEntry->TextContentBorder->SetBrush(Brush);
		}
	}
	
	SelectBuildContext(RequestToSelectTag, bWasContextSelected == false);
}

void UPDBuildWidgetBase::SelectBuildable(const FGameplayTag& NewSelectedBuildable)
{
	bool bRequestedBuildableWasValid = false;
	for (const FDataTableRowHandle& Context : CurrentBuildableContexts)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildWidgetBase(%s)::SelectBuildable(ContextRow: %s)"), *GetName(), *Context.RowName.ToString());
		FPDBuildContext* LoadedContext = Context.GetRow<FPDBuildContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag.IsValid() == false) { continue; }
		
		for (const FDataTableRowHandle& BuildData : LoadedContext->BuildablesData)
		{
			const FString BuildDatumCtxtStr = CtxtStr + FString::Printf(TEXT("(BuildableData: %s)"), *BuildData.RowName.ToString());
			const FPDBuildable* Buildable = BuildData.GetRow<FPDBuildable>(BuildDatumCtxtStr);
			if (Buildable == nullptr || (Buildable->BuildableTag != NewSelectedBuildable))
			{
				continue;
			}

			// @todo nasty inner search, will need to @todo @refactor
			const TArray<UObject*>& ListItems = Buildables->GetListItems();
			bool bWasAlreadyDeselected = false;
			for (UObject* const& Item : ListItems)
			{
				const UPDBuildableEntry* AsBuildableEntry = Buildables->GetEntryWidgetFromItem<UPDBuildableEntry>(Item);
				if (AsBuildableEntry == nullptr || AsBuildableEntry->IsValidLowLevelFast() == false)
				{
					UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildWidgetBase::SelectBuildable -- Context(%s) -- AsBuildableEntry: %i"), *LoadedContext->ContextReadableName.ToString(), AsBuildableEntry != nullptr)
					continue;
				}
				if (AsBuildableEntry->BuildableTag == NewSelectedBuildable )
				{
					bWasAlreadyDeselected = AsBuildableEntry->bIsSelected != true;
				}
			}
			
			bRequestedBuildableWasValid = LastSelectedBuildableTag != NewSelectedBuildable || bWasAlreadyDeselected;
			UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildWidgetBase::SelectBuildable -- bRequestedBuildableWasValid: %i"), bRequestedBuildableWasValid)
			break; 
		}
	}

	UpdateSelectedBuildable(NewSelectedBuildable, bRequestedBuildableWasValid);

	// set after updating tag
	LastSelectedBuildableTag = bRequestedBuildableWasValid ? NewSelectedBuildable : LastSelectedBuildableTag;
}

void UPDBuildWidgetBase::UpdateSelectedBuildable(const FGameplayTag& RequestToSelectTag, const bool bRequestedBuildableWasValid)
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildWidgetBase::UpdateSelectedBuildable - Buildable Tag (%s)"), *RequestToSelectTag.GetTagName().ToString())
	
	// Is it already selected?
	FGameplayTag FinalTagSelection = FGameplayTag::EmptyTag;
	ERTSBuildMenuModules SelectedAction = ERTSBuildMenuModules::DeselectBuildable;
	for (const FDataTableRowHandle& Context : CurrentBuildableContexts)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildWidgetBase(%s)::UpdateSelectedBuildable(ContextRow: %s)"), *GetName(), *Context.RowName.ToString());
		const FPDBuildContext* LoadedContext = Context.GetRow<FPDBuildContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag.IsValid() == false)
		{
			/** @todo Output to log with warning or error level verbosity if the context or the tag was invalid */
			UE_LOG(PDLog_RTSBase, Warning, TEXT("UPDBuildWidgetBase::UpdateSelectedBuildable -- Context is null or has invalid flag"))
			continue;
		}
		
		// Resets any previous selected tints
		const TArray<UObject*>& ListItems = Buildables->GetListItems();
		for (UObject* const& Item : ListItems)
		{
			UPDBuildableEntry* AsBuildableEntry = Buildables->GetEntryWidgetFromItem<UPDBuildableEntry>(Item);
			if (AsBuildableEntry == nullptr || AsBuildableEntry->IsValidLowLevelFast() == false)
			{
				UE_LOG(PDLog_RTSBase, Warning, TEXT("UPDBuildWidgetBase::UpdateSelectedBuildable -- Context(%s) -- AsBuildableEntry: %i"), *LoadedContext->ContextReadableName.ToString(), AsBuildableEntry != nullptr)
				continue;
			}

			const bool bWidgetFlairValid = SelectedWidgetFlair != nullptr;
			if (bWidgetFlairValid == false)
			{
				UE_LOG(PDLog_RTSBase, Warning, TEXT("UPDBuildWidgetBase::UpdateSelectedBuildable - Widget of type 'UPDBuildingActionsWidgetBase' does not have a set 'WidgetFlair'"))
			}			

			const bool bWasSelected = bRequestedBuildableWasValid && (RequestToSelectTag == AsBuildableEntry->BuildableTag);
			FSlateBrush& Brush = AsBuildableEntry->TextContentBorder->Background;
			Brush.TintColor = FSlateColor(bWidgetFlairValid ? SelectedWidgetFlair->NotSelectedBuildableTint: Brush.TintColor);

			// Selected new tag, if deselection it will not enter this @todo clean things up here
			if (bWasSelected)
			{
				UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildWidgetBase::UpdateSelectedBuildable -- Selecting buildable(%s)"), *AsBuildableEntry->BuildableTag.GetTagName().ToString())
				FinalTagSelection = RequestToSelectTag;
				SelectedAction = ERTSBuildMenuModules::SelectBuildable;
				Brush.TintColor = FSlateColor(bWidgetFlairValid ? SelectedWidgetFlair->SelectedBuildableTint: Brush.TintColor);
				AsBuildableEntry->bIsSelected = true;
			}
			else
			{
				UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildWidgetBase::UpdateSelectedBuildable -- Deselecting buildable(%s)"), *AsBuildableEntry->BuildableTag.GetTagName().ToString())				
				AsBuildableEntry->bIsSelected = false;
			}
			
			AsBuildableEntry->TextContentBorder->SetBrush(Brush);
		}
	}
	
	// This has been checked in mouse input event, if it is called manually it is up to the caller to have checked this beforehand
	APawn* CachedOwner = GetOwningPlayer()->GetPawnOrSpectator();
	if (ensure(CachedOwner != nullptr && CachedOwner->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass())))
	{
		IPDRTSBuilderInterface::Execute_SelectBuildMenuEntry(CachedOwner, SelectedAction, FinalTagSelection);
	}
}

void UPDBuildWidgetBase::SpawnWorkerBuildMenu(const FPDBuildWorker& BuildWorker)
{
	FPDBuildContextParam ContextParams;
	ContextParams.NewBuildableContext = BuildWorker.GrantedContexts;

	AddToViewport();
	Buildables->SetRenderOpacity(0.0);
	BuildContexts->SetRenderOpacity(0.0);
	
	OverwriteBuildContext(ContextParams);
	LoadBuildContexts();
	PlayAnimation(OpenWidget);

	bIsMenuVisible = true;
}

void UPDBuildWidgetBase::BeginCloseWorkerBuildMenu()
{
	if (CloseWidget == nullptr || CloseWidget->IsValidLowLevelFast() == false || CloseWidget->GetEndTime() < SMALL_NUMBER)
	{
		RemoveFromParent();
		bIsMenuVisible = false;
		return;
	}
	
	// When animation finished
	FWidgetAnimationDynamicEvent Delegate;
	Delegate.BindUFunction(this, TEXT("EndCloseWorkerBuildMenu"));
	BindToAnimationFinished(CloseWidget, Delegate);
	PlayAnimation(CloseWidget);
}

void UPDBuildWidgetBase::EndCloseWorkerBuildMenu()
{
	APawn* CachedOwner = GetOwningPlayer()->GetPawnOrSpectator();
	if (CachedOwner != nullptr && CachedOwner->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()))
	{
		IPDRTSBuilderInterface::Execute_SelectBuildMenuEntry(CachedOwner, ERTSBuildMenuModules::DeselectBuildable, FGameplayTag());
	}
	
	bIsMenuVisible = false;
	RemoveFromParent();
}

void UPDBuildWidgetBase::OpenWorkerContextMenu()
{
	PlayAnimation(OpenContextMenu);	
}

void UPDBuildWidgetBase::BeginCloseWorkerContext()
{
	if (CloseContextMenu == nullptr || CloseContextMenu->IsValidLowLevelFast() == false || CloseContextMenu->GetEndTime() < SMALL_NUMBER)
	{
		RemoveFromParent();
		return;
	}
	
	// When animation finished
	FWidgetAnimationDynamicEvent Delegate;
	Delegate.BindUFunction(this, TEXT("EndCloseWorkerBuildMenu"));
	BindToAnimationFinished(CloseContextMenu, Delegate);
	PlayAnimation(CloseContextMenu);
}

void UPDBuildWidgetBase::EndCloseWorkerContext()
{
	APawn* CachedOwner = GetOwningPlayer()->GetPawnOrSpectator();
	if (CachedOwner != nullptr && CachedOwner->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()))
	{
		IPDRTSBuilderInterface::Execute_SelectBuildMenuEntry(CachedOwner, ERTSBuildMenuModules::DeselectContext, FGameplayTag());
		IPDRTSBuilderInterface::Execute_SelectBuildMenuEntry(CachedOwner, ERTSBuildMenuModules::DeselectBuildable, FGameplayTag());
	}	
}

//
// Buildable Actions -- Actions granted to a buildable, (buildable ghosts will possibly have fewer actions, aborting the build for example, or none at all)
void UPDBuildableActionEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UPDBuildableActionWrapper* Item = Cast<UPDBuildableActionWrapper>(ListItemObject);
	if (Item == nullptr) { return; }
	
	// 1. Read data
	ActionTitle->SetText(Item->GetBuildableTitle());
	bCanBuild             = Item->GetCanBuild();
	DirectParentReference = Item->GetDirectParentReference();
	ParentMenuContextTag  = Item->GetParentContextTag();
	ActionTag             = Item->GetActionTag();
	SetOwningPlayer(DirectParentReference->GetOwningPlayer());
	
	// 2. Set data-asset variable
	TArray<FDataTableRowHandle>& BuildContexts = DirectParentReference->GetCurrentActionContexts();
	for (const FDataTableRowHandle& Context : BuildContexts)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildableActionEntry(%s)::NativeOnListItemObjectSet(ContextRow: %s)"), *GetName(), *Context.RowName.ToString());
		const FPDBuildActionContext* LoadedContext = Context.GetRow<FPDBuildActionContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag.IsValid() == false || LoadedContext->ContextTag != ParentMenuContextTag)
		{
			/** @todo Output to log with warning or error level verbosity if the context or the tag was invalid */ 
			continue;
		}

		const UPDBuildableDataAsset* BuildDataAsset = nullptr;
		const TArray<FDataTableRowHandle>& ActionData = LoadedContext->ActionData;
		for (const FDataTableRowHandle& ActionDatumHandle : ActionData)
		{
			const FString BuildDatumCtxtStr = CtxtStr + FString::Printf(TEXT("(BuildableData: %s)"), *ActionDatumHandle.RowName.ToString());
			const FPDBuildAction* Action = ActionDatumHandle.GetRow<FPDBuildAction>(BuildDatumCtxtStr);
			if (Action == nullptr || Action->ActionTag != ActionTag)
			{
				continue;
			}
			
			BuildDataAsset = Action->DABuildAsset;
			break;
		}
		
		if (BuildDataAsset != nullptr)
		{
			UMaterialInstance* MatInst = BuildDataAsset->Buildable_MaterialInstance;
			UTexture2D* Tex = BuildDataAsset->Buildable_Texture;
			if (MatInst != nullptr)
			{
				TextContentBorder->SetBrushFromMaterial(MatInst);
			}
			else if (Tex != nullptr) // Fallback to texture
			{
				TextContentBorder->SetBrushFromTexture(Tex);
			}
		}
	}
	
	// 3. Clear and Bind delegates
	Hitbox->OnClicked.Clear();
	Hitbox->OnClicked.AddDynamic(this, &UPDBuildableActionEntry::OnClicked);

	Hitbox->OnReleased.Clear();
	Hitbox->OnReleased.AddDynamic(this, &UPDBuildableActionEntry::OnReleased);
	
	Hitbox->OnHovered.Clear();
	Hitbox->OnHovered.AddDynamic(this, &UPDBuildableActionEntry::OnHovered);
	
	Hitbox->OnUnhovered.Clear();
	Hitbox->OnUnhovered.AddDynamic(this, &UPDBuildableActionEntry::OnUnhovered);

	Hitbox->OnPressed.Clear();
	Hitbox->OnPressed.AddDynamic(this, &UPDBuildableActionEntry::OnPressed);
	
}

void UPDBuildableActionEntry::OnPressed()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildableActionEntry::OnPressed"))

	const APawn* CachedOwner = GetOwningPlayer()->GetPawnOrSpectator();
	if (CachedOwner == nullptr)
	{
		/** @todo @test also write tests for this */
		UE_LOG(PDLog_RTSBase, Error, TEXT("UPDBuildableActionEntry::OnPressed - Player owning pawn is not set!"))
		return;
	}
	
	if (CachedOwner->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()) == false)
	{
		/** @todo @test also write tests for this */
		UE_LOG(PDLog_RTSBase, Error, TEXT("UPDBuildableActionEntry::OnPressed - Player pawn class does not implement 'PDRTSBuilderInterface' or object has been sliced."))
		
		return;
	}

	// tell DirectParentReference about the clicked button 
	DirectParentReference->SelectAction(ActionTag);
}

void UPDBuildableActionEntry::OnHovered()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildableActionEntry::OnHovered"))
	/** @todo: write impl.*/	
}

void UPDBuildableActionEntry::OnUnhovered()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildableActionEntry::OnUnhovered"))
	/** @todo: write impl.*/	
}

void UPDBuildableActionEntry::OnClicked()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildableActionEntry::OnClicked"))
	/** @todo: write impl.*/	
}

void UPDBuildableActionEntry::OnReleased()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildableActionEntry::OnReleased"))
	/** @todo: write impl.*/		
}

void UPDBuildActionContextEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	const UPDBuildActionContextWrapper* Item = Cast<UPDBuildActionContextWrapper>(ListItemObject);
	if (Item == nullptr) { return; }
	
	// 1. Read data
	ContextTitle->SetText(Item->GetBuildContextTitle());
	SelfContextTag        = Item->GetContextTag();
	DirectParentReference = Item->GetDirectParentReference();
	SetOwningPlayer(DirectParentReference->GetOwningPlayer());

	// 2. Set data-asset variable
	TArray<FDataTableRowHandle>& BuildContexts = DirectParentReference->GetCurrentActionContexts();
	for (const FDataTableRowHandle& Context : BuildContexts)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildActionContextEntry(%s)::NativeOnListItemObjectSet(ContextRow: %s)"), *GetName(), *Context.RowName.ToString());
		const FPDBuildActionContext* LoadedContext = Context.GetRow<FPDBuildActionContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag.IsValid() == false)
		{
			/** @todo Output to log with warning or error level verbosity if the context or the tag was invalid */ 
			continue;
		}

		const UPDBuildContextDataAsset* BuildDataAsset = LoadedContext->ContextData.DABuildContextAsset;
		if (BuildDataAsset != nullptr)
		{
			UMaterialInstance* MatInst = BuildDataAsset->BuildContext_MaterialInstance;
			UTexture2D* Tex = BuildDataAsset->BuildContext_Texture;
			if (MatInst != nullptr)
			{
				TextContentBorder->SetBrushFromMaterial(MatInst);
			}
			else if (Tex != nullptr) // Fallback to texture
			{
				TextContentBorder->SetBrushFromTexture(Tex);
			}
		}
	}

	// 3 Clear and Bind delegates
	Hitbox->OnClicked.Clear();
	Hitbox->OnClicked.AddDynamic(this, &UPDBuildActionContextEntry::OnClicked);

	Hitbox->OnReleased.Clear();
	Hitbox->OnReleased.AddDynamic(this, &UPDBuildActionContextEntry::OnReleased);

	Hitbox->OnHovered.Clear();
	Hitbox->OnHovered.AddDynamic(this, &UPDBuildActionContextEntry::OnHovered);

	Hitbox->OnUnhovered.Clear();
	Hitbox->OnUnhovered.AddDynamic(this, &UPDBuildActionContextEntry::OnUnhovered);

	Hitbox->OnPressed.Clear();
	Hitbox->OnPressed.AddDynamic(this, &UPDBuildActionContextEntry::OnPressed);	
}

void UPDBuildActionContextEntry::OnPressed()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildActionContextEntry::OnPressed"))
	APawn* CachedOwner = GetOwningPlayer()->GetPawnOrSpectator();
	if (CachedOwner == nullptr)
	{
		/** @todo @test also write tests for this */
		UE_LOG(PDLog_RTSBase, Error, TEXT("UPDBuildActionContextEntry::OnPressed - Player owning pawn is not set!"))
		
		return;
	}
	
	if (CachedOwner->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()) == false)
	{
		/** @todo @test also write tests for this */
		UE_LOG(PDLog_RTSBase, Error, TEXT("UPDBuildActionContextEntry::OnPressed - Player pawn class does not implement 'PDRTSBuilderInterface' or object has been sliced."))
		
		return;
	}
	ensure(DirectParentReference != nullptr && "if you hit this ensure´then you've forgotten to add an appropriate parent reference upon spawning the widget");

	IPDRTSBuilderInterface::Execute_SelectActionMenuEntry(CachedOwner, ERTSBuildableActionMenuModules::SelectBuildableActionContext, SelfContextTag, TArray<uint8>{});
	if (DirectParentReference->SelectedWidgetFlair == nullptr)
	{
		/** @todo: log about widget flair not being set.*/
		UE_LOG(PDLog_RTSBase, Warning, TEXT("UPDBuildActionContextEntry::OnPressed - Parent widget of type 'UPDBuildingActionsWidgetBase' does not have a set 'widget flair'"))
	}

	// tell DirectParentReference about the clicked button 
	DirectParentReference->UpdateSelectedActionContext(SelfContextTag);
}

void UPDBuildActionContextEntry::OnHovered()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildActionContextEntry::OnHovered"))
	/** @todo: write impl.*/
}

void UPDBuildActionContextEntry::OnUnhovered()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildActionContextEntry::OnUnhovered"))
	/** @todo: write impl.*/	
}

void UPDBuildActionContextEntry::OnClicked()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildActionContextEntry::OnClicked"))
	/** @todo: write impl.*/
}

void UPDBuildActionContextEntry::OnReleased()
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildActionContextEntry::OnReleased"))
	/** @todo: write impl.*/	
}

//
// Buildables action menu, the menu that shows when selecting a buildable

void UPDBuildingActionsWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	CurrentActionContextHandles = DefaultActionContexts;
	
	SelectedWidgetFlair = BuildWidgetFlairHandle.GetRow<FPDSharedBuildWidgetFlair>("");
}

void UPDBuildingActionsWidgetBase::OverwriteActionContext(const FPDBuildActionContextParam& NewRowParams)
{
	CurrentActionContextHandles = NewRowParams.NewBuildActionContexts;
}

void UPDBuildingActionsWidgetBase::LoadActionContexts()
{
	CountSelector->SetVisibility(ESlateVisibility::Hidden);
	
	// Build the context menu entries (Category)
	ActionContexts->ClearListItems();

	FGameplayTag SelectedContextTag = FGameplayTag::EmptyTag;
	bool bLastSelectedContextStillValid = false;
	for (const FDataTableRowHandle& Context : CurrentActionContextHandles)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildingActionsWidgetBase(%s)::LoadActionContexts(ContextRow: %s)"), *GetName(), *Context.RowName.ToString());
		FPDBuildActionContext* LoadedContext = Context.GetRow<FPDBuildActionContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag.IsValid() == false)
		{
			/** @todo Output to log with warning or error level verbosity if the context or the tag was invalid */ 
			continue;
		}
		
		UPDBuildActionContextWrapper* DataWrapper = NewObject<UPDBuildActionContextWrapper>(this, UPDBuildActionContextWrapper::StaticClass());		

		FText SelectedName = LoadedContext->ContextData.ReadableName.IsEmpty() ?
			FText::FromName(LoadedContext->ContextTag.GetTagName()) : LoadedContext->ContextData.ReadableName;

		TArray<FDataTableRowHandle>& InnerBuildableHandles = LoadedContext->ActionData; 
		
		DataWrapper->AssignData(SelectedName, LoadedContext->ContextTag, InnerBuildableHandles, this);
		ActionContexts->AddItem(DataWrapper);
		
		SelectedContextTag = SelectedContextTag.IsValid() == false ? LoadedContext->ContextTag : SelectedContextTag;
		bLastSelectedContextStillValid &= LastSelectedActionContextTag == LoadedContext->ContextTag;
	}
	
	SelectedContextTag = bLastSelectedContextStillValid ? LastSelectedActionContextTag : SelectedContextTag;
	SelectActionContext(SelectedContextTag, true);	
}

void UPDBuildingActionsWidgetBase::SelectActionContext(const FGameplayTag& NewSelectedContext, const bool bWasDeselected)
{
	bool bRequestedContextWasValid = false;
	for (const FDataTableRowHandle& Context : CurrentActionContextHandles)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildingActionsWidgetBase(%s)::SelectActionContext(ContextRow: %s)"), *GetName(), *Context.RowName.ToString());
		FPDBuildActionContext* LoadedContext = Context.GetRow<FPDBuildActionContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag != NewSelectedContext) { continue; }

		bRequestedContextWasValid = true;
		
		Actions->ClearListItems();
		CountSelector->SetVisibility(ESlateVisibility::Hidden);
		if (bWasDeselected) // deselecting a category should intuitively deselect any selected entry
		{
			APawn* CachedOwner = GetOwningPlayer()->GetPawnOrSpectator();
			if (ensure(CachedOwner != nullptr && CachedOwner->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass())))
			{
				IPDRTSBuilderInterface::Execute_SelectActionMenuEntry(CachedOwner, ERTSBuildableActionMenuModules::DeselectBuildableActionContext, FGameplayTag::EmptyTag, TArray<uint8>{});
			}
			return;
		}
		
		for (const FDataTableRowHandle& ActionDatum : LoadedContext->ActionData)
		{
			const FString ActionCtxtStr = FString::Printf(TEXT("UPDBuildingActionsWidgetBase(%s)::SelectActionContext(ActionDatum: %s)"), *GetName(), *ActionDatum.RowName.ToString());
			const FPDBuildAction* Action = ActionDatum.GetRow<FPDBuildAction>(ActionCtxtStr);
			if (Action == nullptr) { continue; }

			UPDBuildableActionWrapper* DataWrapper = NewObject<UPDBuildableActionWrapper>(this, UPDBuildableActionWrapper::StaticClass());
			
			FGameplayTag ActionTag = Action->ActionTag;

			constexpr bool bCanBuild = true; // @todo extrapolate bCanBuild based on available resources and such, possibly control via a virtual function and define in game module
			DataWrapper->AssignData(Action->ReadableName, bCanBuild, ActionTag, LoadedContext->ContextTag, this);
			Actions->AddItem(DataWrapper);
		}

		//
		// Make selector visible if we have at-least one buildable action visible
		if (Actions->GetListItems().IsEmpty() == false)
		{
			// @todo replace hardocoded values here 
			CountSelector->ApplySettings(FPDRangedSliderSettings{
					1,
					250,
					1.0, 
					1.0});
			CountSelector->RangedSlider->SetValue(1);		
			CountSelector->SetVisibility(ESlateVisibility::Visible);
		}
		
		break;
	}
	
	LastSelectedActionContextTag = bRequestedContextWasValid ? NewSelectedContext : LastSelectedActionContextTag;	
}

void UPDBuildingActionsWidgetBase::UpdateSelectedActionContext(const FGameplayTag& RequestToSelectTag)
{
	// Is it already selected?
	bool bWasContextSelected = false;
	for (const FDataTableRowHandle& Context : CurrentActionContextHandles)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildingActionsWidgetBase(%s)::UpdateSelectedActionContext(ContextRow: %s)"), *GetName(), *Context.RowName.ToString());
		const FPDBuildActionContext* LoadedContext = Context.GetRow<FPDBuildActionContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag.IsValid() == false)
		{
			/** @todo Output to log with warning or error level verbosity if the context or the tag was invalid */ 
			continue;
		}
		
		// Resets any previous selected tints
		const TArray<UObject*>& ListItems = ActionContexts->GetListItems();
		for (UObject* const& Item : ListItems)
		{
			UPDBuildActionContextEntry* AsContextEntry = ActionContexts->GetEntryWidgetFromItem<UPDBuildActionContextEntry>(Item);
			if (AsContextEntry == nullptr || AsContextEntry->IsValidLowLevelFast() == false)
			{
				continue;
			}
			
			const bool bWidgetFlairValid = SelectedWidgetFlair != nullptr;
			if (bWidgetFlairValid == false)
			{
				/** @todo: log about widget flair not being set.*/
				UE_LOG(PDLog_RTSBase, Warning, TEXT("UPDBuildingActionsWidgetBase::UpdateSelectedContext - Widget of type 'UPDBuildingActionsWidgetBase' does not have a set 'WidgetFlair'"))
			}
			
			// Selected new tag, @todo clean things up here
			FSlateBrush& Brush = AsContextEntry->TextContentBorder->Background;
			const FString& ContextEntryString = AsContextEntry->SelfContextTag.GetTagName().ToString();
			const bool bShouldSelect = (RequestToSelectTag == AsContextEntry->SelfContextTag) && AsContextEntry->bIsSelected == false;
			if (bShouldSelect)
			{
				UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildingActionsWidgetBase::UpdateSelectedContext -- Selecting context(%s)"), *ContextEntryString)
				
				Brush.TintColor = FSlateColor(bWidgetFlairValid ? SelectedWidgetFlair->SelectedContextTint: Brush.TintColor);
				bWasContextSelected = AsContextEntry->bIsSelected = true;
			}
			else // Deselecting any entries we had selected before // @todo redesign this slightly ,as we only need to know the previously selected entry, we should cache it so we can skip this loop
			{
				UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildWidgetBase::UpdateSelectedContext -- Deselecting context(%s)"), *ContextEntryString)				
				Brush.TintColor = FSlateColor(bWidgetFlairValid ? SelectedWidgetFlair->NotSelectedContextTint : Brush.TintColor);
				bWasContextSelected = AsContextEntry->bIsSelected = false;
			}
			AsContextEntry->TextContentBorder->SetBrush(Brush);
		}
	}
	
	SelectActionContext(RequestToSelectTag, bWasContextSelected == false);	
}

void UPDBuildingActionsWidgetBase::SelectAction(const FGameplayTag& NewSelectedAction)
{
	bool bRequestedActionWasValid = false;
	for (const FDataTableRowHandle& Context : CurrentActionContextHandles)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildingActionsWidgetBase(%s)::SelectAction(ContextRow: %s)"), *GetName(), *Context.RowName.ToString());
		FPDBuildActionContext* LoadedContext = Context.GetRow<FPDBuildActionContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag.IsValid() == false) { continue; }
		
		for (const FDataTableRowHandle& ActionDatum : LoadedContext->ActionData)
		{
			const FString ActionDatumCtxtStr = CtxtStr + FString::Printf(TEXT("(ActionDatum: %s)"), *ActionDatum.RowName.ToString());
			const FPDBuildAction* Buildable = ActionDatum.GetRow<FPDBuildAction>(ActionDatumCtxtStr);
			if (Buildable == nullptr || (Buildable->ActionTag != NewSelectedAction))
			{
				continue;
			}

			// @todo nasty inner search, will need to @todo @refactor
			const TArray<UObject*>& ListItems = Actions->GetListItems();
			bool bWasAlreadyDeselected = false;
			for (UObject* const& Item : ListItems)
			{
				const UPDBuildableActionEntry* AsBuildableEntry = Actions->GetEntryWidgetFromItem<UPDBuildableActionEntry>(Item);
				if (AsBuildableEntry == nullptr || AsBuildableEntry->IsValidLowLevelFast() == false)
				{
					UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildingActionsWidgetBase::SelectAction -- Context(%s) -- AsBuildableEntry: %i"), *LoadedContext->ContextData.ReadableName.ToString(), AsBuildableEntry != nullptr)
					continue;
				}
				if (AsBuildableEntry->ActionTag == NewSelectedAction )
				{
					bWasAlreadyDeselected = AsBuildableEntry->bIsSelected != true;
				}
			}
			
			bRequestedActionWasValid = LastSelectedActionTag != NewSelectedAction || bWasAlreadyDeselected;
			UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildingActionsWidgetBase::SelectAction -- bRequestedActionWasValid: %i"), bRequestedActionWasValid)
			break; 
		}
	}

	UpdateSelectedAction(NewSelectedAction, bRequestedActionWasValid);

	// set after updating tag
	LastSelectedActionTag = bRequestedActionWasValid ? NewSelectedAction : LastSelectedActionTag;	
}


void UPDBuildingActionsWidgetBase::UpdateSelectedAction(const FGameplayTag& RequestToSelectTag, const bool bRequestedBuildableWasValid)
{
	UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildingActionsWidgetBase::UpdateSelectedBuildable - Buildable Tag (%s)"), *RequestToSelectTag.GetTagName().ToString())
	
	// Is it already selected?
	FGameplayTag FinalTagSelection = FGameplayTag::EmptyTag;
	ERTSBuildableActionMenuModules SelectedActionMode = ERTSBuildableActionMenuModules::DoNothing;
	for (const FDataTableRowHandle& Context : CurrentActionContextHandles)
	{
		const FString CtxtStr = FString::Printf(TEXT("UPDBuildingActionsWidgetBase(%s)::UpdateSelectedAction(ContextRow: %s)"), *GetName(), *Context.RowName.ToString());
		const FPDBuildActionContext* LoadedContext = Context.GetRow<FPDBuildActionContext>(CtxtStr);
		if (LoadedContext == nullptr || LoadedContext->ContextTag.IsValid() == false)
		{
			/** @todo Output to log with warning or error level verbosity if the context or the tag was invalid */
			UE_LOG(PDLog_RTSBase, Warning, TEXT("UPDBuildingActionsWidgetBase::UpdateSelectedBuildable -- Context is null or has invalid flag"))
			continue;
		}
		
		// Resets any previous selected tints
		const TArray<UObject*>& ListItems = Actions->GetListItems();
		for (UObject* const& Item : ListItems)
		{
			UPDBuildableActionEntry* AsBuildableActionEntry = Actions->GetEntryWidgetFromItem<UPDBuildableActionEntry>(Item);
			if (AsBuildableActionEntry == nullptr || AsBuildableActionEntry->IsValidLowLevelFast() == false)
			{
				UE_LOG(PDLog_RTSBase, Warning, TEXT("UPDBuildingActionsWidgetBase::UpdateSelectedBuildable -- Context(%s) -- AsBuildableEntry: %i"), *LoadedContext->ContextData.ReadableName.ToString(), AsBuildableActionEntry != nullptr)
				continue;
			}
			const bool bWidgetFlairValid = SelectedWidgetFlair != nullptr;
			if (bWidgetFlairValid == false)
			{
				UE_LOG(PDLog_RTSBase, Warning, TEXT("UPDBuildingActionsWidgetBase::UpdateSelectedBuildable - Widget of type 'UPDBuildingActionsWidgetBase' does not have a set 'WidgetFlair'"))
			}			
			

			const bool bWasSelected = bRequestedBuildableWasValid && (RequestToSelectTag == AsBuildableActionEntry->ActionTag);
			FSlateBrush& Brush = AsBuildableActionEntry->TextContentBorder->Background;
			Brush.TintColor = FSlateColor(bWidgetFlairValid ? SelectedWidgetFlair->NotSelectedBuildableTint : Brush.TintColor);

			// Selected new tag, if deselection it will not enter this @todo clean things up here
			if (bWasSelected)
			{
				UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildingActionsWidgetBase::UpdateSelectedBuildable -- Selecting buildable(%s)"), *AsBuildableActionEntry->ActionTag.GetTagName().ToString())
				FinalTagSelection = RequestToSelectTag;
				SelectedActionMode = ERTSBuildableActionMenuModules::FireBuildableAction;
				Brush.TintColor = FSlateColor(bWidgetFlairValid ? SelectedWidgetFlair->SelectedBuildableTint : Brush.TintColor);
				AsBuildableActionEntry->bIsSelected = true;
			}
			else
			{
				UE_LOG(PDLog_RTSBase, Verbose, TEXT("UPDBuildingActionsWidgetBase::UpdateSelectedBuildable -- Deselecting buildable(%s)"), *AsBuildableActionEntry->ActionTag.GetTagName().ToString())				
				AsBuildableActionEntry->bIsSelected = false;
			}
			
			AsBuildableActionEntry->TextContentBorder->SetBrush(Brush);
		}
	}
	
	// This has been checked in mouse input event, if it is called manually it is up to the caller to have checked this beforehand
	APawn* CachedOwner = GetOwningPlayer()->GetPawnOrSpectator();
	if (ensure(CachedOwner != nullptr && CachedOwner->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass())))
	{
		const TArray<uint8> Payload
		{
			MASK_BYTE(CountSelector->SelectedCount, 0),
			MASK_BYTE(CountSelector->SelectedCount, 1), 
			MASK_BYTE(CountSelector->SelectedCount, 2),
			MASK_BYTE(CountSelector->SelectedCount, 3)
		};

		UE_LOG(PDLog_RTSBase, Warning, TEXT("UPDBuildingActionsWidgetBase::UpdateSelectedAction -- Selected Count: %i"), CountSelector->SelectedCount);

		
		IPDRTSBuilderInterface::Execute_SelectActionMenuEntry(CachedOwner, SelectedActionMode, FinalTagSelection, Payload);
	}	
}

void UPDBuildingActionsWidgetBase::SpawnBuildableActionMenu(const FPDBuildable& BuildableToSourceActionsFrom)
{
	FPDBuildActionContextParam ContextParams;
	ContextParams.NewBuildActionContexts = BuildableToSourceActionsFrom.ActionContextHandles;
	
	AddToViewport();
	Actions->SetRenderOpacity(0.0);
	ActionContexts->SetRenderOpacity(0.0);
	
	OverwriteActionContext(ContextParams);
	LoadActionContexts();
	PlayAnimation(OpenWidget);

	bIsMenuVisible = true;	
}

void UPDBuildingActionsWidgetBase::BeginCloseBuildableActionMenu()
{
	if (CloseWidget == nullptr || CloseWidget->IsValidLowLevelFast() == false || CloseWidget->GetEndTime() < SMALL_NUMBER)
	{
		RemoveFromParent();
		bIsMenuVisible = false;
		return;
	}
	
	// When animation finished
	FWidgetAnimationDynamicEvent Delegate;
	Delegate.BindUFunction(this, TEXT("EndCloseBuildableActionMenu"));
	BindToAnimationFinished(CloseWidget, Delegate);
	PlayAnimation(CloseWidget);	
}

void UPDBuildingActionsWidgetBase::EndCloseBuildableActionMenu()
{
	APawn* CachedOwner = GetOwningPlayer()->GetPawnOrSpectator();
	if (CachedOwner != nullptr && CachedOwner->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()))
	{
		IPDRTSBuilderInterface::Execute_SelectActionMenuEntry(CachedOwner, ERTSBuildableActionMenuModules::DoNothing, FGameplayTag(), TArray<uint8>{});
	}
	
	bIsMenuVisible = false;
	RemoveFromParent();	
}

void UPDBuildingActionsWidgetBase::OpenActionContextMenu()
{
	PlayAnimation(OpenContextMenu);	
}

void UPDBuildingActionsWidgetBase::BeginCloseActionContext()
{
	if (CloseContextMenu == nullptr || CloseContextMenu->IsValidLowLevelFast() == false || CloseContextMenu->GetEndTime() < SMALL_NUMBER)
	{
		RemoveFromParent();
		return;
	}
	
	// When animation finished
	FWidgetAnimationDynamicEvent Delegate;
	Delegate.BindUFunction(this, TEXT("EndCloseActionContext"));
	BindToAnimationFinished(CloseContextMenu, Delegate);
	PlayAnimation(CloseContextMenu);
}

void UPDBuildingActionsWidgetBase::EndCloseActionContext()
{
	APawn* CachedOwner = GetOwningPlayer()->GetPawnOrSpectator();
	if (CachedOwner != nullptr && CachedOwner->GetClass()->ImplementsInterface(UPDRTSBuilderInterface::StaticClass()))
	{
		IPDRTSBuilderInterface::Execute_SelectActionMenuEntry(CachedOwner, ERTSBuildableActionMenuModules::DeselectBuildableActionContext, FGameplayTag(), TArray<uint8>{});
	}
	
	bIsMenuVisible = false;
	RemoveFromParent();	
}

void UPDBuildingActionsWidgetBase::SetNewWorldActor(AActor* NewWorldActor, const FPDBuildable& BuildableToSourceActionsFrom)
{
	CurrentWorldActor = NewWorldActor;
	if (CurrentWorldActor == nullptr || BuildableToSourceActionsFrom.BuildableTag.IsValid() == false)
	{
		BeginCloseBuildableActionMenu();
		return;
	}
	SpawnBuildableActionMenu(BuildableToSourceActionsFrom);
}

void UPDBuildingActionsWidgetBase::DestroyCurrentWorldActor() const
{
	if (CurrentWorldActor == nullptr || CurrentWorldActor->IsActorBeingDestroyed())
	{
		return;
	}
	
	CurrentWorldActor->Destroy();
	UPDBuildingActionsWidgetBase* MutableSelf = const_cast<UPDBuildingActionsWidgetBase*>(this);
	MutableSelf->SetNewWorldActor(nullptr, FPDBuildable{});
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
