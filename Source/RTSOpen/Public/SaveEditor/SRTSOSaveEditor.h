/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RTSOpenCommon.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Interfaces/PDInteractInterface.h"
#include "Subsystems/EngineSubsystem.h"
#include "SRTSOSaveEditor.generated.h"

DECLARE_LOG_CATEGORY_CLASS(PDLog_SaveEditor, Log, All);

/** @brief Player location struct, is planned to be expanded to keep other generic per-player data */
USTRUCT()
struct FPlayerLocationStruct
{
	GENERATED_BODY()

	/** @brief UserID */
	int32 Key;
	/** @brief User Location */
	FVector Value;
};
/** @brief Player inventory wrapper */
USTRUCT()
struct FUserInventoriesStruct
{
	GENERATED_BODY()
	/** @brief UserID/Owning player */
	int32 Key;
	/** @brief User Inventory Wrapper */
	FRTSSavedItems Value;
};
/** @brief Conversation Actor State wrapper */
USTRUCT()
struct FConversationStateStruct
{
	GENERATED_BODY()

	/** @brief ActorID */
	int32 Key;
	/** @brief State date for conv actor */
	FRTSSavedConversationActorData Value;
	
	/** @brief Copy selected class (_HiddenInstantiatedClass) to softobject if not already copied, clear _HiddenInstantiatedClass no matter if it was copied or not */
	void CopySelectedToSoftClass(UClass* PotentialOverrideHiddenClass = nullptr)
	{
		if (PotentialOverrideHiddenClass != nullptr || Value._HiddenInstantiatedClass == nullptr)
		{
			Value._HiddenInstantiatedClass = PotentialOverrideHiddenClass;
		}
		
		if (Value._HiddenInstantiatedClass != nullptr)
		{
			if (TSoftClassPtr<ARTSOInteractableConversationActor>(Value._HiddenInstantiatedClass) != Value.ActorClassType)
			{
				Value.ActorClassType = TSoftClassPtr<ARTSOInteractableConversationActor>(Value._HiddenInstantiatedClass);
			}
			Value._HiddenInstantiatedClass = nullptr;
		}		
	}
};

/** @brief Conversation State InnerStruct, progression level per player/user */
USTRUCT()
struct FConversationProgressionInnerStruct
{
	GENERATED_BODY()
	/** @brief UserID */
	int32 Key;
	/** @brief ProgressionLevel */
	FRTSOConversationMetaProgressionListWrapper Value;
};
/** @brief The accumulated (mission) tags a player/user has, (milestones/subobjectives) */
USTRUCT()
struct FUserMissionTagsStruct
{
	GENERATED_BODY()
	/** @brief PlayerID */
	int32 Key;
	/** @brief Accumulated Mission Tags */
	FGameplayTagContainer Value;
};
/** @brief Inventory item stack wrapper  */
USTRUCT()
struct FStacksStruct
{
	GENERATED_BODY()
	/** @brief StackIndex*/
	int32 Key;
	/** @brief ItemCount*/
	int32 Value;
};

/** @brief Class filter for the save editor:
 * @todo This might not be usable when moving away from the class picker.
 * Note: the class picker is dependent on editor-only code*/
class FRTSSaveEd_InteractableClassFilter : public IClassViewerFilter
{
public:
	/** The meta class for the property that classes must be a child-of. */
	const UClass* ClassPropertyMetaClass = nullptr;

	/** The interface that must be implemented. */
	const UClass* InterfaceThatMustBeImplemented = nullptr;

	/** Whether or not abstract classes are allowed. */
	bool bAllowAbstract = false;

	/** Classes that can be picked */
	TArray<const UClass*> AllowedClassFilters;

	/** Classes that can't be picked */
	TArray<const UClass*> DisallowedClassFilters;

	/** @brief Boilerplate condition helper */
	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs ) override
	{
		return IsClassAllowedHelper(InClass);
	}
	
	/** @brief Boilerplate condition helper */
	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InBlueprint, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return IsClassAllowedHelper(InBlueprint);
	}

private:

	/** @brief Boilerplate template condition helper */
	template <typename TClass>
	bool IsClassAllowedHelper(TClass InClass)
	{
		bool bMatchesFlags = !InClass->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated) &&
			(bAllowAbstract || !InClass->HasAnyClassFlags(CLASS_Abstract));

		if (bMatchesFlags && InClass->IsChildOf(ClassPropertyMetaClass)
			&& (!InterfaceThatMustBeImplemented || InClass->ImplementsInterface(InterfaceThatMustBeImplemented)))
		{
			auto PredicateFn = [InClass](const UClass* Class)
			{
				return InClass->IsChildOf(Class);
			};

			if (DisallowedClassFilters.FindByPredicate(PredicateFn) == nullptr &&
				(AllowedClassFilters.Num() == 0 || AllowedClassFilters.FindByPredicate(PredicateFn) != nullptr))
			{
				return true;
			}
		}

		return false;
	}
};


/**
 * @brief  Loads custom tags that may have been added by a player/user
*/
class RTSOPEN_API SRTSOSaveEditorBase : public SCompoundWidget
{
public:
	/** @brief Base call, ensures we have a title-font loaded  */
	virtual void UpdateChildSlot(void* OpaqueData)
	{
		if (TitleFont.TypefaceFontName.IsNone())
		{
			// @todo Set up a custom slate styleset for the saveeditors fonts and icons 
			TitleFont = FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont"));
			TitleFont.Size *= 8;
		}
	}

	/** @brief Generic call to create picker windows of differing types @todo finish impl. of related Picker classes  */
	template<typename TPickerClass>
	TSharedRef<SWindow> CreatePickerDialog(TSharedRef<SWindow>& PickerWindow, UClass* FilterInterfaceClass)
	{
		FClassViewerInitializationOptions InitOptions;
		InitOptions.Mode = EClassViewerMode::ClassPicker;
		InitOptions.DisplayMode = EClassViewerDisplayMode::TreeView;

		const TSharedRef<FRTSSaveEd_InteractableClassFilter> SaveEd_InteractableClassFilter = MakeShared<FRTSSaveEd_InteractableClassFilter>();
		SaveEd_InteractableClassFilter->InterfaceThatMustBeImplemented = FilterInterfaceClass;
		InitOptions.ClassFilters.Add(SaveEd_InteractableClassFilter);
	
		return SNew(TPickerClass)
				.ParentWindow(PickerWindow)
				.Options(InitOptions)
				.AssetType(nullptr);		
	}

	/** @brief Generic call to create picker windows of differing types @todo finish impl. of related Picker classes  */
	template<typename TPickerClass>
	TSharedRef<SWindow> CreatePickerWindow()
	{
		// Create the window to pick the class
		TSharedRef<SWindow> PickerWindow = SNew(SWindow)
			.Title(FText())
			.SizingRule( ESizingRule::Autosized )
			.ClientSize( FVector2D( 0.f, 300.f ))
			.SupportsMaximize(false)
			.SupportsMinimize(false);

		TSharedRef<TPickerClass> PickerDialog = CreatePickerDialog<TPickerClass>(PickerWindow, UPDInteractInterface::StaticClass());
		PickerWindow->SetContent(PickerDialog);

		const TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelRegularWindow();
		if( ParentWindow.IsValid() )
		{
			FSlateApplication::Get().AddModalWindow(PickerWindow, ParentWindow );
		}

		return PickerWindow;		
	}

	/** @brief Font we want to use for titles in teh save editor */
	FSlateFontInfo TitleFont;	
	/** @brief Linked copy of the selected savedata. Any changes will be on this copy until we want to commit them to the actual save-file */
	FRTSSaveData* LinkedSaveDataCopy = nullptr;	
};

#define VERTICAL_SEPARATOR(thickness) \
SVerticalBox::Slot() \
[ \
	SNew(SSeparator) \
	.Thickness(thickness) \
	.Orientation(Orient_Vertical) \
]

#define HORIZONTAL_SEPARATOR(thickness) \
SHorizontalBox::Slot() \
[ \
	SNew(SSeparator) \
	.Thickness(thickness) \
	.Orientation(Orient_Horizontal) \
]

#define INSET_AUTO_VERTICAL_SLOT(VerticalPadding) \
SVerticalBox::Slot() \
	.Padding(FMargin{0, VerticalPadding}) \
	.AutoHeight()

#define INSET_VERTICAL_SLOT(VerticalPadding) \
SVerticalBox::Slot() \
	.Padding(FMargin{0, VerticalPadding}) \
	.FillHeight(10) 

#define INSET_HORIZONTAL_SLOT(HorizontalPadding) \
SHorizontalBox::Slot() \
	.Padding(FMargin{HorizontalPadding, 0}) \
	.FillWidth(1)

#define INSET_AUTO_HORIZONTAL_SLOT(HorizontalPadding) \
SHorizontalBox::Slot() \
	.Padding(FMargin{HorizontalPadding, 0}) \
	.AutoWidth()

#define INSET_VERTICAL_SLOT_CL(VerticalPadding) \
SVerticalBox::Slot() \
	.Padding(FMargin{0, VerticalPadding}) \
	.FillHeight(1) \
	.HAlign(HAlign_Center) \
	.VAlign(VAlign_Center)

#define INSET_HORIZONTAL_SLOT_CL(HorizontalPadding) \
SHorizontalBox::Slot() \
	.Padding(FMargin{HorizontalPadding, 0}) \
	.FillWidth(1) \
	.HAlign(HAlign_Center) \
	.VAlign(VAlign_Center) 



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
