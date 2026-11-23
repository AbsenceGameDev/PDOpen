/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "RTSOpenCommon.h"
#include "Widgets/SCompoundWidget.h"
#include "GameFramework/GameUserSettings.h"
#include "SRTSOSettingsStringSelector.generated.h"

DECLARE_DELEGATE_TwoParams(FOnStringValueSelected, FString /*SelectedString*/, UWidget* /*Caller*/)

/** @brief Selectable dropdown list of string options with callbacks bound so user can use as onlclick events. Wraps the list in an expandable area */
class RTSOPEN_API SRTSOStringSelector : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRTSOStringSelector) 
      : _OptionsArray()
      , _OnStringValueSelected(FOnStringValueSelected{}) 
      , _Caller(nullptr) 
      {}
		SLATE_ARGUMENT(TArray<TSharedPtr<FString>>*, OptionsArray)
		SLATE_ARGUMENT(FOnStringValueSelected, OnStringValueSelected)
		SLATE_ARGUMENT(UWidget*, Caller)
	SLATE_END_ARGS()

	/** @brief Assings some values fomr InArgs and calls UpdateOption */
	void Construct(const FArguments& InArgs);
	/** @brief Simple wrapper function to open the expandable area via animation*/
	void UpdateExpandableArea(bool bOpen);

	/** @brief Generates a listview over the input options */
	void UpdateOptions(TArray<TSharedPtr<FString>>* InOptionsArray);

	/** @brief Generates the listentry widget */
	TSharedRef<ITableRow> MakeListViewWidget_SettingOptionsSelector(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable) const;
	/** @brief Calls a user bound delegate when this is called, this is called when an entry in the list has been selected */
	void OnOptionSelected_SettingOptionsSelector(TSharedPtr<FString> InItem, ESelectInfo::Type InSelectInfo);

	/** @brief Copy of user delegate */
   FOnStringValueSelected OnStringValueSelected;

	/** @brief Local copy of options array */
	TArray<TSharedPtr<FString>>* OptionsArray;

	/** @brief Cached pointer to the caller stringselector */
   class UWidget* Caller = nullptr;

	/** @brief Wraps our list, for a more user friendly appearance in the options menu this is mostly used in */
   TSharedPtr<class SExpandableArea> ExpandableArea;  
};


UCLASS(Config = "Game", DefaultConfig)
class RTSOPEN_API URTSStringSelectorDeveloperSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   UPROPERTY(Config, EditAnywhere, Category = "String Selector")
   FSlateFontInfo DefaultFont;

};

// UCLASS(Config = "Game", DefaultConfig)
// class RTSOPEN_API URTSActionLogDefaultDeveloperSettings : public UDeveloperSettings
// {
// public:
// 	GENERATED_BODY()

// 	/** @brief Build Contexts (categories) table soft objects */
// 	UPROPERTY(Config, EditAnywhere, Category = "Action Log Subsystem", Meta = (RequiredAssetDataTags="RowStructure=/Script/RTSOpen.RTSActionLogStyleCompound"))
// 	TArray<TSoftObjectPtr<UDataTable>> ActionLogStyleTables;

// 	/** @brief Build Contexts (categories) table soft objects */
// 	UPROPERTY(Config, EditAnywhere, Category = "Action Log Subsystem")
// 	bool bShowActionLogTimestamps = true;	
// };



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
