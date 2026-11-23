/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "PDRTSSharedUI.generated.h"

class UPDRangedSelector;
class UPDGenericDialog;
struct FPDBuildable;
struct FPDBuildWorker;
class UImage;
class UTextBlock;
struct FPDBuildContext;

DECLARE_LOG_CATEGORY_CLASS(PDLog_RTSBaseUI, Log, All);

DECLARE_DELEGATE_OneParam(FPDOnIntValueChanged, int32);

//
// Build widget wrappers

/** @brief Tile view object type for loading buildable data */
UCLASS(Blueprintable)
class UPDBuildContextWrapper : public UObject
{
	GENERATED_BODY()
public:
	
	/** @brief Self-explanatory */
	void AssignData(
		const FText& InBuildableTitle,
		const FGameplayTag& InContextTag,
		const TArray<FDataTableRowHandle>& InInnerBuildableHandles,
		class UPDBuildWidgetBase* InDirectParentReference)
	{
		BuildContextTitle = InBuildableTitle;
		ContextTag = InContextTag;
		InnerBuildableHandles = InInnerBuildableHandles;
		DirectParentReference = InDirectParentReference;
	}

	/** @brief Immutable access to private member BuildContextTitle */
	UFUNCTION(BlueprintCallable)
	const FText& GetBuildContextTitle() const { return BuildContextTitle; };

	/** @brief Immutable access to private member InnerBuildableTags */
	UFUNCTION(BlueprintCallable)
	const FGameplayTag& GetContextTag() const { return ContextTag; };	
	
	/** @brief Immutable access to private member InnerBuildableTags */
	UFUNCTION(BlueprintCallable)
	const TArray<FDataTableRowHandle>& GetBuildableHandles() const { return InnerBuildableHandles; };	

	/** @brief Mutable access to private member ptr DirectParentReference */
	UFUNCTION(BlueprintCallable)
	class UPDBuildWidgetBase* GetDirectParentReference() const { return DirectParentReference; };

private:
	/** @brief Assigned by 'AssignData', retrieved by 'GetBuildableTitle'  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	FText BuildContextTitle{};

	/** @brief Assigned by 'AssignData', retrieved by 'GetBuildableTag'  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	FGameplayTag ContextTag{};
	
	/** @brief Assigned by 'AssignData', retrieved by 'GetBuildableTag'  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	TArray<FDataTableRowHandle> InnerBuildableHandles;

	/** @brief Assigned by 'AssignData', retrieved by 'GetDirectParentReference'  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	class UPDBuildWidgetBase* DirectParentReference = nullptr;
};

/** @brief Tile view object type for loading buildable data */
UCLASS(Blueprintable)
class UPDBuildableWrapper : public UObject
{
	GENERATED_BODY()
public:
	
	/** @brief Used to assign data to the wrapper before it is passed into a list-view and/or tile-view */
	void AssignData(
		const FText& InBuildableTitle,
		const bool bInCanBuild,
		const FGameplayTag& InBuildableTag,
		const FGameplayTag& InParentContextTag,
		class UPDBuildWidgetBase* InDirectParentReference)
	{
		BuildableTitle = InBuildableTitle;
		BuildableTag = InBuildableTag;
		ParentMenuContextTag = InParentContextTag;
		bCanBuild = bInCanBuild;
		DirectParentReference = InDirectParentReference;
	}

	/** @brief Immutable access to private member BuildableTitle */
	UFUNCTION(BlueprintCallable)
	const FText& GetBuildableTitle() const { return BuildableTitle; };
	
	/** @return private bCanBuild value */
	UFUNCTION(BlueprintCallable)
	bool GetCanBuild() const { return bCanBuild; };

	/** @brief Immutable access to private member BuildableTag */
	UFUNCTION(BlueprintCallable)
	const FGameplayTag& GetBuildableTag() const { return BuildableTag; };	

	/** @brief Direct access to private member DirectParentReference */
	UFUNCTION(BlueprintCallable)
	class UPDBuildWidgetBase* GetDirectParentReference() const { return DirectParentReference; };

	/** @brief Immutable access to private member ParentMenuContextTag */
	UFUNCTION(BlueprintCallable)
	const FGameplayTag& GetParentContextTag() const { return ParentMenuContextTag; };
	
private:
	/** @brief Assigned by 'AssignData', retrieved by 'GetBuildableTitle'. Is passed into a tileview or listview entry upon creation  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	FText BuildableTitle{};

	/** @brief Assigned by 'AssignData', retrieved by 'GetCanBuild''. Is passed into a tileview or listview entry upon creation   */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	bool bCanBuild = true;

	/** @brief Assigned by 'AssignData', retrieved by 'GetBuildableTag''. Is passed into a tileview or listview entry upon creation   */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	FGameplayTag BuildableTag;
	
	/** @brief Assigned by 'AssignData', retrieved by 'GetDirectParentReference''. Is passed into a tileview or listview entry upon creation   */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	class UPDBuildWidgetBase* DirectParentReference = nullptr;

	/** @brief Assigned by 'AssignData', retrieved by 'GetParentContextTag''. Is passed into a tileview or listview entry upon creation   */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	FGameplayTag ParentMenuContextTag{};
};

/** @brief Entry widget for a buildable. Used by the build menu class  */
UCLASS()
class PDRTSBASE_API UPDBuildableEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
public:

	/** @brief  Reads data from a given uobject and sets up delegates, will exit early if it is not derived of type 'UPDBuildableWrapper' */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	/** @brief Tells DirectParentReference about the clicked button, Logs to error if it fails */
	UFUNCTION() void OnPressed();
	/** @brief  Reserved */
	UFUNCTION() void OnHovered();
	/** @brief  Reserved */
	UFUNCTION() void OnUnhovered();
	/** @brief  Reserved */
	UFUNCTION() void OnClicked();
	/** @brief  Reserved */
	UFUNCTION() void OnReleased();

	/** @brief Border for the 'TextContent' text-block widget.*/
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UBorder* TextContentBorder = nullptr;	
	
	/** @brief Title to display for the buildable */
	UPROPERTY(Meta = (BindWidget))
	UTextBlock* BuildableTitle = nullptr;

	/** @brief Tells us if the button is clickable */
	UPROPERTY(BlueprintReadWrite)
	bool bCanBuild = true;

	/** @brief Tells us if the button is clickable */
	UPROPERTY(BlueprintReadWrite)
	bool bIsSelected = false;
	
	/** @brief The tag related to the buildable we are representing on the buildable element widget */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FGameplayTag BuildableTag;

	/** @brief Hitbox, what our mouse events actually interacts with */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UButton* Hitbox = nullptr;
	
	/** @brief Assigned by 'AssignData', retrieved by 'DirectParentReference'  */
	UPROPERTY(BlueprintReadWrite, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	class UPDBuildWidgetBase* DirectParentReference = nullptr;
	
	/** @brief Parent menu (Build context owning the entry) */
	FGameplayTag ParentMenuContextTag{};
};

/** @brief Entry widget for a build-context. Used by the build menu class  */
UCLASS()
class PDRTSBASE_API UPDBuildContextEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
public:

	/** @brief  Reads data from a given uobject and sets up delegates, will exit early if it is not derived of type 'UPDBuildContextWrapper' */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	/** @brief Tells DirectParentReference about the clicked button, Logs to error if it fails */
	UFUNCTION() void OnPressed();
	/** @brief  Reserved */
	UFUNCTION() void OnHovered();
	/** @brief  Reserved */
	UFUNCTION() void OnUnhovered();
	/** @brief  Reserved */
	UFUNCTION() void OnClicked();
	/** @brief  Reserved */
	UFUNCTION() void OnReleased();

	/** @brief Border for the 'TextContent' text-block widget.*/
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UBorder* TextContentBorder = nullptr;	
	
	/** @brief  Textblock title for the build-context name */
	UPROPERTY(Meta = (BindWidget))
	UTextBlock* ContextTitle = nullptr;

	/** @brief Assigned by 'AssignData', retrieved by 'DirectParentReference'  */
	UPROPERTY(BlueprintReadWrite, Meta = (ExposeOnSpawn=true))
	class UPDBuildWidgetBase* DirectParentReference = nullptr;
	
	/** @brief Hitbox, what our mouse events actually interacts with */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UButton* Hitbox = nullptr;	
	
	/** @brief  Tag of context that this widget represents */
	FGameplayTag SelfContextTag{};

	/** @brief Flag to tell us our selection state */
	bool bIsSelected = false;
};

/** @brief  Wrapper struct that allows us to expose rowtype filters */
USTRUCT(Blueprintable)
struct FPDBuildContextParam
{
	GENERATED_BODY()

	/** @brief  Inner array of build-context handles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (RowType = "/Script/PDRTSBase.PDBuildContext"))
	TArray<FDataTableRowHandle> NewBuildableContext;
};

/** @brief Build-system main widget.
 * @note Declares two tileviews and applies the BuildableEntry and BuildContextEntry widget to them.
 * @note Handles event hooks and loading of context data */
UCLASS()
class PDRTSBASE_API UPDBuildWidgetBase : public UUserWidget
{
	GENERATED_BODY()
public:

	/** @brief  Loads the widget flair data */
	virtual void NativeConstruct() override;

	/** @brief  Overwrites 'CurrentBuildableContext' */
	UFUNCTION(BlueprintCallable)
	void OverwriteBuildContext(const FPDBuildContextParam& NewRowParams);

	/** @brief  Loads 'CurrentBuildableContexts' data into the widget */
	UFUNCTION(BlueprintCallable)
	virtual void LoadBuildContexts();
	
	/** @brief  Selects the given context and loads its data into the widget, given that it is part of the CurrentBuildableContexts array */
	UFUNCTION(BlueprintCallable)
	virtual void SelectBuildContext(const FGameplayTag& NewSelectedContext, const bool bWasDeselected);
	
	/** @return 'CurrentBuildableContext' */
	UFUNCTION(BlueprintCallable)
	virtual TArray<FDataTableRowHandle>& GetCurrentBuildContexts() { return CurrentBuildableContexts; }

	/** @return 'DefaultBuildableContext' */
	UFUNCTION(BlueprintCallable)
	virtual TArray<FDataTableRowHandle>& GetDefaultBuildContexts() { return DefaultBuildableContexts; }

	/** @return 'DefaultBuildableContext' */
	UFUNCTION(BlueprintCallable)
	virtual const FGameplayTag& GetLastSelectedContextTag() { return LastSelectedContextTag; }

	/** @brief  Update the selected context menu, flushing and loading new buildable data if it is a new context menu. If an already existing menu it will close it */
	UFUNCTION(BlueprintCallable)
	void UpdateSelectedContext(const FGameplayTag& RequestToSelectTag);
	/** @brief  Select a buildable from the buildable list and calls UpdateSelectedBuildable */
	void SelectBuildable(const FGameplayTag& NewSelectedBuildable);

	/** @brief Dispatched a call to interface IPDRTSBuilderInterface::Execute_NewAction(this) if function succeeds.
	 *  @note If an already selected buildable entry is selected again it gets de-selected */
	UFUNCTION(BlueprintCallable)
	void UpdateSelectedBuildable(const FGameplayTag& RequestToSelectTag, const bool bRequestedBuildableWasValid);

	/** @brief  Spawns the worker build menu and plays it's 'Open' animation*/
	UFUNCTION(BlueprintCallable)
	void SpawnWorkerBuildMenu(const FPDBuildWorker& BuildWorker);
	/** @brief  Closes worker build menu and plays it's 'Close' animation*/
	UFUNCTION(BlueprintCallable)
	void BeginCloseWorkerBuildMenu();
	/** @brief  Finalize closing worker build menu when the 'Close' animation has finished*/
	UFUNCTION(BlueprintCallable)
	void EndCloseWorkerBuildMenu();

	/** @brief  Opens the worker build context submenu and plays it's 'Open' animation*/
	UFUNCTION(BlueprintCallable)
	void OpenWorkerContextMenu();	
	/** @brief  Closes worker build context submenu and plays it's 'Close' animation*/
	UFUNCTION(BlueprintCallable)
	void BeginCloseWorkerContext();
	/** @brief  Finalize closing worker build context submenu when the 'Close' animation has finished*/
	UFUNCTION(BlueprintCallable)
	void EndCloseWorkerContext();		

	/** @brief Tileview which will display our 'UPDBuildableEntry's */
	UPROPERTY(BlueprintReadWrite, Meta = (BindWidget))
	class UTileView* Buildables = nullptr;

	/** @brief Tileview which will display our contexts themselves */
	UPROPERTY(BlueprintReadWrite, Meta = (BindWidget))
	class UTileView* BuildContexts = nullptr;

	/** @brief Widget animation to play when opening the widget  */
	UPROPERTY(Transient, BlueprintReadWrite, Meta = (BindWidgetAnim))
	class UWidgetAnimation* OpenWidget = nullptr;	

	/** @brief Widget animation to play when closing the widget  */
	UPROPERTY(Transient, BlueprintReadWrite, Meta = (BindWidgetAnim))
	class UWidgetAnimation* CloseWidget = nullptr;

	/** @brief Widget animation to play when opening a widget context menus  */
	UPROPERTY(Transient, BlueprintReadWrite, Meta = (BindWidgetAnim))
	class UWidgetAnimation* OpenContextMenu = nullptr;	

	/** @brief Widget animation to play when closing the current widget context menu  */
	UPROPERTY(Transient, BlueprintReadWrite, Meta = (BindWidgetAnim))
	class UWidgetAnimation* CloseContextMenu = nullptr;			
	
	/** @brief DefaultBuildContext  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (RowType = "/Script/PDRTSBase.PDBuildContext"))
	TArray<FDataTableRowHandle> DefaultBuildableContexts;

	/** @brief DefaultBuildContext  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (RowType = "/Script/PDRTSBase.PDSharedBuildWidgetFlair"))
	FDataTableRowHandle BuildWidgetFlairHandle;
	
	/** @brief DefaultBuildContext  */
	struct FPDSharedBuildWidgetFlair* SelectedWidgetFlair = nullptr;

	/** @brief DefaultBuildContext  */
	bool bIsMenuVisible = false;

private:
	/** @brief Currently selected build context  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess="true", RowType = "/Script/PDRTSBase.PDBuildContext"))
	TArray<FDataTableRowHandle> CurrentBuildableContexts;

	/** @brief  Previously selected context-tag*/
	FGameplayTag LastSelectedContextTag{};
	/** @brief  Previously selected buildable-tag*/
	FGameplayTag LastSelectedBuildableTag{};
};



//
// Buildable actions and buildable action contexts -- Actions granted to a buildable

/** @brief Tile view object type for loading buildable data */
UCLASS(Blueprintable)
class UPDBuildActionContextWrapper : public UObject
{
	GENERATED_BODY()
public:
	
	/** @brief Self-explanatory */
	void AssignData(
		const FText& InBuildableTitle,
		const FGameplayTag& InContextTag,
		const TArray<FDataTableRowHandle>& InInnerBuildableHandles,
		class UPDBuildingActionsWidgetBase* InDirectParentReference)
	{
		BuildActionContextTitle = InBuildableTitle;
		ActionContextTag = InContextTag;
		InnerBuildableHandles = InInnerBuildableHandles;
		DirectParentReference = InDirectParentReference;
	}

	/** @brief Immutable access to private member BuildContextTitle */
	UFUNCTION(BlueprintCallable)
	const FText& GetBuildContextTitle() const { return BuildActionContextTitle; };

	/** @brief Immutable access to private member InnerBuildableTags */
	UFUNCTION(BlueprintCallable)
	const FGameplayTag& GetContextTag() const { return ActionContextTag; };	
	
	/** @brief Immutable access to private member InnerBuildableTags */
	UFUNCTION(BlueprintCallable)
	const TArray<FDataTableRowHandle>& GetBuildableTags() const { return InnerBuildableHandles; };	

	/** @brief Mutable access to private member ptr DirectParentReference */
	UFUNCTION(BlueprintCallable)
	class UPDBuildingActionsWidgetBase* GetDirectParentReference() const { return DirectParentReference; };

private:
	/** @brief Assigned by 'AssignData', retrieved by 'GetBuildableTitle'  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	FText BuildActionContextTitle{};

	/** @brief Assigned by 'AssignData', retrieved by 'GetBuildableTag'  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	FGameplayTag ActionContextTag{};
	
	/** @brief Assigned by 'AssignData', retrieved by 'GetBuildableTag'  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	TArray<FDataTableRowHandle> InnerBuildableHandles;

	/** @brief Assigned by 'AssignData', retrieved by 'GetDirectParentReference'  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	class UPDBuildingActionsWidgetBase* DirectParentReference = nullptr;
};

/** @brief Tile view object type for loading buildable data */
UCLASS(Blueprintable)
class UPDBuildableActionWrapper : public UObject
{
	GENERATED_BODY()
public:
	
	/** @brief Used to assign data to the wrapper before it is passed into a list-view and/or tile-view */
	void AssignData(
		const FText& InActionTitle,
		const bool bInCanBuild,
		const FGameplayTag& InActionTag,
		const FGameplayTag& InParentContextTag,
		class UPDBuildingActionsWidgetBase* InDirectParentReference)
	{
		ActionTitle = InActionTitle;
		ActionTag = InActionTag;
		ParentMenuContextTag = InParentContextTag;
		bCanBuild = bInCanBuild;
		DirectParentReference = InDirectParentReference;
	}

	/** @brief Immutable access to private member BuildableTitle */
	UFUNCTION(BlueprintCallable)
	const FText& GetBuildableTitle() const { return ActionTitle; };
	
	/** @return private bCanBuild value */
	UFUNCTION(BlueprintCallable)
	bool GetCanBuild() const { return bCanBuild; };

	/** @brief Immutable access to private member BuildableTag */
	UFUNCTION(BlueprintCallable)
	const FGameplayTag& GetActionTag() const { return ActionTag; };	

	/** @brief Direct access to private member DirectParentReference */
	UFUNCTION(BlueprintCallable)
	class UPDBuildingActionsWidgetBase* GetDirectParentReference() const { return DirectParentReference; };

	/** @brief Immutable access to private member ParentMenuContextTag */
	UFUNCTION(BlueprintCallable)
	const FGameplayTag& GetParentContextTag() const { return ParentMenuContextTag; };
	
private:
	/** @brief Assigned by 'AssignData', retrieved by 'GetBuildableTitle'. Is passed into a tileview or listview entry upon creation  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	FText ActionTitle{};

	/** @brief Assigned by 'AssignData', retrieved by 'GetCanBuild''. Is passed into a tileview or listview entry upon creation   */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	bool bCanBuild = true;

	/** @brief Assigned by 'AssignData', retrieved by 'GetBuildableTag''. Is passed into a tileview or listview entry upon creation   */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	FGameplayTag ActionTag;
	
	/** @brief Assigned by 'AssignData', retrieved by 'GetDirectParentReference''. Is passed into a tileview or listview entry upon creation   */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	class UPDBuildingActionsWidgetBase* DirectParentReference = nullptr;

	/** @brief Assigned by 'AssignData', retrieved by 'GetParentContextTag''. Is passed into a tileview or listview entry upon creation   */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	FGameplayTag ParentMenuContextTag{};
};

/** @brief Entry widget for a buildable. Used by the build menu class  */
UCLASS()
class PDRTSBASE_API UPDBuildableActionEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
public:

	/** @brief  Reads data from a given uobject and sets up delegates, will exit early if it is not derived of type 'UPDBuildableWrapper' */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	/** @brief Tells DirectParentReference about the clicked button, Logs to error if it fails */
	UFUNCTION() void OnPressed();
	/** @brief  Reserved */
	UFUNCTION() void OnHovered();
	/** @brief  Reserved */
	UFUNCTION() void OnUnhovered();
	/** @brief  Reserved */
	UFUNCTION() void OnClicked();
	/** @brief  Reserved */
	UFUNCTION() void OnReleased();

	/** @brief Border for the 'TextContent' text-block widget.*/
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UBorder* TextContentBorder = nullptr;	
	
	/** @brief Title to display for the buildable */
	UPROPERTY(Meta = (BindWidget))
	UTextBlock* ActionTitle = nullptr;

	/** @brief Tells us if the button is clickable */
	UPROPERTY(BlueprintReadWrite)
	bool bCanBuild = true;

	/** @brief Tells us if the button is clickable */
	UPROPERTY(BlueprintReadWrite)
	bool bIsSelected = false;
	
	/** @brief The tag related to the buildable we are representing on the buildable element widget */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FGameplayTag ActionTag;

	/** @brief Hitbox, what our mouse events actually interacts with */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UButton* Hitbox = nullptr;
	
	/** @brief Assigned by 'AssignData', retrieved by 'DirectParentReference'  */
	UPROPERTY(BlueprintReadWrite, Meta = (ExposeOnSpawn=true, AllowPrivateAccess="true"))
	class UPDBuildingActionsWidgetBase* DirectParentReference = nullptr;
	
	/** @brief Parent menu (Build context owning the entry) */
	FGameplayTag ParentMenuContextTag{};
};

/** @brief Entry widget for a build-context. Used by the build menu class  */
UCLASS()
class PDRTSBASE_API UPDBuildActionContextEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
public:

	/** @brief  Reads data from a given uobject and sets up delegates, will exit early if it is not derived of type 'UPDBuildContextWrapper' */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	/** @brief Tells DirectParentReference about the clicked button, Logs to error if it fails */
	UFUNCTION() void OnPressed();
	/** @brief  Reserved */
	UFUNCTION() void OnHovered();
	/** @brief  Reserved */
	UFUNCTION() void OnUnhovered();
	/** @brief  Reserved */
	UFUNCTION() void OnClicked();
	/** @brief  Reserved */
	UFUNCTION() void OnReleased();

	/** @brief Border for the 'TextContent' text-block widget.*/
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UBorder* TextContentBorder = nullptr;	
	
	/** @brief  Textblock title for the build-context name */
	UPROPERTY(Meta = (BindWidget))
	UTextBlock* ContextTitle = nullptr;

	/** @brief Assigned by 'AssignData', retrieved by 'DirectParentReference'  */
	UPROPERTY(BlueprintReadWrite, Meta = (ExposeOnSpawn=true))
	class UPDBuildingActionsWidgetBase* DirectParentReference = nullptr;
	
	/** @brief Hitbox, what our mouse events actually interacts with */
	UPROPERTY(BlueprintReadWrite, Meta=(BindWidget))
	class UButton* Hitbox = nullptr;	
	
	/** @brief  Tag of context that this widget represents */
	FGameplayTag SelfContextTag{};

	/** @brief Flag to tell us our selection state */
	bool bIsSelected = false;
};

/** @brief  Wrapper struct that allows us to expose rowtype filters */
USTRUCT(Blueprintable)
struct FPDBuildActionContextParam
{
	GENERATED_BODY()

	/** @brief  Inner array of buildable action-context handles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (RowType = "/Script/PDRTSBase.PDBuildActionContext"))
	TArray<FDataTableRowHandle> NewBuildActionContexts;
};

/** @brief Build-system buildable actions widget. The widget we want to display when having a placed buildable selected
 * @note Declares two tile-views and applies the BuildableActionEntry and ActionContextEntry widget to them.
 * @note Handles event hooks and loading of context data */
UCLASS()
class PDRTSBASE_API UPDBuildingActionsWidgetBase : public UUserWidget
{
	GENERATED_BODY()
public:

	/** @brief  Loads the widget flair data */
	virtual void NativeConstruct() override;

	/** @brief  Overwrites 'CurrentBuildableContext' */
	UFUNCTION(BlueprintCallable)
	void OverwriteActionContext(const FPDBuildActionContextParam& NewRowParams);

	/** @brief  Loads 'CurrentBuildableContexts' data into the widget */
	UFUNCTION(BlueprintCallable)
	virtual void LoadActionContexts();
	
	/** @brief  Selects the given context and loads its data into the widget, given that it is part of the CurrentBuildableContexts array */
	UFUNCTION(BlueprintCallable)
	virtual void SelectActionContext(const FGameplayTag& NewSelectedContext, const bool bWasDeselected);
	
	/** @return 'CurrentBuildableContext' */
	UFUNCTION(BlueprintCallable)
	virtual TArray<FDataTableRowHandle>& GetCurrentActionContexts() { return CurrentActionContextHandles; }

	/** @return 'DefaultBuildableContext' */
	UFUNCTION(BlueprintCallable)
	virtual TArray<FDataTableRowHandle>& GetDefaultActionContexts() { return DefaultActionContexts; }

	/** @return 'DefaultBuildableContext' */
	UFUNCTION(BlueprintCallable)
	virtual const FGameplayTag& GetLastSelectedContextTag() { return LastSelectedActionContextTag; }

	/** @brief  Update the selected context menu, flushing and loading new buildable data if it is a new context menu. If an already existing menu it will close it */
	UFUNCTION(BlueprintCallable)
	void UpdateSelectedActionContext(const FGameplayTag& RequestToSelectTag);
	/** @brief  Select a buildable from the buildable list and calls UpdateSelectedBuildable */
	void SelectAction(const FGameplayTag& NewSelectedAction);

	/** @brief Dispatched a call to interface IPDRTSBuilderInterface::Execute_NewAction(this) if function succeeds.
	 *  @note If an already selected buildable entry is selected again it gets de-selected */
	UFUNCTION(BlueprintCallable)
	void UpdateSelectedAction(const FGameplayTag& RequestToSelectTag, const bool bRequestedBuildableWasValid);

	/** @brief  Spawns the worker build menu and plays it's 'Open' animation*/
	UFUNCTION(BlueprintCallable)
	void SpawnBuildableActionMenu(const FPDBuildable& BuildableToSourceActionsFrom);
	/** @brief  Closes worker build menu and plays it's 'Close' animation*/
	UFUNCTION(BlueprintCallable)
	void BeginCloseBuildableActionMenu();
	/** @brief  Finalize closing worker build menu when the 'Close' animation has finished*/
	UFUNCTION(BlueprintCallable)
	void EndCloseBuildableActionMenu();

	/** @brief  Opens the worker build context submenu and plays it's 'Open' animation*/
	UFUNCTION(BlueprintCallable)
	void OpenActionContextMenu();	
	/** @brief  Closes worker build context submenu and plays it's 'Close' animation*/
	UFUNCTION(BlueprintCallable)
	void BeginCloseActionContext();
	/** @brief  Finalize closing worker build context submenu when the 'Close' animation has finished*/
	UFUNCTION(BlueprintCallable)
	void EndCloseActionContext();
	
	UFUNCTION(BlueprintCallable)
	void SetNewWorldActor(AActor* NewWorldActor, const FPDBuildable& BuildableToSourceActionsFrom);

	UFUNCTION(BlueprintCallable)
	void DestroyCurrentWorldActor() const;

	/** @brief UPDRangedSelector which will be used to select amounts for actions that allow it
	 * @todo Actually need to make use of this */
	UPROPERTY(BlueprintReadWrite, Meta = (BindWidget, ExposeOnSpawn))
	UPDRangedSelector* CountSelector = nullptr;
	
	/** @brief Tileview which will display our 'UPDBuildableEntry's */
	UPROPERTY(BlueprintReadWrite, Meta = (BindWidget))
	class UTileView* Actions = nullptr;

	/** @brief Tileview which will display our contexts themselves */
	UPROPERTY(BlueprintReadWrite, Meta = (BindWidget))
	class UTileView* ActionContexts = nullptr;

	/** @brief Widget animation to play when opening the widget  */
	UPROPERTY(Transient, BlueprintReadWrite, Meta = (BindWidgetAnim))
	class UWidgetAnimation* OpenWidget = nullptr;	

	/** @brief Widget animation to play when closing the widget  */
	UPROPERTY(Transient, BlueprintReadWrite, Meta = (BindWidgetAnim))
	class UWidgetAnimation* CloseWidget = nullptr;

	/** @brief Widget animation to play when opening a widget context menus  */
	UPROPERTY(Transient, BlueprintReadWrite, Meta = (BindWidgetAnim))
	class UWidgetAnimation* OpenContextMenu = nullptr;	

	/** @brief Widget animation to play when closing the current widget context menu  */
	UPROPERTY(Transient, BlueprintReadWrite, Meta = (BindWidgetAnim))
	class UWidgetAnimation* CloseContextMenu = nullptr;			
	
	/** @brief DefaultBuildContext  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (RowType = "/Script/PDRTSBase.PDBuildActionContext"))
	TArray<FDataTableRowHandle> DefaultActionContexts;

	/** @brief DefaultBuildContext  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (RowType = "/Script/PDRTSBase.PDSharedBuildWidgetFlair"))
	FDataTableRowHandle BuildWidgetFlairHandle;
	
	/** @brief DefaultBuildContext  */
	struct FPDSharedBuildWidgetFlair* SelectedWidgetFlair = nullptr;

	/** @brief DefaultBuildContext  */
	bool bIsMenuVisible = false;

	/** @brief World actor this which we draw the context from  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Meta = (RowType = "/Script/PDRTSBase.PDSharedBuildWidgetFlair"))
	AActor* CurrentWorldActor = nullptr;
	
	/** @brief The widget class for any dialogs we might need */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UPDGenericDialog> ConfirmDialogClass = nullptr;		
private:
	/** @brief Currently selected build context  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess="true"))
	TArray<FDataTableRowHandle> CurrentActionContextHandles;

	/** @brief  Previously selected context-tag*/
	FGameplayTag LastSelectedActionContextTag{};
	/** @brief  Previously selected buildable-tag*/
	FGameplayTag LastSelectedActionTag{};
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
