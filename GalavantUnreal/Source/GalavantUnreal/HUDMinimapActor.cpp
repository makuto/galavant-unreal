// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "HUDMinimapActor.h"

#include "ConversionHelpers.h"

#include "util/Logging.hpp"

#include "world/ProceduralWorld.hpp"
#include "world/WorldResourceLocator.hpp"

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

void AHUDMinimapActor::UpdateMinimap(FVector& worldPosition, FRotator& rotation)
{
	if (IsRunningDedicatedServer())
		return;

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
			currentPixel->r = 0;
			currentPixel->g = ((uint8)noiseValue) % banding;
			currentPixel->b = (((uint8)noiseValue) / banding) * (255 / banding);
			currentPixel->a = 255;

			// Add a marker at the center for where the player is at
			int playerIconSize = 2;
			if (abs(row - (TextureHeight / 2)) < playerIconSize &&
			    abs(col - (TextureWidth / 2)) < playerIconSize)
				currentPixel->r = 255;

			// Draw a line for player facing
			/*if (col == TextureWidth / 2 && row <= TextureHeight / 2)
				currentPixel->r = 255;*/
			/*if (row == TextureHeight / 2 && col <= TextureWidth / 2)
				currentPixel->r = 255;*/

			if (col < 4 && row < 4)
				currentPixel->r = 255;
		}
	}
#undef SAMPLE_WIDTH
#undef SAMPLE_WIDTH

	// Draw WorldResources
	{
		gv::Position cellSize(gv::ProceduralWorld::GetActiveWorldParams().WorldCellTileSize);
		const gv::WorldResourceLocator::ResourceList* resourcesToDraw[] = {
		    gv::WorldResourceLocator::GetResourceList(gv::WorldResourceType::Agent),
		    gv::WorldResourceLocator::GetResourceList(gv::WorldResourceType::Food)};

		for (int i = 0; i < sizeof(resourcesToDraw) / sizeof(resourcesToDraw[0]); i++)
		{
			if (!resourcesToDraw[i])
				continue;

			for (const gv::WorldResourceLocator::Resource& currentResource :
			     *gv::WorldResourceLocator::GetResourceList(gv::WorldResourceType::Agent))
			{
				gv::Position positionRelativeToCenter =
				    currentResource.position - ToPosition(worldPosition);
				// Rotate positions to face player
				// TODO: This isn't going to work unless we rotate chunk sample too
				//positionRelativeToCenter = ToPosition(rotation.RotateVector(ToFVector(positionRelativeToCenter)));
				gv::Position pixelPosition = positionRelativeToCenter / cellSize;
				pixelPosition.X += TextureWidth / 2;
				pixelPosition.Y += TextureHeight / 2;

				DynamicTexturePixel* targetPixel =
				    MinimapTexture.GetPixel((int)pixelPosition.Y, (int)pixelPosition.X);
				if (targetPixel)
				{
					targetPixel->r = 0;
					targetPixel->g = (i * 255) / gv::WorldResourceType::WorldResourceType_count;
					targetPixel->b = 0;
				}
			}
		}
	}

	// Draw player direction
	{
		float offset = 10.f;
		FVector facingVector = rotation.Vector();
		facingVector.Z = 0.f;
		facingVector = facingVector.GetSafeNormal(0.001f) * offset;
		FVector pixelVector = worldPosition + facingVector;
		pixelVector.X += TextureWidth / 2;
		pixelVector.Y += TextureHeight / 2;
		DynamicTexturePixel* targetPixel = MinimapTexture.GetPixel(
		    (int)(pixelVector.Y - worldPosition.Y), (int)(pixelVector.X - worldPosition.X));
		if (targetPixel)
		{
			targetPixel->r = 255;
			targetPixel->g = 0;
			targetPixel->b = 0;
		}
	}

	MinimapTexture.Update();
}

void AHUDMinimapActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (!IsRunningDedicatedServer())
	{
		MinimapTexture.Initialize(MinimapMesh, 0, TextureWidth, TextureHeight);

		FVector position(0.f, 0.f, 0.f);
		FRotator rotation;
		UpdateMinimap(position, rotation);

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

	static float lastMinimapUpdate = 0.f;
	const float minimapUpdateRate = 0.1f;
	lastMinimapUpdate += DeltaTime;
	if (lastMinimapUpdate >= minimapUpdateRate)
	{
		FVector worldPosition;
		FRotator rotation;
		AActor* parentActor = GetParentActor();
		USceneComponent* SceneComponent = parentActor ? parentActor->GetRootComponent() : nullptr;
		if (SceneComponent)
		{
			// We don't want to use the minimap's position/rotation because it follows the camera,
			// which when pitched mucks with our yaw (the only axis we care about)
			worldPosition = SceneComponent->GetComponentLocation();
			rotation = SceneComponent->GetComponentRotation();
		}

		UpdateMinimap(worldPosition, rotation);
		lastMinimapUpdate = 0.f;
	}
}
