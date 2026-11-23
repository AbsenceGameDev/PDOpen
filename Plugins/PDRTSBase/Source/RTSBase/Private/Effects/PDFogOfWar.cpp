/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Effects/PDFogOfWar.h"
#include "PDRTSCommon.h"
#include "RHICommandList.h"
#include "Rendering/Texture2DResource.h"

#include "Serialization/BufferArchive.h"
#include "Serialization/ArchiveSerializedPropertyChain.h"


void FPDWorldData::VisitedWorldLocation(const FPDVisitedWorldDatum& WorldLocation)
{
	bool bVisibilityChanged = true;
	
	if (WorldCellStates.Contains(WorldLocation.WorldPosition))
	{
		// Already In the map, update visibility state
		FPDVisitedWorldDatum& CellState = *WorldCellStates.Find(WorldLocation.WorldPosition);
		bVisibilityChanged = CellState.WorldVisibility != WorldLocation.WorldVisibility;
		CellState.WorldVisibility = WorldLocation.WorldVisibility;  
	}
	else
	{
		WorldCellStates.Emplace(WorldLocation); 
	}

	
	// Do nothing if we've not updated our visibility
	if (bVisibilityChanged == false) { return;}

	
	UpdateCellStateOnMap(WorldLocation);
	UpdateTexture();
}

void FPDWorldData::VisitedWorldLocation(const FVector& WorldLocation)
{
	// This (UPDHashGridSubsystem::GetCellIndexStatic) will clamp to the worlds cell sizes
	const FPDVisitedWorldDatum ConstructedWorldDatum{
		UPDHashGridSubsystem::GetCellIndexStatic(WorldLocation),
	EPDWorldVisibility::EVisible};

	VisitedWorldLocation(ConstructedWorldDatum);
}

void FPDWorldData::InitializeFOWTexture()
{
	if (InitCount > 0)
	{
		DeinitializeFOWTexture(); // remove previously allocated data if this is called multiple times in a row on the same object
	}
	InitCount++;
	
	
	TotalWorldSize = WorldDescriptor.SizeNegative + WorldDescriptor.SizePositive;
	FOWTextureTotalPixels = TotalWorldSize.X * TotalWorldSize.Y;

	// Get Total Bytes of Texture - Each pixel has 4 bytes for RGBA
	FOWTextureDataSize = FOWTextureTotalPixels * 4;
	FOWTextureDataSqrtSize = TotalWorldSize.X * 4;

	// Initialize Texture Data Array
	FOWTextureData = new uint8[FOWTextureDataSize];

	// Create Dynamic Texture Object
	FOWTexture = UTexture2D::CreateTransient(TotalWorldSize.X, TotalWorldSize.Y, EPixelFormat::PF_R8G8B8A8, "FOWTexture");
	FOWTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	FOWTexture->SRGB = 0;
	FOWTexture->Filter = TextureFilter::TF_Nearest;
	FOWTexture->AddToRoot();
	FOWTexture->UpdateResource();

	//Create Update Region Struct Instance
	TextureRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, TotalWorldSize.X, TotalWorldSize.Y);

	// Initial fill then update texture
	SetMapUndiscovered();	
	UpdateTexture();
}

void FPDWorldData::DeinitializeFOWTexture()
{
	if (InitCount <= 0)
	{
		return;
	}
	--InitCount;
	
	FOWTexture->RemoveFromRoot();

	// Initialize Texture Data Array
	delete [] FOWTextureData;
	delete TextureRegion;
}

void FPDWorldData::FillMap(const bool bDiscovered) const
{
	const FColor Color = bDiscovered ? FColor::White : FColor::Black;
	
	// Loop for each pixel, setting the pixels values from Color
	for (uint32 i = 0; i < FOWTextureTotalPixels; i++)
	{
		const uint32 ColourStartIdx = i * 4;
		FOWTextureData[ColourStartIdx] = Color.B;
		FOWTextureData[ColourStartIdx + 1] = Color.G;
		FOWTextureData[ColourStartIdx + 2] = Color.R;
		FOWTextureData[ColourStartIdx + 3] = Color.A;
	}	
}

void FPDWorldData::UpdateCellStateOnMap(const FPDVisitedWorldDatum& WorldLocation) const
{
	//
	// SELECT CELL VISIBILITY STATE:
	
	FColor Color = FColor::Black;
	switch (WorldLocation.WorldVisibility)
	{
	case EPDWorldVisibility::EVisible:
		Color = FColor::White;
		break;
	case EPDWorldVisibility::EObscured:
		Color = FColor::Silver;
		break;
	case EPDWorldVisibility::ENotVisible:
	default:
		break;
	}

	//
	// UPDATE WHOLE CELL:
	
	// Cell size determines how much of the world will be revealed
	const double CellSize = UPDHashGridSubsystem::Get()->UniformCellSize;

	const FVector CellClampedLocation = WorldLocation.WorldPosition.ToFloatVector(CellSize);

	constexpr static double FloatingPointTruncOffset = 0.5; 
	const double RevealEndRow = CellClampedLocation.X + CellSize + FloatingPointTruncOffset; 
	const double RevealEndCol = CellClampedLocation.Y + CellSize + FloatingPointTruncOffset;

	const int32 RowSize = TotalWorldSize.X;
	for (int32 ColStep = CellClampedLocation.Y; ColStep < RevealEndCol; ColStep++)
	{
		for (int32 RowStep = CellClampedLocation.X; RowStep < RevealEndRow; RowStep++)
		{
			const uint32 CellPixelFirstColourIdx = ((ColStep * RowSize) + RowStep) * 4;
			
			FOWTextureData[CellPixelFirstColourIdx] = Color.B;
			FOWTextureData[CellPixelFirstColourIdx + 1] = Color.G;
			FOWTextureData[CellPixelFirstColourIdx + 2] = Color.R;
			FOWTextureData[CellPixelFirstColourIdx + 3] = Color.A;	
		}

	}
}

void FPDWorldData::SetMapDiscovered() const
{
	FillMap(true);
}

void FPDWorldData::SetMapUndiscovered() const
{
	FillMap(false);
}

void FPDWorldData::UpdateTexture(bool bFreeInnerData)
{
    if (FOWTexture == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("FOW Texture tried to Update before being initialized!"));
        return;
    }

	// Defining the region data to pass into the render command. 
    struct FPDUpdateRegionData
    {
        FTexture2DResource* Texture2DResource;
        FRHITexture2D* TextureRHI;
        int32 MipIndex;
        uint32 NumRegions;
        FUpdateTextureRegion2D* Regions;
        uint32 SrcPitch;
        uint32 SrcBpp;
        uint8* SrcData;
    };


	// Create an instance of 'FPDUpdateRegionData' and set all the values based on our texture variables. 
    FPDUpdateRegionData* RegionData = new FPDUpdateRegionData;

    UTexture2D* Texture = FOWTexture;

	// Assign region data
    RegionData->Texture2DResource = static_cast<FTexture2DResource*>(Texture->GetResource());
    RegionData->TextureRHI = RegionData->Texture2DResource->GetTexture2DRHI();
    RegionData->NumRegions = 1;
    RegionData->MipIndex = 0;
    RegionData->SrcData = FOWTextureData;
    RegionData->Regions = TextureRegion;
    RegionData->SrcPitch = FOWTextureDataSqrtSize;
    RegionData->SrcBpp = 4;

	// Creating an ENQUEUE_RENDER_COMMAND, passing that region data we just defined.
    ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)(
        [RegionData, bFreeInnerData, Texture](FRHICommandListImmediate& RHICmdList)
        {
            for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
            {
	            const int32 CurrentFirstMip = Texture->FirstResourceMemMip;
                if (RegionData->TextureRHI == nullptr || RegionData->MipIndex < CurrentFirstMip)
                {
	                continue;
                }

            	// Update the Texture.
                RHIUpdateTexture2D(
                    RegionData->TextureRHI,
                    RegionData->MipIndex - CurrentFirstMip,
                    RegionData->Regions[RegionIndex],
                    RegionData->SrcPitch,
                    RegionData->SrcData
                    + RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch
                    + RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp
                );
            }
            if (bFreeInnerData)
            {
                FMemory::Free(RegionData->Regions);
                FMemory::Free(RegionData->SrcData);
            }
            delete RegionData;
        });
}

void FPDWorldData::CopyWorldFOWDataToFriendlyFormat(uint8* SourceTextureData, TArray<FColor>& TargetTextureData, uint32 PixelCount)
{
	TargetTextureData.SetNumUninitialized(PixelCount);
	
	// Loop for each pixel, setting the pixels values from Color
	for (uint32 i = 0; (i * 4) < PixelCount; i++)
	{
		const uint32 ColourStartIdx = i * 4;
		TargetTextureData[i] = FColor{
			SourceTextureData[ColourStartIdx + 0],
			SourceTextureData[ColourStartIdx + 1],
			SourceTextureData[ColourStartIdx + 2],
			SourceTextureData[ColourStartIdx + 3]
		};
	}
}

void FPDWorldData::SerializeWorldFOWData(FPDPixelArchive& Ar, uint8* SourceTextureData, uint32 PixelCount)
{
	// PixelCount should most often be: FOWTextureTotalPixels 
	// SourceTextureData should most often be: FOWTextureData
	
	TArray<FColor> OutTextureData;
	CopyWorldFOWDataToFriendlyFormat(SourceTextureData,OutTextureData, PixelCount);
	Ar << OutTextureData; 
}

void GetColoursFromArchive(FPDPixelArchive& Ar, TArray<FColor>& OutPixels)
{
	const uint32 ColourCount = Ar.Num();
	const uint32 PixelCount = ColourCount / 4; // 4 bytes per fcolor
	OutPixels.SetNum(PixelCount);
	
	// Iterate all bytes, recompose our data
	for (uint32 PixelIndex = 0; (PixelIndex * 4) < ColourCount; PixelIndex++)
	{
		const uint32 ColourByteIdx = PixelIndex * 4;
		OutPixels[PixelIndex] = FColor{
			Ar[ColourByteIdx + 0],
			Ar[ColourByteIdx + 1],
			Ar[ColourByteIdx + 2],
			Ar[ColourByteIdx + 3]
		};
	}		
}

TArray<FColor> FPDWorldData::DeserializeWorldFOWData(FPDPixelArchive& Ar)
{
	TArray<FColor> TextureData;
	GetColoursFromArchive(Ar,TextureData);
	return TextureData;
}

TArray<FColor> FPDWorldData::DeserializeWorldFOWData(FPDPixelArchive& Ar, const bool bUpdateCurrentlyDisplayedTextureData)
{
	TArray<FColor> TextureData = DeserializeWorldFOWData(Ar);

	if (bUpdateCurrentlyDisplayedTextureData)
	{
		const int32 PixelCount = TextureData.Num();
		check(FOWTextureDataSize == PixelCount); // This should always be true
		
		// Iterate all bytes, overwrite our active colour data
		for (uint32 PixelIndex = 0; (PixelIndex * 4) < FOWTextureDataSize; PixelIndex++)
		{
			const FColor Col = TextureData[PixelIndex];
			
			const uint32 ColourByteIdx = PixelIndex * 4;
			FOWTextureData[ColourByteIdx + 0] = Col.R;
			FOWTextureData[ColourByteIdx + 1] = Col.G;
			FOWTextureData[ColourByteIdx + 2] = Col.B;
			FOWTextureData[ColourByteIdx + 3] = Col.A;
		}			
	}
	
	return TextureData;
}



UPDFogOfWarSubsystem::UPDFogOfWarSubsystem()
{
	RequestedFOWState = OldFOWState = FGameplayTag{};
}

void UPDFogOfWarSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	ProcessTables();
}

bool UPDFogOfWarSubsystem::HasCompleteAuthority() const
{
	switch (GetWorld()->GetNetMode())
	{ 
	default: ;
	case NM_MAX:
	case NM_Client:
		return false;
	case NM_Standalone:
	case NM_DedicatedServer:
	case NM_ListenServer:
		break;
	}
	return true;
}

void UPDFogOfWarSubsystem::Tick(float DeltaTime)
{
	// if on server or single player, otherwise exit out early

	if (HasCompleteAuthority() == false)
	{
		Deinitialize();
		return;
	}
	
	Super::Tick(DeltaTime);
}

void UPDFogOfWarSubsystem::UpdateWorldDatum(AController* RequestedController, const FPDVisitedWorldDatum& WorldLocation)
{
	const bool bIsNewControllerValid = RequestedController != nullptr && RequestedController->IsValidLowLevelFast();
	if (TrackedControllers.Contains(RequestedController) == false || bIsNewControllerValid == false)
	{
		if (bIsNewControllerValid)
		{
			FPDWorldData InitWorldData;
			InitWorldData.WorldDescriptor = WorldDescriptor;
			InitWorldData.InitializeFOWTexture();
			
			TrackedControllers.Emplace(RequestedController, InitWorldData);
		}
		else
		{
			return;
		}
	}

	FPDWorldData* TrackedControllerWorldData = TrackedControllers.Find(RequestedController);
	if (TrackedControllerWorldData == nullptr) { return; }

	TrackedControllerWorldData->VisitedWorldLocation(WorldLocation);
}

void UPDFogOfWarSubsystem::UpdateWorldLocation(AController* RequestedController, const FVector& WorldLocation)
{
	// This (UPDHashGridSubsystem::GetCellIndexStatic) will clamp to the worlds cell sizes
	const FPDVisitedWorldDatum ConstructedWorldDatum{
		UPDHashGridSubsystem::GetCellIndexStatic(WorldLocation),
	EPDWorldVisibility::EVisible};
	
	UpdateWorldDatum(RequestedController, ConstructedWorldDatum);
}

void UPDFogOfWarSubsystem::SetCustomMode_Implementation(FGameplayTag Tag)
{
	// If TagToSettings has a valid entry, then TagToTable & TagToRowName also have the same valid entries  
	if (TagToSettings.Contains(Tag) == false)
	{
		const FString BuildString =
			"UPDFogOfWarSubsystem::SetCustomMode -- "
			"\n Trying to set mode without a valid gameplay tag. Skipping processing entry";
		UE_LOG(PDLog_RTSBase, Error, TEXT("%s"), *BuildString);
		
		return;
	}
	

	// Overwriting current selected settings
	CurrentSettingsHandle.DataTable = TagToTable.FindRef(Tag);
	CurrentSettingsHandle.RowName   = TagToRowname.FindRef(Tag);
	
	const FPDFogOfWarSettings& Settings = *TagToSettings.FindRef(Tag);

	// @todo  Apply settings 
}


TStatId UPDFogOfWarSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPDFogOfWarSubsystem, STATGROUP_Tickables);
}

void UPDFogOfWarSubsystem::ProcessTables()
{
	const UPDFogOfWarSettingsSource* FogOfWarGlobalSettings = GetDefault<UPDFogOfWarSettingsSource>();
	
	for (const TSoftObjectPtr<UDataTable> SoftObjectTablePtr : FogOfWarGlobalSettings->SettingsTables)
	{
		const UDataTable* Table = SoftObjectTablePtr.LoadSynchronous();
		if (Table == nullptr
			|| Table->IsValidLowLevelFast() == false
			|| Table->RowStruct != FPDFogOfWarSettings::StaticStruct())
		{
			continue;
		}

		TArray<FPDFogOfWarSettings*> Rows;
		Table->GetAllRows("", Rows);
		TArray<FName> RowNames = Table->GetRowNames();
		
		for (const FName& Name : RowNames)
		{
			FPDFogOfWarSettings* DefaultDatum = Table->FindRow<FPDFogOfWarSettings>(Name,"");
			check(DefaultDatum != nullptr) // This should never be nullptr

			const FGameplayTag& FOWMode = DefaultDatum->FOWMode;

			if (FOWMode.IsValid() == false)
			{
				FString BuildString = "UPDFogOfWarSubsystem::ProcessTables -- "
				+ FString::Printf(TEXT("Processing table(%s)"), *Table->GetName()) 
				+ FString::Printf(TEXT("\n Trying to add settings on row (%s) Which does not have a valid gameplay tag. Skipping processing entry"), *Name.ToString());
				UE_LOG(PDLog_RTSBase, Error, TEXT("%s"), *BuildString);

				// @todo Write some test cases and some data validation to handle this properly, the logs will do for now  
				continue;
			}
			
			// @note If duplicates, ignore duplicate and output errors to screen and to log
			if (TagToSettings.Contains(FOWMode))
			{
				const UDataTable* RetrievedTable = *TagToTable.Find(FOWMode);
				
				FString BuildString = "UPDFogOfWarSubsystem::ProcessTables -- "
				+ FString::Printf(TEXT("Processing table(%s)"), *Table->GetName()) 
				+ FString::Printf(TEXT("\n Trying to add setting(%s) which has already been added by previous table(%s)."),
						*FOWMode.GetTagName().ToString(), RetrievedTable != nullptr ? *RetrievedTable->GetName() : *FString("INVALID TABLE"));
				UE_LOG(PDLog_RTSBase, Error, TEXT("%s"), *BuildString);

				// @todo Write some test cases and some data validation to handle this properly, the logs will do for now  
				continue;
			}
			

			TagToSettings.Emplace(FOWMode) = DefaultDatum;
			TagToTable.Emplace(FOWMode) = Table;
			TagToRowname.Emplace(FOWMode) = Name;
		}
	}
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