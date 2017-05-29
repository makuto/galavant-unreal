// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "HUDMinimapActor.h"

#include "ConversionHelpers.h"

#include "util/Logging.hpp"

#include "world/ProceduralWorld.hpp"

// Sets default values
AHUDMinimapActor::AHUDMinimapActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	{
		MinimapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
		/*//"Game/Geometry/Meshes/Minimap_Plane"
		static ConstructorHelpers::FClassFinder<UStaticMesh> MinimapPlane(
		    TEXT("StaticMesh'/Game/Geometry/Meshes/Minimap_Plane'"));
		if (!MinimapPlane.Class)
		    LOGE << "Couldn't find Minimap Plane!";
		else
		    MinimapMesh->SetStaticMesh(MinimapPlane.Class);*/

		MinimapMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MinimapMesh->SetCastShadow(false);
	}

	// Establish component relationships
	RootComponent = MinimapMesh;

	TextureWidth = 128;
	TextureHeight = 128;

	SetActorEnableCollision(false);
}

void AHUDMinimapActor::UpdateMinimap(FVector& worldPosition)
{
	static FVector lastPositionUpdated(0.f, 0.f, 0.f);

	if (!IsRunningDedicatedServer() && !lastPositionUpdated.Equals(worldPosition, 1000.f))
	{
#define SAMPLE_WIDTH 128
#define SAMPLE_HEIGHT 128
		static unsigned char sampleHeights[SAMPLE_HEIGHT][SAMPLE_WIDTH];
		gv::ProceduralWorld::SampleWorldCellHeights(ToPosition(worldPosition), SAMPLE_WIDTH,
		                                            SAMPLE_HEIGHT, (unsigned char*)&sampleHeights);

		for (int row = 0; row < TextureWidth; row++)
		{
			for (int col = 0; col < TextureHeight; col++)
			{
				const uint8 banding = 10;
				unsigned char noiseValue = sampleHeights[row % SAMPLE_HEIGHT][col % SAMPLE_WIDTH];
				DynamicTexturePixel* currentPixel = MinimapTexture.GetPixel(row, col);
				currentPixel->g = ((uint8)noiseValue) % banding;
				currentPixel->b = (((uint8)noiseValue) / banding) * (255 / banding);
				currentPixel->a = 255;

				// Add a marker at the center for where the player is at
				if (abs(row - (TextureHeight / 2)) < 5 && abs(col - (TextureWidth / 2)) < 5)
					currentPixel->r = 255;
			}
		}

		MinimapTexture.Update();

		lastPositionUpdated = worldPosition;

#undef SAMPLE_WIDTH
#undef SAMPLE_WIDTH
	}
}

void AHUDMinimapActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (!IsRunningDedicatedServer())
	{
		MinimapTexture.Initialize(MinimapMesh, 0, TextureWidth, TextureHeight);

		FVector position(0.f, 0.f, 0.f);
		UpdateMinimap(position);

		MinimapTexture.Update();
	}
}

// Called when the game starts or when spawned
void AHUDMinimapActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AHUDMinimapActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	USceneComponent* SceneComponent = GetRootComponent();
	FVector worldPosition;
	if (SceneComponent)
		worldPosition = SceneComponent->GetComponentLocation();
	UpdateMinimap(worldPosition);
}
