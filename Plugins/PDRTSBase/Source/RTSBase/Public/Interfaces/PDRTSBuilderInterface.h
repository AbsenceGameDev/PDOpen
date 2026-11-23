/* @author: Ario Amin @ Permafrost Development. @copyright: Full Apache License included at bottom of the file  */
/** @todo rethink if needed/wanted or if this plugin should only use entities with logic fragments for handling actions */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PDBuildCommon.h"
#include "UObject/Interface.h"
#include "PDRTSBuilderInterface.generated.h"

UENUM()
enum class ERTSBuildMenuModules : uint8
{
	SelectBuildable,
	SelectContext,
	DeselectBuildable,
	DeselectContext, 
};

UENUM()
enum class ERTSBuildableActionMenuModules : uint8
{
	SelectBuildableActionContext,    // Context of actions granted to a buildable  
	DeselectBuildableActionContext,  // Context of actions granted to a buildable  
	FireBuildableAction,             // Actions granted to a buildable 
	DoNothing,                       // Do nothing 
};



/** @brief Boilerplate */
UINTERFACE(MinimalAPI) class UPDRTSBuilderInterface : public UInterface { GENERATED_BODY() };

/**
 * @brief This interface will be placed on pawns or characters we want to perform an action from.
 * @note It is abstracted in the plugin to allow anything else to hook into it in the game layer
 *
 */
class PDRTSBASE_API IPDRTSBuilderInterface
{
	GENERATED_BODY()

public:
	/** @brief Used by the build system for when a enw context or buildable is selected (or when an old one is deselected) */
	UFUNCTION(BlueprintNativeEvent, CallInEditor, Category = "Action|Interface")
	void SelectBuildMenuEntry(ERTSBuildMenuModules ActionMode, FGameplayTag ActionTag);
	virtual void SelectBuildMenuEntry_Implementation(ERTSBuildMenuModules ActionMode, FGameplayTag ActionTag)
	{
		// Perform actions
		switch (ActionMode)
		{
		case ERTSBuildMenuModules::SelectBuildable:
			break;
		case ERTSBuildMenuModules::SelectContext:
			break;
		case ERTSBuildMenuModules::DeselectBuildable:
			break;
		case ERTSBuildMenuModules::DeselectContext:
			break;
		}
		
		return;
	}


	/** @brief Used by the build system for when a enw context or buildable is selected (or when an old one is deselected) */
	UFUNCTION(BlueprintNativeEvent, CallInEditor, Category = "Action|Interface")
	void SelectActionMenuEntry(ERTSBuildableActionMenuModules ActionMode, FGameplayTag ActionTag, const TArray<uint8>& Payload);
	virtual void SelectActionMenuEntry_Implementation(ERTSBuildableActionMenuModules ActionMode, FGameplayTag ActionTag, const TArray<uint8>& Payload)
	{
		// Perform actions
		switch (ActionMode)
		{
		case ERTSBuildableActionMenuModules::SelectBuildableActionContext:
			break;
		case ERTSBuildableActionMenuModules::DeselectBuildableActionContext:
			break;			
		case ERTSBuildableActionMenuModules::FireBuildableAction:
			break;
		case ERTSBuildableActionMenuModules::DoNothing:
			break;
		}
		
		return;
	}	

	/** @brief @todo this is a temporary function signature,
	 *   @todo cont: Must rewrite other code to manage actor IDs properly
	 *   @todo cont: then replace this to return TArray<in32> instead of TArray<AActor*>,
	 *   @todo cont: ActorIDs will be safe to use in save files, the Actor pointers will not be viable for save data
	 * @note it is up to the subclass implementation to ensure this actually does anything */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Interface")
	void GetOwnedBuildings(TArray<AActor*>& OutArray);
	virtual void GetOwnedBuildings_Implementation(TArray<AActor*>& OutArray)
	{
		static TArray<AActor*> Dummy{};
		OutArray = Dummy;
	};

	/** @brief Pass an actor which we want to set as a new building,
	 * @note it is up to the subclass implementation to ensure this actually does anything */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action|Interface")
	void SetOwnedBuilding(AActor* NewBuilding);
	virtual void SetOwnedBuilding_Implementation(AActor* NewBuilding) { }

	/** @brief Returns the default actor type (via tag), we wany for our default base types
	 * Interface impl. returns TAG_BUILD_ActionContext_Base0, but this can be overriden if setting up races/factions that uses a different building for their default base */	
	UFUNCTION(BlueprintNativeEvent, CallInEditor, Category = "Action|Interface")
	FGameplayTag GetDefaultBaseType();
	virtual FGameplayTag GetDefaultBaseType_Implementation()
	{
		return TAG_BUILD_ActionContext_Base0;
	}
	
	/** @brief If if logic has already been implemented to generate persistent IDs for players, then implement this and pass it as return */
	UFUNCTION(BlueprintNativeEvent, CallInEditor, Category = "Action|Interface")
	int32 GetBuilderID();
	virtual int32 GetBuilderID_Implementation() { return INDEX_NONE; }
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