#include "GalavantUnreal.h"

#include "DynamicTexture.h"

#include "util/Logging.hpp"

// See https://wiki.unrealengine.com/Procedural_Materials

struct UpdateTextureRegionsParams
{
	UTexture2D* Texture;
	int32 MipIndex;
	uint32 NumRegions;
	FUpdateTextureRegion2D* Regions;
	uint32 SrcPitch;
	uint32 SrcBpp;
	uint8* SrcData;
	bool FreeData;
};

// Send the render command to update the texture
void UpdateTextureRegions(UpdateTextureRegionsParams params)
{
	if (params.Texture && params.Texture->Resource)
	{
		struct FUpdateTextureRegionsData
		{
			FTexture2DResource* Texture2DResource;
			int32 MipIndex;
			uint32 NumRegions;
			FUpdateTextureRegion2D* Regions;
			uint32 SrcPitch;
			uint32 SrcBpp;
			uint8* SrcData;
		};

		FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;

		RegionData->Texture2DResource = (FTexture2DResource*)params.Texture->Resource;
		RegionData->MipIndex = params.MipIndex;
		RegionData->NumRegions = params.NumRegions;
		RegionData->Regions = params.Regions;
		RegionData->SrcPitch = params.SrcPitch;
		RegionData->SrcBpp = params.SrcBpp;
		RegionData->SrcData = params.SrcData;

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		    UpdateTextureRegionsData, FUpdateTextureRegionsData*, RegionData, RegionData, bool,
		    FreeData, params.FreeData,
		    {
			    for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
			    {
				    int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
				    if (RegionData->MipIndex >= CurrentFirstMip)
				    {
					    RHIUpdateTexture2D(
					        RegionData->Texture2DResource->GetTexture2DRHI(),
					        RegionData->MipIndex - CurrentFirstMip,
					        RegionData->Regions[RegionIndex], RegionData->SrcPitch,
					        RegionData->SrcData +
					            RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch +
					            RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp);
				    }
			    }
			    if (FreeData)
			    {
				    FMemory::Free(RegionData->Regions);
				    FMemory::Free(RegionData->SrcData);
			    }
			    delete RegionData;
			});
	}
}

void SetDynamicTexturePixelsToColor(DynamicTexturePixel* pixels, int32 numPixels,
                                    DynamicTexturePixel color)
{
	if (pixels)
	{
		for (int i = 0; i < numPixels; i++)
			pixels[i] = color;
	}
}

DynamicTexture::~DynamicTexture()
{
	if (Pixels)
		delete[] Pixels;
	if (UpdateTextureRegion)
		delete UpdateTextureRegion;
}

void DynamicTexture::Initialize(UStaticMeshComponent* staticMesh, int32 materialIndex, int32 width,
                                int32 height)
{
	if (!staticMesh)
		return;

	Width = width;
	Height = height;

	if (!Width || !Height)
	{
		LOGE << "Dynamic Texture needs width and height in order to function!";
		return;
	}

	// Texture2D setup
	{
		Texture2D = UTexture2D::CreateTransient(Width, Height);
		// Ensure there's no compression (we're editing pixel-by-pixel)
		Texture2D->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
		// Turn off Gamma correction
		Texture2D->SRGB = 0;
		// Make sure it never gets garbage collected
		Texture2D->AddToRoot();
		// Update the texture with these new settings
		Texture2D->UpdateResource();
	}

	// TextureRegion setup
	UpdateTextureRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, Width, Height);

	// Pixels setup
	{
		NumPixels = Width * Height;
		if (Pixels)
			delete[] Pixels;
		Pixels = new DynamicTexturePixel[NumPixels];

		SetDynamicTexturePixelsToColor(Pixels, NumPixels, {0, 0, 0, 255});
	}

	// Material setup
	{
		UMaterialInstanceDynamic* material =
		    staticMesh->CreateAndSetMaterialInstanceDynamic(materialIndex);

		if (!material)
		{
			LOGE << "Couldn't get material from mesh!";
			return;
		}

		DynamicMaterials.Empty();
		DynamicMaterials.Add(material);

		for (UMaterialInstanceDynamic* dynamicMaterial : DynamicMaterials)
		{
			if (dynamicMaterial)
				dynamicMaterial->SetTextureParameterValue("DynamicTextureParam", Texture2D);
		}
	}

	Ready = true;
}

void DynamicTexture::Update()
{
	if (Ready)
	{
		UpdateTextureRegionsParams params = {
		    /*Texture = */ Texture2D,
		    /*MipIndex = */ 0,
		    /*NumRegions = */ 1,
		    /*Regions = */ UpdateTextureRegion,
		    /*SrcPitch = */ static_cast<uint32>(Width * sizeof(DynamicTexturePixel)),
		    /*SrcBpp = */ sizeof(DynamicTexturePixel),
		    /*SrcData = */ reinterpret_cast<uint8*>(Pixels),
		    /*FreeData = */ false,
		};
		UpdateTextureRegions(params);
	}
}

DynamicTexturePixel* DynamicTexture::GetPixel(int32 row, int32 column)
{
	if (!Pixels || row >= Height || column >= Width || row < 0 || column < 0)
		return nullptr;

	return &Pixels[(row * Width) + column];
}