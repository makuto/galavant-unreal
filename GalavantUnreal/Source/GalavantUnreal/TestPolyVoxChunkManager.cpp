#include "GalavantUnreal.h"
#include "TestPolyVoxChunkManager.h"

#include "TestPolyVoxChunk.h"

// Sets default values
ATestPolyVoxChunkManager::ATestPolyVoxChunkManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create components
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));

	// Establish component relationships
	RootComponent = SceneComponent;

	PropertiesChanged = false;

	// Property defaults
	ChunkSpawnRadius = 12800.f;
	MaxNumChunks = 75;
	Use3dNoise = false;

	TimeSinceLastUpdate = 9999.f;  // Mark as needing update
}

void ATestPolyVoxChunkManager::Destroyed()
{
	FlushPersistentDebugLines(GetWorld());
}

// Disabled, because you cannot programmatically create objects in editor mode, unfortunately
bool ATestPolyVoxChunkManager::ShouldTickIfViewportsOnly() const
{
	return false;
}

FVector ATestPolyVoxChunkManager::GetChunkManagerLocation()
{
	FVector worldLocation = SceneComponent->GetComponentLocation();

	// Our chunk manager shouldn't care about the Z axis. Otherwise, player movement in the Z axis
	// would change our chunk Z axis positioning. If we make 3D chunks (!), this would be removed
	worldLocation[2] = 0.f;
	return worldLocation;
}

// Generates a list of points where chunks should be placed for location and spawnRadius
int GeneratePointsForChunkSpawn(TArray<FVector> &pointsOut, FVector &location, float spawnRadius,
                                FVector chunkSize)
{
	int numPositions = 0;
	int numChunksWidth = spawnRadius / chunkSize[0];

	for (int x = -numChunksWidth; x < numChunksWidth; x++)
	{
		int numChunksLength = spawnRadius / chunkSize[1];
		for (int y = -numChunksLength; y < numChunksLength; y++)
		{
			FVector newChunkLocation(location);

			newChunkLocation[0] += x * chunkSize[0];
			newChunkLocation[1] += y * chunkSize[1];

			// Because I'm dumb and just generated a rectangular grid, cull points in radius so that
			//  it's circular
			float chunkHalfWidth = chunkSize.Size() / 2;
			if (FVector::Dist(location, newChunkLocation) + chunkHalfWidth > spawnRadius)
				continue;

			pointsOut.Add(newChunkLocation);
			numPositions++;
		}
	}

	return numPositions;
}

void ATestPolyVoxChunkManager::DrawDebugVisualizations(TArray<FVector> *chunkPositions = nullptr)
{
	UWorld *world = GetWorld();
	FVector worldPosition = GetChunkManagerLocation();

	FlushPersistentDebugLines(world);
	DrawDebugSphere(world, worldPosition, ChunkSpawnRadius, 32, FColor(255, 0, 0), true);

	// If provided, draw the expected chunk positions
	if (chunkPositions)
	{
		for (auto &position : (*chunkPositions))
		{
			const float POINT_SCALE = 10.f;
			DrawDebugPoint(world, position, POINT_SCALE, FColor(0, 0, 255), true, 0.f);
		}
	}
}

#if WITH_EDITOR
void ATestPolyVoxChunkManager::PostEditChangeProperty(struct FPropertyChangedEvent &e)
{
	PropertiesChanged = true;

	// This doesn't work because the chunk size might not actually be known yet
	/*// Create needed chunks in spawn radius
	TArray<FVector> requiredChunkPositions;
	FVector worldPosition = SceneComponent->GetComponentLocation();
	GeneratePointsForChunkSpawn(requiredChunkPositions, worldPosition, ChunkSpawnRadius, ChunkSize);

	std::cout << requiredChunkPositions.Num() << "\n";*/

	DrawDebugVisualizations();

	Super::PostEditChangeProperty(e);
}
#endif

ATestPolyVoxChunk *ATestPolyVoxChunkManager::CreateChunk(FVector &location, FRotator &rotation)
{
	FActorSpawnParameters spawnParams;
	spawnParams.Owner = this;
	ATestPolyVoxChunk *newChunk = (ATestPolyVoxChunk *)GetWorld()->SpawnActor<ATestPolyVoxChunk>(
	    location, rotation, spawnParams);

	newChunk->SetUse3dNoise(Use3dNoise);

	newChunk->Construct();

	if (newChunk)
	{
		UE_LOG(LogGalavantUnreal, Log, TEXT("Spawned chunk named '%s' with size %s"),
		       *newChunk->GetHumanReadableName(), *newChunk->GetChunkSize().ToString());
	}
	else
		UE_LOG(LogGalavantUnreal, Log, TEXT("ERROR: Failed to spawn new chunk!"));

	return newChunk;
}

// Called when the game starts or when spawned
void ATestPolyVoxChunkManager::BeginPlay()
{
	Super::BeginPlay();

	FVector location(400.f, 0.f, 0.f);
	FRotator rotation(0.f, 0.f, 0.f);

	// Test creating a chunk
	ATestPolyVoxChunk *newChunk = CreateChunk(location, rotation);

	// This needs to happen before Tick. Need to have a cleaner way of getting
	//  chunk size; maybe finally put Cell and shit in its own file?
	if (newChunk)
	{
		ChunkSize = newChunk->GetChunkSize();
	}

	newChunk->Destroy();
}

void ATestPolyVoxChunkManager::DestroyUnneededChunks()
{
	FVector worldPosition = GetChunkManagerLocation();

	// Delete any chunks outside spawn radius
	for (int i = Chunks.Num() - 1; i >= 0; i--)
	{
		ATestPolyVoxChunk *chunk = Chunks[i];
		if (!chunk)
			continue;

		FVector chunkPosition = chunk->GetActorLocation();

		// Used to center chunk position so as to make the radius check better
		float chunkHalfWidth = chunk->GetChunkSize().Size() / 2;

		// If the chunk is outside the spawn radius, destroy it
		if (FVector::Dist(worldPosition, chunkPosition) + chunkHalfWidth > ChunkSpawnRadius)
		{
			UE_LOG(LogGalavantUnreal, Log, TEXT("Destroying chunk '%s' (%f units away)"),
			       *chunk->GetHumanReadableName(),
			       FVector::Dist(worldPosition, chunkPosition) + chunkHalfWidth);
			chunk->Destroy();
			Chunks.RemoveAtSwap(i);
		}
	}
}

// Called every frame
void ATestPolyVoxChunkManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float UPDATE_FREQUENCY = 1.f;

	FVector worldPosition = GetChunkManagerLocation();

	TimeSinceLastUpdate += DeltaTime;

	if ((TimeSinceLastUpdate > UPDATE_FREQUENCY &&
	     worldPosition.Equals(LastUpdatedPosition, 0.1f) == false) ||
	    PropertiesChanged)
	{
		TimeSinceLastUpdate = 0.f;
		LastUpdatedPosition = worldPosition;
		PropertiesChanged = false;

		DestroyUnneededChunks();

		// Create needed chunks in spawn radius
		TArray<FVector> requiredChunkPositions;
		GeneratePointsForChunkSpawn(requiredChunkPositions, worldPosition, ChunkSpawnRadius,
		                            ChunkSize);

		for (auto &newChunkPosition : requiredChunkPositions)
		{
			// Make sure we don't spawn too many chunks
			if (Chunks.Num() > MaxNumChunks)
				break;

			// Make sure the chunk doesn't already exist (good lord)
			bool alreadyExists = false;
			for (auto &chunk : Chunks)
			{
				if (!chunk)
					continue;

				if (newChunkPosition.Equals(chunk->GetActorLocation(), 10.f))
					alreadyExists = true;
			}

			if (!alreadyExists)
			{
				FRotator defaultRotation(0.f, 0.f, 0.f);

				ATestPolyVoxChunk *newChunk = CreateChunk(newChunkPosition, defaultRotation);

				if (newChunk)
					Chunks.Add(newChunk);
			}
		}

		DrawDebugVisualizations(&requiredChunkPositions);
	}
}

void ATestPolyVoxChunkManager::SetChunkSpawnRadius(float radius)
{
	ChunkSpawnRadius = radius;
}