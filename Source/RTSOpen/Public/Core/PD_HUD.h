/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include "CoreMinimal.h"
#include "PDRTSBaseSubsystem.h"
#include "GameFramework/HUD.h"
#include "Engine/DeveloperSettings.h"
#include "PD_HUD.generated.h"

/**
 * @brief Handles drawing things on the HUD mainly, is ued to draw the marquee rectangle on screen
 */
UCLASS()
class RTSOPEN_API APD_HUD : public AHUD
{
	GENERATED_BODY()

	/** @brief Reads the start and current marquee positions from the player controller,
	 * and display draw a rectangle on screen to convey the marquee selection actually happening to the user */
	virtual void DrawHUD() override;
	
	/** @brief Draws the selection marquee directly on the hud, gets called in APD_HUD::DrawHUD. */
	/** @brief Deprecated, moved into slate widget */
	void DrawRadarMinimap();

	/** @brief Draws the selection marquee directly on the hud, gets called in APD_HUD::DrawHUD. Reads selection/marquee data from the player controller*/
	void DrawSelectionMarquee();


private:
	APD_HUD();

	virtual void BeginPlay() override;

	/** @brief Draw all nearby actors on the minimap */
	/** @brief Deprecated, moved into slate widget */
	void DrawActorsOnMiniMap();

	/** @brief Draw nearby entities on the minimap */
	/** @brief Deprecated, moved into slate widget */
	void DrawEntitiesOnMiniMap();	
	
	/** @brief Draw self/local player on minimap */
	/** @brief Deprecated, moved into slate widget */
	void DrawOwnerOnMiniMap();
	
	/** @brief Draws the actual radar, defines the visuals of it */
	/** @brief Deprecated, moved into slate widget */
	void DrawRadar(FLinearColor Color = FLinearColor::Yellow);

	/** @brief Returns center screen location where the radar will be displayed */
	FVector2D GetRadarCenter() const;

	/** @brief Owning/local actors current world location */
	FVector GetCurrentActorLocation() const;

	/** @brief Player world to screen transformation */
	FVector2D WorldToScreen2D(FLEntityCompound& EntityCompound) const;
	/** @brief Player world to screen transformation */
	FVector2D WorldToScreen2D(const AActor* ActorToPlace) const;
	/** @brief Player world to screen transformation */
	FVector2D WorldToScreen2D(FVector& WorldLocation) const;
public:
	/** @brief On screen location where the unscaled radar will be placed */
	/** @brief Deprecated, moved into slate widget */
	UPROPERTY(EditAnywhere)
	FVector2D RadarStartLocation = FVector2D(10.f, 10.f);

	/** @brief On screen location where the unscaled radar will be placed */
	/** @brief Deprecated, moved into slate widget */
	UPROPERTY(EditAnywhere)
	double RadarSize = 200.0;

	/** @brief Deprecated, moved into slate widget */
	UPROPERTY(EditAnywhere, Category = MiniMap2D)
	double RadarDistanceScale = 25.0;

	/** @brief Deprecated, moved into slate widget */
	UPROPERTY(EditAnywhere, Category = MiniMap2D)
	double SphereHeight = 300.0;

	/** @brief Deprecated, moved into slate widget */
	UPROPERTY(EditAnywhere, Category = MiniMap2D)
	double RadarTraceSphereRadius = 4000.0;

	/** @brief Deprecated, moved into slate widget */
	UPROPERTY(EditAnywhere, Category = MiniMap2D)
	double GenericMinimapIconRectSize = 3.0;

	/** @brief Deprecated, moved into slate widget */
	UPROPERTY(EditAnywhere, Category = MiniMap2D, DisplayName = "Mission Color")
	FColor MissionColour {1,1,0};
	
	/** @brief Deprecated, moved into slate widget */
	UPROPERTY(EditAnywhere, Category = MiniMap2D, DisplayName = "Interactable Color")
	FColor InteractableColour {0,1,1};
	
	/** @brief Deprecated, moved into slate widget */
	UPROPERTY(EditAnywhere, Category = MiniMap2D, DisplayName = "Enemy Color")
	FColor EnemyColour {1,0,0};

	/** @brief Deprecated, moved into slate widget */
	UPROPERTY(EditAnywhere, Category = MiniMap2D, DisplayName = "Owned Units Color")
	FColor OwnedUnitsColour {0,1,0};		
	
	/** @brief Deprecated, moved into slate widget */
	UPROPERTY(EditAnywhere, Category = MiniMap2D, DisplayName = "Friend Color")
	FColor FriendColour {0,0,1};

private:
	/** @brief Deprecated, moved into slate widget */
	UPROPERTY()
	URTSOMinimapData_Deprecated* MiniMapData = nullptr;
	/** @brief Deprecated, moved into slate widget */
	const float FixedTextureScale = 0.02f;
	
	/** @brief Deprecated, moved into slate widget */
	UPROPERTY()
	TArray<AActor*> OnWorldActors;
	/** @brief Deprecated, moved into slate widget */
	TArray<FHitResult*> Results;
	/** @brief Deprecated, moved into slate widget */
	const float Alpha = 1.f;
};


// Move the below classes into the shared UI under 'Classes' when moving his into it's own slate widget 
/** @brief DEPRECATED */
UCLASS()
class RTSOPEN_API URTSOMinimapData_Deprecated : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Minimap")
	UTexture2D* ArrowTexture = nullptr;
};

/** @brief DEPRECATED */
UCLASS(Config = "Game", DefaultConfig)
class RTSOPEN_API URTSOMinimapDeveloperSettings_Deprecated : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	URTSOMinimapDeveloperSettings_Deprecated(){}
	
	/** @brief Default Minimap data */
	UPROPERTY(Config, EditAnywhere, Category = "Minimap")
	TSoftObjectPtr<URTSOMinimapData_Deprecated> DefaultMinimapData;
	
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