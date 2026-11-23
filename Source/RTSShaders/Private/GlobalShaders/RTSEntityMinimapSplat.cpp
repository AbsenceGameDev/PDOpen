/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "GlobalShaders/RTSEntityMinimapSplat.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "Materials/Material.h"

#define ENTITYDEBUG 0

#if ENTITYDEBUG == 1
uint32_t Colors(int32_t Index)
{
    static const uint32_t ColorIDs[256] = { 0xB88183, 0x922329, 0x5A0007, 0xD7BFC2, 0xD86A78, 0xFF8A9A, 0x3B000A, 0xE20027, 0x943A4D, 0x5B4E51, 0xB05B6F, 0xFEB2C6, 0xD83D66, 0x895563, 0xFF1A59, 0xFFDBE5, 0xCC0744, 0xCB7E98, 0x997D87, 0x6A3A4C, 0xFF2F80, 0x6B002C, 0xA74571, 0xC6005A, 0xFF5DA7, 0x300018, 0xB894A6, 0xFF90C9, 0x7C6571, 0xA30059, 0xDA007C, 0x5B113C, 0x402334, 0xD157A0, 0xDDB6D0, 0x885578, 0x962B75, 0xA97399, 0xD20096, 0xE773CE, 0xAA5199, 0xE704C4, 0x6B3A64, 0xFFA0F2, 0x6F0062, 0xB903AA, 0xC895C5, 0xFF34FF, 0x320033, 0xDBD5DD, 0xEEC3FF, 0xBC23FF, 0x671190, 0x201625, 0xF5E1FF, 0xBC65E9, 0xD790FF, 0x72418F, 0x4A3B53, 0x9556BD, 0xB4A8BD, 0x7900D7, 0xA079BF, 0x958A9F, 0x837393, 0x64547B, 0x3A2465, 0x353339, 0xBCB1E5, 0x9F94F0, 0x9695C5, 0x0000A6, 0x000035, 0x636375, 0x00005F, 0x97979E, 0x7A7BFF, 0x3C3E6E, 0x6367A9, 0x494B5A, 0x3B5DFF, 0xC8D0F6, 0x6D80BA, 0x8FB0FF, 0x0045D2, 0x7A87A1, 0x324E72, 0x00489C, 0x0060CD, 0x789EC9, 0x012C58, 0x99ADC0, 0x001325, 0xDDEFFF, 0x59738A, 0x0086ED, 0x75797C, 0xBDC9D2, 0x3E89BE, 0x8CD0FF, 0x0AA3F7, 0x6B94AA, 0x29607C, 0x404E55, 0x006FA6, 0x013349, 0x0AA6D8, 0x658188, 0x5EBCD1, 0x456D75, 0x0089A3, 0xB5F4FF, 0x02525F, 0x1CE6FF, 0x001C1E, 0x203B3C, 0xA3C8C9, 0x00A6AA, 0x00C6C8, 0x006A66, 0x518A87, 0xE4FFFC, 0x66E1D3, 0x004D43, 0x809693, 0x15A08A, 0x00846F, 0x00C2A0, 0x00FECF, 0x78AFA1, 0x02684E, 0xC2FFED, 0x47675D, 0x00D891, 0x004B28, 0x8ADBB4, 0x0CBD66, 0x549E79, 0x1A3A2A, 0x6C8F7D, 0x008941, 0x63FFAC, 0x1BE177, 0x006C31, 0xB5D6C3, 0x3D4F44, 0x4B8160, 0x66796D, 0x71BB8C, 0x04F757, 0x001E09, 0xD2DCD5, 0x00B433, 0x9FB2A4, 0x003109, 0xA3F3AB, 0x456648, 0x51A058, 0x83A485, 0x7ED379, 0xD1F7CE, 0xA1C299, 0x061203, 0x1E6E00, 0x5EFF03, 0x55813B, 0x3B9700, 0x4FC601, 0x1B4400, 0xC2FF99, 0x788D66, 0x868E7E, 0x83AB58, 0x374527, 0x98D058, 0xC6DC99, 0xA4E804, 0x76912F, 0x8BB400, 0x34362D, 0x4C6001, 0xDFFB71, 0x6A714A, 0x222800, 0x6B7900, 0x3A3F00, 0xBEC459, 0xFEFFE6, 0xA3A489, 0x9FA064, 0xFFFF00, 0x61615A, 0xFFFFFE, 0x9B9700, 0xCFCDAC, 0x797868, 0x575329, 0xFFF69F, 0x8D8546, 0xF4D749, 0x7E6405, 0x1D1702, 0xCCAA35, 0xCCB87C, 0x453C23, 0x513A01, 0xFFB500, 0xA77500, 0xD68E01, 0xB79762, 0x7A4900, 0x372101, 0x886F4C, 0xA45B02, 0xE7AB63, 0xFAD09F, 0xC0B9B2, 0x938A81, 0xA38469, 0xD16100, 0xA76F42, 0x5B4534, 0x5B3213, 0xCA834E, 0xFF913F, 0x953F00, 0xD0AC94, 0x7D5A44, 0xBE4700, 0xFDE8DC, 0x772600, 0xA05837, 0xEA8B66, 0x391406, 0xFF6832, 0xC86240, 0x29201D, 0xB77B68, 0x806C66, 0xFFAA92, 0x89412E, 0xE83000, 0xA88C85, 0xF7C9BF, 0x643127, 0xE98176, 0x7B4F4B, 0x1E0200, 0x9C6966, 0xBF5650, 0xBA0900, 0xFF4A46, 0xF4ABAA, 0x000000, 0x452C2C, 0xC8A1A1 };
    return ColorIDs[Index % 256];
}
FLinearColor HexToCol(uint32_t Hex)
{
    float R = float((Hex & 0x00ff0000) >> 16) / 255;
    float G = float((Hex & 0x0000ff00) >> 8) / 255;
    float B = float(Hex & 0x000000ff) / 255.0;
    return FLinearColor(R, G, B);
}
bool InBounds(FIntVector2 Pos) {return (Pos.X >= 0 && Pos.X < 256 && Pos.Y >= 0 && Pos.Y < 256);}

void SplatEntity(FVector2D ExactPixelPos, int HalfWidth, int HalfHeight)
{
    for (int x = -HalfWidth; x <= HalfWidth; x++)
    {
        for (int y = -HalfHeight; y <= HalfHeight; y++)
        {
            FIntVector2 CurrentPixel = FIntVector2(ExactPixelPos.X, ExactPixelPos.Y) + FIntVector2(x, y);
            if (InBounds(CurrentPixel))
            {
				UE_LOG(LogTemp, Warning, TEXT("======= ENTITY PIXEL(%i, %i) IS IN MINIMAP BOUNDS"), x,y)
            }
			else
			{
				UE_LOG(LogTemp, Error, TEXT("======= ERROR: ENTITY PIXEL(%i, %i) IS IN NOT MINIMAP BOUNDS"), x,y)
			}
        }
    }
}

//
// Note: Need to move this to some shared space, currently have it defined in another plugin also
struct FPDRTSPerPixelStorageHelper
{
	FORCEINLINE static FLinearColor ConstructData(FVector Location, uint16_t Entity16WayRotation, uint8_t EntityFlags, uint16_t TeamColourId)
	{
 		const uint32_t ConstructedAlphaChannel = Entity16WayRotation | uint32_t(EntityFlags) >> 7 | uint32_t(TeamColourId) >> 15;
    	return FLinearColor(
    		Location.X, 
    		Location.Y,
    		Location.Z, 
    		*reinterpret_cast<const float*>(&ConstructedAlphaChannel));
	}

	FORCEINLINE static uint32 ConstructData(uint16_t Entity16WayRotation, uint8_t EntityFlags, uint8_t TeamColourId)
	{
 		return Entity16WayRotation | uint32_t(EntityFlags) >> 7 | uint32_t(TeamColourId) >> 15;
	}


	FORCEINLINE static void DeconstructData(const FLinearColor& InData, FVector& OutLocation, uint16_t& OutEntity16WayRotation, uint8_t& OutEntityFlags, uint16_t& OutTeamColourId)
	{
		OutLocation.X = InData.R;
		OutLocation.Y = InData.G;
		OutLocation.Z = InData.B;

		const uint32 AlphaAsBits = *reinterpret_cast<const uint32*>(&InData.A);
		OutEntity16WayRotation = AlphaAsBits > (32-4);
		OutEntityFlags = (static_cast<uint32>(AlphaAsBits > (32-15)) < 7);
		OutTeamColourId = AlphaAsBits < 15;
	}	
};
#endif

void FRTSMinimapSplat::BuildAndExecuteGraph(FRHICommandListImmediate& RHICmdList, UTextureRenderTarget2D* RenderTarget, const TRefCountPtr<FRDGPooledBuffer>& EntityInputPooledBuffer, const TArray<FLinearColor>& InData, 
	const float CameraYawInRads, 
	const FVector& RegionMin, 
	const FVector& RegionSize)
{
	FTextureRenderTargetResource* RTResource = RenderTarget != nullptr ? RenderTarget->GetRenderTargetResource() : nullptr;
	FRHITexture* TextureRHI = RTResource != nullptr ? RTResource->GetRenderTargetTexture() : nullptr;
	const bool bValidTarget = TextureRHI != nullptr && EnumHasAnyFlags(TextureRHI->GetDesc().Flags, TexCreate_UAV);
	const int32 NumEntities = FMath::Min(InData.Num(), EntityInputPooledBuffer.IsValid() ? static_cast<int32>(EntityInputPooledBuffer->Desc.NumElements) : 0);
	if (bValidTarget == false || NumEntities == 0) { return; } // no UAV-capable target or nothing to upload


	FRDGBuilder GraphBuilder(RHICmdList);
	FRTSMinimapSplat::FParameters* AllocatedPassParameter = GraphBuilder.AllocParameters<FRTSMinimapSplat::FParameters>();
	AllocatedPassParameter->CosCameraYawLookDirection = FMath::Cos(CameraYawInRads);
	AllocatedPassParameter->SinCameraYawLookDirection = FMath::Sin(CameraYawInRads);

	// Buffer UAV
	FRDGBufferRef DataBufferRDG = GraphBuilder.RegisterExternalBuffer(
		EntityInputPooledBuffer,
		*FString(TEXT("SortDataBuffer")),
		ERDGBufferFlags::MultiFrame
	);
	// GraphBuilder.QueueBufferUpload(DataBufferRDG, TArrayView<const FLinearColor>(InData), ERDGInitialDataFlags::None);
	GraphBuilder.QueueBufferUpload(DataBufferRDG, TArrayView<const FLinearColor>(InData.GetData(), NumEntities), ERDGInitialDataFlags::None);

	// FRDGBufferSRVDesc SRVDesc(DataBufferRDG, EPixelFormat::PF_A32B32G32R32F);
	// FRDGBufferSRVRef EntitySRV = GraphBuilder.CreateSRV(SRVDesc);
	FRDGBufferSRVRef EntitySRV = GraphBuilder.CreateSRV(DataBufferRDG); // structured SRV, matches StructuredBuffer<float4> in MinimapSplat.usf
	AllocatedPassParameter->EntityData = EntitySRV;
	

//	// TODO: Move block to function: Wrap rendertarget RHI texture inot scene render target item and set up a pool render targer desc
//	FRDGTextureRef OutTextureRef;
//	FTextureRenderTargetResource* RTResource = RenderTarget->GetRenderTargetResource();
//	{
//		FTexture2DRHIRef TextureRHI = RTResource->GetTexture2DRHI();
//
//		FSceneRenderTargetItem Item;
//		Item.TargetableTexture = TextureRHI;
//		Item.ShaderResourceTexture = TextureRHI;
//		
//		FPooledRenderTargetDesc Desc = FPooledRenderTargetDesc::Create2DDesc(
//			FIntPoint(RenderTarget->SizeX, RenderTarget->SizeY), 
//	    	PF_A32B32G32R32F, 
//	    	FClearValueBinding::None, 
//	    	TexCreate_None, 
//	    	TexCreate_ShaderResource | TexCreate_UAV, 
//			false);
//
//		TRefCountPtr<IPooledRenderTarget> PooledRT;
//		GRenderTargetPool.CreateUntrackedElement(Desc, PooledRT, Item);
//
//		OutTextureRef = GraphBuilder.RegisterExternalTexture(PooledRT, TEXT("MinimapRTOuput"));
//	}

	// Descriptor derived from the real RHI texture, so format and flags match the resource
	FRDGTextureRef OutTextureRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(TextureRHI, TEXT("MinimapRTOuput")));


	FRDGTextureUAVDesc OutTextureUAVDesc(OutTextureRef);
	FRDGTextureUAVRef TextureBufferUAV = GraphBuilder.CreateUAV(OutTextureUAVDesc, ERDGUnorderedAccessViewFlags::None);
	AllocatedPassParameter->OutSortedEntityDataTexture = TextureBufferUAV;

	AddClearUAVPass(GraphBuilder, TextureBufferUAV, FLinearColor::Transparent);

#if ENTITYDEBUG == 1
	UE_LOG(LogTemp, Warning, TEXT("=============================================="))
	UE_LOG(LogTemp, Warning, TEXT("============= ENTITY DEBUG START ============="))
	int32 Step = 0;
	for(FLinearColor Datum : InData)
	{
		UE_LOG(LogTemp, Warning, TEXT("======= ENTITY %i ======="), Step++)
		
		FVector OutLocation = FVector::ZeroVector;
		uint16_t OutEntity16WayRotation = 0;
		uint8_t OutEntityFlags = 0; 
		uint16_t OutTeamColourId = 0;
		FPDRTSPerPixelStorageHelper::DeconstructData(Datum, OutLocation, OutEntity16WayRotation, OutEntityFlags, OutTeamColourId);
		UE_LOG(LogTemp, Warning, TEXT("======= Loc xyz[%f, %f, %f]"), OutLocation.X, OutLocation.Y, OutLocation.Z)
		UE_LOG(LogTemp, Warning, TEXT("======= Packed Data Rotation index %i"), OutEntity16WayRotation)
		UE_LOG(LogTemp, Warning, TEXT("======= Packed Data Entity Flags %i"), OutEntityFlags)
		UE_LOG(LogTemp, Warning, TEXT("======= Packed Data Team Colour %s"), *HexToCol(Colors(OutTeamColourId)).ToString())
		
	    // Convert world to 0-255 pixel space
	    FVector2D ExactPixelPos = (FVector2D(OutLocation) - FVector2D(RegionMin)) / FVector2D(RegionSize) * 255.0;
		UE_LOG(LogTemp, Warning, TEXT("======= ExactPixelPos xy(%f, %f)"), ExactPixelPos.X, ExactPixelPos.Y)
		
	    SplatEntity(ExactPixelPos, 2, 2);
		UE_LOG(LogTemp, Warning, TEXT("========================="), Step++)

	}
	UE_LOG(LogTemp, Warning, TEXT("============== ENTITY DEBUG END =============="))
	UE_LOG(LogTemp, Warning, TEXT("=============================================="))
#endif

	constexpr uint32 GroupSize = 64; 
	AllocatedPassParameter->RegionMin = FVector2f{static_cast<float>(RegionMin.X), static_cast<float>(RegionMin.Y)};
	AllocatedPassParameter->RegionSize = FVector2f{static_cast<float>(RegionSize.X), static_cast<float>(RegionSize.Y)};

	// AllocatedPassParameter->NumEntities = InData.Num();
	AllocatedPassParameter->NumEntities = NumEntities;

	TShaderMapRef<FRTSMinimapSplat> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	// FRDGPassRef PassRef = FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("RTSMinimapSplat"), ComputeShader, AllocatedPassParameter, FComputeShaderUtils::GetGroupCount(InData.Num(), GroupSize));	
	FRDGPassRef PassRef = FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("RTSMinimapSplat"), ComputeShader, AllocatedPassParameter, FComputeShaderUtils::GetGroupCount(NumEntities, GroupSize));	

	GraphBuilder.SetTextureAccessFinal(OutTextureRef, ERHIAccess::SRVMask);
	GraphBuilder.Execute();
}

IMPLEMENT_GLOBAL_SHADER(FRTSMinimapSplat, "/Project/MinimapSplat.usf", "MinimapSplat", SF_Compute);



// Unused since changing plans 
IMPLEMENT_GLOBAL_SHADER(FRTSMinimapSortInnerCheat, "/Project/SortData.usf", "SortDataInnerCheat", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FRTSMinimapSortFirstPass, "/Project/SortData.usf", "SortDataFirstPass", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FRTSMinimapSortCopyToTexture, "/Project/SortData.usf", "CopyToTexture", SF_Compute);


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

The Business Source License (this document, or the “License”) is not an Shaders
Source license. However, the Licensed Work will eventually be made available
under an Shaders Source License, as stated in this License.

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