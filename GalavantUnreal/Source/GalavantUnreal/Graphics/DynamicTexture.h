#pragma once

// See https://wiki.unrealengine.com/Procedural_Materials

struct DynamicTexturePixel
{
	// Do not rearrage or add members to this struct
	uint8 b;
	uint8 g;
	uint8 r;
	uint8 a;
};

void SetDynamicTexturePixelsToColor(DynamicTexturePixel* pixels, int32 numPixels,
                                    DynamicTexturePixel color);

struct DynamicTexture
{
private:
	TArray<class UMaterialInstanceDynamic*> DynamicMaterials;
	UTexture2D* Texture2D;
	FUpdateTextureRegion2D* UpdateTextureRegion;

	bool Ready;

public:
	int32 Width;
	int32 Height;

	int32 NumPixels;

	// You can modify this as desired (it should be null when InitializeDynamicTexture() is called).
	// Call Update() once you've finished modifying the Pixels to let Unreal know we want to use the
	// new ones
	DynamicTexturePixel* Pixels;

	~DynamicTexture();

	void Initialize(UStaticMeshComponent* staticMesh, int32 materialIndex, int32 width,
	                int32 height);

	void Update();

	DynamicTexturePixel* GetPixel(int32 row, int32 column);
};
