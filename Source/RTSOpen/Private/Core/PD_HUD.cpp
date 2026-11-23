/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Core/PD_HUD.h"

#include "MassEntitySubsystem.h"
#include "PDRTSBaseSubsystem.h"
#include "RTSOpenCommon.h"
#include "Actors/GodHandPawn.h"
#include "Actors/RTSOController.h"
#include "AI/Mass/RTSOMassFragments.h"
#include "Chaos/DebugDrawQueue.h"
#include "Kismet/KismetSystemLibrary.h"

APD_HUD::APD_HUD()
	: Super()
{
	const TSoftObjectPtr<URTSOMinimapData_Deprecated> SofMinimapDataPtr = GetDefault<URTSOMinimapDeveloperSettings_Deprecated>()->DefaultMinimapData;
	if (SofMinimapDataPtr.IsValid() == false)
	{
		UE_LOG(PDLog_RTSO, Warning, TEXT("APD_HUD::BeginPlay -- URTSOMinimapDeveloperSettings has no valid default value set for 'DefaultMinimapData'"))
		return;
	}
	MiniMapData = SofMinimapDataPtr.LoadSynchronous();
}

void APD_HUD::BeginPlay()
{
	Super::BeginPlay();

	// Already have a valid arrow texture, skip
	if (MiniMapData != nullptr) { return; }

	const TSoftObjectPtr<URTSOMinimapData_Deprecated> SofMinimapDataPtr = GetDefault<URTSOMinimapDeveloperSettings_Deprecated>()->DefaultMinimapData;
	if (SofMinimapDataPtr.IsValid() == false)
	{
		UE_LOG(PDLog_RTSO, Error, TEXT("APD_HUD::BeginPlay -- URTSOMinimapDeveloperSettings has no valid default value set for 'DefaultMinimapData'"))
		return;
	}
	
	MiniMapData = SofMinimapDataPtr.LoadSynchronous();
}

void APD_HUD::DrawRadarMinimap()
{
	// DrawRadar(FLinearColor::Gray);
	// DrawOwnerOnMiniMap();
	// DrawActorsOnMiniMap();
	// DrawEntitiesOnMiniMap();
}

void APD_HUD::DrawSelectionMarquee()
{
	const ARTSOController* RTSOController = Cast<ARTSOController>(GetOwningPlayerController());
	if (RTSOController == nullptr || RTSOController->IsDrawingMarquee() == false) { return; }
	
	const FVector2D DeltaMarquee(RTSOController->GetCurrentMousePositionMarquee() - RTSOController->GetStartMousePositionMarquee());
	const float Width = DeltaMarquee.X;
	const float Height = DeltaMarquee.Y;
	
	DrawRect(FLinearColor(.4, .4, .4, .42), RTSOController->GetStartMousePositionMarquee().X, RTSOController->GetStartMousePositionMarquee().Y, Width, Height);
}

void APD_HUD::DrawHUD()
{
	Super::DrawHUD();

	// @todo Moved to slate widget
	// //
	// // Draw Minimap
	// DrawRadarMinimap(); // todo, remove, won't make sense with the current layered RT texture appraoch
	
	//
	// Draw Marquee for entity selection
	DrawSelectionMarquee();
}

void APD_HUD::DrawActorsOnMiniMap()
{
	APawn* OwnerPawn = GetWorld()->GetFirstPlayerController()->GetPawn();
	if (OwnerPawn == nullptr) { return; }
	
	OnWorldActors.Empty();
	UKismetSystemLibrary::SphereOverlapActors(
		OwnerPawn,
		OwnerPawn->GetActorLocation(),
		RadarTraceSphereRadius,
		{UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic)},
		nullptr, 
		{OwnerPawn},
		OnWorldActors);
	
	const FVector2D RadarCenter = GetRadarCenter();
	const FLinearColor EnemyLinearColour = FLinearColor(EnemyColour.R, EnemyColour.G, EnemyColour.B, 0.0);
	const FLinearColor GenericActorLinearColour = FColor::Silver.ReinterpretAsLinear();
	FLinearColor OffsetAlpha(0,0,0, 0);
	
	for (const AActor* It : OnWorldActors)
	{
		const FVector2D Location2D = WorldToScreen2D(It);

		//Clamp positions in between the minimap size so they are not out of bounds
		const float NewX = FMath::Clamp<float>(Location2D.X, -RadarSize / 2, RadarSize / 2 - GenericMinimapIconRectSize);
		const float NewY = FMath::Clamp<float>(Location2D.Y, -RadarSize / 2, RadarSize / 2 - GenericMinimapIconRectSize);

		FVector MyLocation = GetCurrentActorLocation();
		FVector EnemyLocation = It->GetActorLocation();

		const float Distance = FVector::Distance(MyLocation, EnemyLocation);
		
		// // @todo Decide what type of actors this is, if enemy use 'EnemyColour' and so forth
		// Contexts to consider:: Mission / Interactable :  {MissionColour; InteractableColour}
		// Relation to consider:: Enemy / OwnedUnits / Friend : {EnemyColour; OwnedUnitsColour; FriendColour;}

		const FLinearColor EntityColour = Cast<APawn>(It) != nullptr ? EnemyLinearColour: GenericActorLinearColour; 
		OffsetAlpha.A = Alpha / Distance * 4;
		DrawRect(EntityColour + OffsetAlpha, RadarCenter.X + NewX, RadarCenter.Y + NewY, GenericMinimapIconRectSize, GenericMinimapIconRectSize);
	}
}

// @todo move calculations out of the hud, keep cached data the HUD can quickly access during the draw call wittout having to run the octree iteration themselves
void APD_HUD::DrawEntitiesOnMiniMap()
{
	// @todo consider caching
	ARTSOController* RTSOController = Cast<ARTSOController>(GetOwningPlayerController());
	if (RTSOController == nullptr || RTSOController->GetPawn() == nullptr) { return; }
	
	// Viewport halfsize
	PD::Mass::Entity::Octree& WorldOctree =  UPDRTSBaseSubsystem::Get()->WorldEntityOctree;
	
	static const FVector OffsetCorner_FwdLeft {1,  -1,  0.0};
	static const FVector OffsetCorner_FwdRight {1,   1,  0.0};
	static const FVector OffsetCorner_BwdLeft {-1,  1,  0.0};
	static const FVector OffsetCorner_BwdRight {-1, -1,  0.0};
	
	const FVector OffsetZVector = FVector{0,0, SphereHeight};
	// // assumes we are rotated along one of four cardinal directions
	FVector BoundsCenter = RTSOController->GetPawn<AGodHandPawn>()->CursorMesh->GetComponentLocation();
	const FBox MarqueeWorldBox{
		{
			// Bottom points, below player
			BoundsCenter - OffsetZVector + (OffsetCorner_FwdLeft  * RadarTraceSphereRadius),
			BoundsCenter - OffsetZVector + (OffsetCorner_FwdRight * RadarTraceSphereRadius),  
			BoundsCenter - OffsetZVector + (OffsetCorner_BwdLeft  * RadarTraceSphereRadius),             
			BoundsCenter - OffsetZVector + (OffsetCorner_BwdRight * RadarTraceSphereRadius),  
			
			// Upper Points, above player
			BoundsCenter + OffsetZVector + (OffsetCorner_FwdLeft  * RadarTraceSphereRadius),
			BoundsCenter + OffsetZVector + (OffsetCorner_FwdRight * RadarTraceSphereRadius),
			BoundsCenter + OffsetZVector + (OffsetCorner_BwdLeft  * RadarTraceSphereRadius),
			BoundsCenter + OffsetZVector + (OffsetCorner_BwdRight * RadarTraceSphereRadius)
		}};
	FVector Extent;
	MarqueeWorldBox.GetCenterAndExtents(BoundsCenter, Extent);
	
	FBoxCenterAndExtent QueryBounds = FBoxCenterAndExtent(BoundsCenter, Extent);
	QueryBounds.Center.W = QueryBounds.Extent.W = 0;

	const FMassEntityManager& EntityManager = GetWorld()->GetSubsystem<UMassEntitySubsystem>()->GetEntityManager();
	
	TArray<FLEntityCompound> OverlappingHandles;
	TArray<FVector> EntityLocations;
	WorldOctree.FindElementsWithBoundsTest(QueryBounds, [&](const FPDEntityOctreeCell& Cell)
	{
		if (Cell.EntityHandle.Index == 0) { return; }
		const int32 OwnerID = EntityManager.GetFragmentDataPtr<FRTSOFragment_Agent>(Cell.EntityHandle)->OwnerID;

		OverlappingHandles.Emplace(FLEntityCompound{Cell.EntityHandle, Cell.Bounds.Center, OwnerID});
	});

	
#if CHAOS_DEBUG_DRAW
	{
		Chaos::FDebugDrawQueue::GetInstance().DrawDebugBox(BoundsCenter, Extent, FQuat::Identity, FColor::Silver, false, 0, 0, 10.0f);
		const FVector& TextLocation = BoundsCenter + FVector(0, 0, Extent.Z * 2);
		Chaos::FDebugDrawQueue::GetInstance().DrawDebugString(TextLocation,FString("Radar Octree Trace"), nullptr, FColor::Yellow, 0, true, 2);
	}
#endif

	const FLinearColor EnemyLinearColour = FLinearColor(EnemyColour.R, EnemyColour.G, EnemyColour.B, 0.0);
	const FLinearColor OwnerLinearColour = FLinearColor(OwnedUnitsColour.R, OwnedUnitsColour.G, OwnedUnitsColour.B, 0.0);
	FLinearColor OffsetAlpha(0,0,0, 0);

	// Copy some values we'll keep calling for the following loop
	const FVector PlayerLocation = GetCurrentActorLocation();
	const FVector2D RadarCenter  = GetRadarCenter();
	const int32 CallerID         = RTSOController->GetActorID();
	
	// Iterate all found entities and draw boxes on screen to display them
	for (FLEntityCompound& EntityCompound : OverlappingHandles)
	{
		FVector EntityLocation = EntityCompound.Location;
		const FVector2D Location2D = WorldToScreen2D(EntityCompound);

		//Clamp positions in between the minimap size so they are not out of bounds
		const float NewX = FMath::Clamp<float>(Location2D.X, -RadarSize / 2, RadarSize / 2 - GenericMinimapIconRectSize);
		const float NewY = FMath::Clamp<float>(Location2D.Y, -RadarSize / 2, RadarSize / 2 - GenericMinimapIconRectSize);

		const float Distance = FVector::Distance(PlayerLocation, EntityLocation);
		
		//// @todo colour based on selection ID and/or owner ID, currently only colours green if owning and red otherwise, with no further disctions being made
		// Contexts to consider:: Mission / Interactable :  {MissionColour; InteractableColour}
		// Relation to consider:: Enemy / OwnedUnits / Friend : {EnemyColour; OwnedUnitsColour; FriendColour;}
		
		const FLinearColor EntityColour = CallerID != EntityCompound.OwnerID ? EnemyLinearColour: OwnerLinearColour; 
		OffsetAlpha.A = Alpha / Distance * 4;

		DrawRect(EntityColour + OffsetAlpha, RadarCenter.X + NewX, RadarCenter.Y + NewY, GenericMinimapIconRectSize, GenericMinimapIconRectSize);
	}
}

void APD_HUD::DrawOwnerOnMiniMap()
{
	//Clamp positions in between the minimap size so they are not out of bounds
	if (MiniMapData == nullptr || MiniMapData->ArrowTexture == nullptr)
	{
		DrawRect(FLinearColor::White, GetRadarCenter().X, GetRadarCenter().Y, GenericMinimapIconRectSize, GenericMinimapIconRectSize);
		return;
	}

	const float TextureWidth = MiniMapData->ArrowTexture->GetSurfaceWidth();
	const float TextureHeight = MiniMapData->ArrowTexture->GetSurfaceHeight();

	DrawTextureSimple(MiniMapData->ArrowTexture, GetRadarCenter().X - TextureWidth * FixedTextureScale / 2, GetRadarCenter().Y - TextureHeight * FixedTextureScale / 2, FixedTextureScale);
}

void APD_HUD::DrawRadar(FLinearColor Color)
{
	const float PosX = RadarStartLocation.X;
	const float PosY = RadarStartLocation.Y;
	const float Size = RadarSize;

	// Draw Outer Box
	DrawRect(Color, PosX, PosY, 1, Size);
	DrawRect(Color, PosX, PosY, Size, 1);
	DrawRect(Color, PosX, PosY + Size, Size, 1);
	DrawRect(Color, PosX + Size, PosY, 1, Size + 1);
	
	// Draw Horizontal Radar Line
	DrawRect(Color, PosX + Size / 2, PosY, 1, Size + 1);

	// Draw Vertical Radar Line
	DrawRect(Color, PosX, PosY + Size / 2, Size, 1);

	// Draw Background Colour @todo replace with background material 
	DrawRect(FLinearColor(0, 0, 0, 0.3f), PosX, PosY, Size, Size);
}

FVector2D APD_HUD::GetRadarCenter() const
{
	return Canvas != nullptr ? FVector2D(RadarStartLocation.X + RadarSize / 2, RadarStartLocation.Y + RadarSize / 2) : FVector2D(0, 0);
}

FVector APD_HUD::GetCurrentActorLocation() const
{
	return GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
}

FVector2D APD_HUD::WorldToScreen2D(FLEntityCompound& EntityCompound) const
{
	const APawn* Player = GetWorld()->GetFirstPlayerController()->GetPawn();
	if (Player == nullptr || EntityCompound.EntityHandle.IsValid() == false)
	{
		return FVector2D(0, 0);
	}

	FVector ActorPosition3D = Player->GetTransform().InverseTransformPosition(EntityCompound.Location);
	return WorldToScreen2D(ActorPosition3D);
}

FVector2D APD_HUD::WorldToScreen2D(const AActor * Actor) const
{
	const APawn* Player = GetWorld()->GetFirstPlayerController()->GetPawn();
	if (Player == nullptr || Actor == nullptr) { return FVector2D(0, 0); }
	
	FVector ActorPosition3D = Player->GetTransform().InverseTransformPosition(Actor->GetActorLocation());
	return WorldToScreen2D(ActorPosition3D);
}

FVector2D APD_HUD::WorldToScreen2D(FVector& WorldLocation) const
{
	WorldLocation = FRotator(0.f, -90.f, 0.f).RotateVector(WorldLocation);
	WorldLocation /= RadarDistanceScale;
	return FVector2D(WorldLocation);	
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