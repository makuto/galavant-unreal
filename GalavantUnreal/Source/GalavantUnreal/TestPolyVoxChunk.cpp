#include "GalavantUnreal.h"
#include "TestPolyVoxChunk.h"

#include "noise/noise.hpp"
#include "PolyVoxCore/CubicSurfaceExtractorWithNormals.h"
#include "PolyVoxCore/MarchingCubesSurfaceExtractor.h"
#include "PolyVoxCore/SurfaceMesh.h"
#include "PolyVoxCore/SimpleVolume.h"
#include "util/Logging.hpp"
#include <vector>
#include <stdlib.h>

const int CELL_X = 16;  // 32;
const int CELL_Y = 16;  // 32;
const int CELL_Z = 64;  // 64;  // In Unreal, Z is up axis

/*
TODO: Copy the DynamicMeshComponent from this page and use it instead (supports both collision and
materials, though
 there may be some complications with collision in editor vs. packaged which I should check on)
https://wiki.unrealengine.com/Procedural_Mesh_Generation
*/

void createSphereInVolume(PolyVox::SimpleVolume<uint8_t>& volData, float fRadius)
{
	// This vector hold the position of the center of the volume
	PolyVox::Vector3DFloat v3dVolCenter(volData.getWidth() / 2, volData.getHeight() / 2,
	                                    volData.getDepth() / 2);

	// This three-level for loop iterates over every voxel in the volume
	for (int z = 0; z < volData.getDepth(); z++)
	{
		for (int y = 0; y < volData.getHeight(); y++)
		{
			for (int x = 0; x < volData.getWidth(); x++)
			{
				// Store our current position as a vector...
				PolyVox::Vector3DFloat v3dCurrentPos(x, y, z);
				// And compute how far the current position is from the center of the volume
				float fDistToCenter = (v3dCurrentPos - v3dVolCenter).length();

				uint8_t uVoxelValue = 0;

				// If the current voxel is less than 'radius' units from the center then we make it
				// solid.
				if (fDistToCenter <= fRadius)
				{
					// Our new voxel value
					uVoxelValue = 255;
				}

				// Wrte the voxel value into the volume
				volData.setVoxelAt(x, y, z, uVoxelValue);
			}
		}
	}
}

void createRandomInSphereVolume(PolyVox::SimpleVolume<uint8_t>& volData, float fRadius)
{
	// This vector hold the position of the center of the volume
	PolyVox::Vector3DFloat v3dVolCenter(volData.getWidth() / 2, volData.getHeight() / 2,
	                                    volData.getDepth() / 2);

	// This three-level for loop iterates over every voxel in the volume
	for (int z = 0; z < volData.getDepth(); z++)
	{
		for (int y = 0; y < volData.getHeight(); y++)
		{
			for (int x = 0; x < volData.getWidth(); x++)
			{
				// Store our current position as a vector...
				PolyVox::Vector3DFloat v3dCurrentPos(x, y, z);
				// And compute how far the current position is from the center of the volume
				float fDistToCenter = (v3dCurrentPos - v3dVolCenter).length();

				uint8_t uVoxelValue = 0;

				// If the current voxel is less than 'radius' units from the center then we make it
				// solid.
				if (fDistToCenter <= fRadius && x % 3 == 0)
				{
					// Our new voxel value
					uVoxelValue = 255;
				}

				// Wrte the voxel value into the volume
				volData.setVoxelAt(x, y, z, uVoxelValue);
			}
		}
	}
}

// This should be a simple 1d -> 2d array. Come on, man
typedef std::vector<uint8_t> NoiseStrip;
typedef std::vector<NoiseStrip*> NoiseMap;
void get2dNoiseMapForPosition(NoiseMap* map, int width, int height, float xOffset, float yOffset,
                              float scale, int32 seed)
{
	if (!map)
		return;

	gv::Noise2d noiseGenerator(seed);

	for (int y = 0; y < height; y++)
	{
		// Create a new X array, if necessary
		if (y >= map->size())
		{
			NoiseStrip* newStrip = new NoiseStrip;
			newStrip->resize(width);
			map->push_back(newStrip);
		}

		for (int x = 0; x < width; x++)
		{
			NoiseStrip* currentStrip = map->at(y);

			if (!currentStrip)
				break;

			float noiseValue = noiseGenerator.scaledOctaveNoise2d(
			    (x + xOffset) * scale, (y + yOffset) * scale, 0, 255, 10, 0.1f, 0.55f, 2);

			if (x >= currentStrip->size())
				currentStrip->push_back(noiseValue);
			else
				(*currentStrip)[x] = noiseValue;
		}
	}
}

uint8_t get2dNoiseMapValue(NoiseMap* map, int x, int y)
{
	if (map)
	{
		NoiseStrip* strip = map->at(y);
		if (strip)
			return strip->at(x);
	}

	return 0;
}

void drawDebugPointsForPosition(UWorld* world, FVector& position, float noiseScale, int noiseSeed,
                                float meshScale)
{
	const float POINT_SCALE = 10.f;
	static NoiseMap noiseMap;
	get2dNoiseMapForPosition(&noiseMap, CELL_X, CELL_Y, position.X, position.Y, noiseScale,
	                         noiseSeed);

	for (int y = 0; y < noiseMap.size(); y++)
	{
		NoiseStrip* currentStrip = noiseMap.at(y);

		if (!currentStrip)
			continue;

		for (int x = 0; x < currentStrip->size(); x++)
		{
			float noiseValue = get2dNoiseMapValue(&noiseMap, x, y);
			FVector point((x * meshScale) + position.X, (y * meshScale) + position.Y,
			              (noiseValue / 255) * CELL_Z * meshScale);

			DrawDebugPoint(world, point, POINT_SCALE, FColor(255, 0, 0), true, 0.f);
		}
	}
}

float lerp(float startA, float stopA, float pointA, float startB, float stopB)
{
	return (((pointA - startA) * (stopB - startB)) / (stopA - startA)) + startB;
}

void create3dNoise(PolyVox::SimpleVolume<uint8_t>* volData, float xOffset, float yOffset,
                   float zOffset, float scale, int32 seed)
{
	static gv::Noise3d noiseGenerator(seed);
	for (int z = 0; z < volData->getDepth(); z++)
	{
		for (int y = 0; y < volData->getHeight(); y++)
		{
			for (int x = 0; x < volData->getWidth(); x++)
			{
				float noiseX = (x + xOffset) * scale;
				float noiseY = (y + yOffset) * scale;
				float noiseZ = (z + zOffset) * scale;

				uint8_t value = noiseGenerator.scaledOctaveNoise3d(noiseX, noiseY, noiseZ, 0, 255,
				                                                   10, 0.1f, 0.55f, 2);

				volData->setVoxelAt(x, y, z, value);
			}
		}
	}
}

void createHeightNoise(PolyVox::SimpleVolume<uint8_t>* volData, float xOffset, float yOffset,
                       float zOffset, float scale, int32 seed)
{
	LOGV << "Using noise\n";

	// Static so no memory allocations after first run of get2dNoiseMapForPosition, which supports
	// this static functionality
	static NoiseMap noiseMap;
	get2dNoiseMapForPosition(&noiseMap, volData->getWidth(), volData->getHeight(), xOffset, yOffset,
	                         scale, seed);

	for (int z = 0; z < volData->getDepth(); z++)
	{
		for (int y = 0; y < volData->getHeight(); y++)
		{
			for (int x = 0; x < volData->getWidth(); x++)
			{
				int halfDepth = volData->getDepth() / 2;
				uint8_t noiseValue = get2dNoiseMapValue(&noiseMap, x, y);
				int noiseHeightValue = (noiseValue / 255.f) * volData->getDepth();

				/*
				Because we have 2D noise, but want 3D values, we must generate a discrete
				 value for each Z. Meshing systems will perform smoother results if the data
				 is smoother
				I just realized this lerping scheme will not result in
				 half density at the noise-specified point - that will be reached at 128, which
				 is not necessarily at the noise point (somewhere inbetween). This should be
				 alright, though it makes it less clear to the noise algorithm where the final
				 point will be
				If the current Z is greater than the noise height, lerp between the noise height
				 and 0 (no density)
				If the current Z is less than the noise height, lerp between 255 (full density)
				 and the noise value
				 */
				uint8_t relativeValue = z > noiseHeightValue ?
				                            lerp(halfDepth, volData->getDepth(), z, noiseValue, 0) :
				                            lerp(0, halfDepth, z, 255, noiseValue);

				volData->setVoxelAt(x, y, z, relativeValue);

				/*if ((float) z / volData->getDepth() < noiseValue / 255.f)
				    volData->setVoxelAt(x, y, z, 255);
				else
				    volData->setVoxelAt(x, y, z, 0); */
			}
		}
	}
}

struct CellGenerationParams
{
	float newX;
	float newY;
	float newZ;

	// Scale the X, Y, and Z parameters before inputting them into the noise function
	float scale;

	int32 seed;

	bool use3dNoise;

	float meshScale;
};

class cell
{
public:
	float x;
	float y;
	float z;
	PolyVox::SimpleVolume<uint8_t>* volData;
	PolyVox::SurfaceMesh<PolyVox::PositionMaterialNormal> mesh;

	cell()
	{
		LOGV << "Creating volume\n";
		volData = new PolyVox::SimpleVolume<uint8_t>(PolyVox::Region(
		    PolyVox::Vector3DInt32(0, 0, 0), PolyVox::Vector3DInt32(CELL_X, CELL_Y, CELL_Z)));
		LOGV << "done\n";
	}
	~cell()
	{
		mesh.clear();
		delete volData;
	}

	int generate(CellGenerationParams& params)
	{
		if (!volData)
			return -1;

		LOGV << "Generating Noise " << params.newX << " , " << params.newY << " , " << params.newZ
		     << "\n";

		if (params.use3dNoise)
			create3dNoise(volData, params.newX / params.meshScale, params.newY / params.meshScale,
			              params.newZ / params.meshScale, params.scale, params.seed);
		else
			createHeightNoise(volData, params.newX / params.meshScale,
			                  params.newY / params.meshScale, params.newZ / params.meshScale,
			                  params.scale, params.seed);

		LOGV << "done\n";
		LOGV << "Creating surface extrator\n";

		// PolyVox::CubicSurfaceExtractorWithNormals< PolyVox::SimpleVolume<uint8_t> >
		// surfaceExtractor(volData, volData->getEnclosingRegion(), &mesh);
		// PolyVox::CubicSurfaceExtractor< PolyVox::SimpleVolume<uint8_t> >
		// surfaceExtractor(volData, volData->getEnclosingRegion(), &mesh);

		mesh.clear();
		PolyVox::MarchingCubesSurfaceExtractor<PolyVox::SimpleVolume<uint8_t>> surfaceExtractor(
		    volData, volData->getEnclosingRegion(), &mesh);
		LOGV << "done\n";

		LOGV << "Executing surface extrator\n";
		surfaceExtractor.execute();
		LOGV << "done\n";

		return mesh.getIndices().size() / 3;
	}
};

void setSurfaceMeshToRender(
    const PolyVox::SurfaceMesh<PolyVox::PositionMaterialNormal>& surfaceMesh,
    TArray<FGeneratedMeshTriangle>* Triangles, float scale)
{
	if (!Triangles)
		return;

	LOGV << "Surf mesh start\n";

	// Convienient access to the vertices and indices
	const std::vector<uint32_t>& vecIndices = surfaceMesh.getIndices();
	const std::vector<PolyVox::PositionMaterialNormal>& vecVertices = surfaceMesh.getVertices();

	LOGV << "num vertices: " << vecVertices.size() << "\n";
	LOGV << "num indices: " << vecIndices.size() << "\n";

	int iCurrentTriangle = 0;
	int triangleType = 1;

	for (std::vector<uint32_t>::const_iterator it = vecIndices.begin(); it != vecIndices.end();
	     ++it)  // note that inside the loop we will move forward two (three indices = 1 triangle)
	{
		// Get the indices for the current triangle
		const uint32_t index1 = (*it);
		++it;
		const uint32_t index2 = (*it);
		++it;
		const uint32_t index3 = (*it);

		// Look up the vertices for the current triangle
		const PolyVox::PositionMaterialNormal* v1 = &vecVertices[index1];
		const PolyVox::PositionMaterialNormal* v2 = &vecVertices[index2];
		const PolyVox::PositionMaterialNormal* v3 = &vecVertices[index3];

		FGeneratedMeshTriangle* pCurrentTriangle = &Triangles->GetData()[iCurrentTriangle];

		if (!pCurrentTriangle || !v1 || !v2 || !v3)
			break;

		// Reverse winding order (I think unreal uses the opposite of OpenGL)
		pCurrentTriangle->Vertex2.Position.Set(
		    v1->position.getX() * scale, v1->position.getY() * scale, v1->position.getZ() * scale);
		pCurrentTriangle->Vertex1.Position.Set(
		    v2->position.getX() * scale, v2->position.getY() * scale, v2->position.getZ() * scale);
		pCurrentTriangle->Vertex0.Position.Set(
		    v3->position.getX() * scale, v3->position.getY() * scale, v3->position.getZ() * scale);

		// UV Coordinates
		// This (sortof) doesn't work! I think I will have to do texture mapping in the
		// material/shader
		// (triplanar)
		switch (triangleType)
		{
			case 1:
				pCurrentTriangle->Vertex0.U = 0.0f;
				pCurrentTriangle->Vertex0.V = 0.0f;

				pCurrentTriangle->Vertex1.U = 0.0f;
				pCurrentTriangle->Vertex1.V = 1.0f;

				pCurrentTriangle->Vertex2.U = 1.0f;
				pCurrentTriangle->Vertex2.V = 1.0f;
				triangleType = 2;
				break;
			case 2:
				pCurrentTriangle->Vertex0.U = 0.0f;
				pCurrentTriangle->Vertex0.V = 0.0f;

				pCurrentTriangle->Vertex1.U = 1.0f;
				pCurrentTriangle->Vertex1.V = 0.0f;

				pCurrentTriangle->Vertex2.U = 1.0f;
				pCurrentTriangle->Vertex2.V = 1.0f;
				triangleType = 1;
				break;
		}

		iCurrentTriangle++;
	}

	LOGV << "Surf mesh done\n";
}

void ATestPolyVoxChunk::ConstructForPosition(FVector Position, float noiseScale, int32 seed,
                                             float meshScale, bool use3dNoise)
{
	FGeneratedMeshTriangle emptyTriangle;

	LOGV << "Creating ATestPolyVoxChunk\n";
	cell* newCell = new cell;

	if (!newCell)
		return;

	// Generate surface mesh for position
	CellGenerationParams params = {Position.X, Position.Y, Position.Z, noiseScale,
	                               seed,       use3dNoise, meshScale};
	int numVertices = newCell->generate(params);

	// Initialize Unreal triangle buffer
	Triangles.Init(emptyTriangle, numVertices);

	LOGV << "Generation finished\n";

	// Get mesh data from voxel surface mesh and put it in Unreal triangle buffer
	setSurfaceMeshToRender(newCell->mesh, &Triangles, meshScale);

	// Give the triangles to Unreal Engine
	GeneratedMesh->SetGeneratedMeshTriangles(Triangles);

	// Free voxel data and surface mesh (this won't happen in actual game, but we don't need it in
	// this case)
	delete newCell;

	// Set ChunkSize, which can be used to know how large the chunk is in-game (for positioning)
	ChunkSize.Set(CELL_X * meshScale, CELL_Y * meshScale, CELL_Z * meshScale);

	LOGV << "Creating ATestPolyVoxChunk done\n";
}

bool ATestPolyVoxChunk::ShouldTickIfViewportsOnly() const
{
	return true;
}

#if WITH_EDITOR
void ATestPolyVoxChunk::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	PropertiesChanged = true;

	// If the mesh is disappering, then you are forgetting to call this
	Super::PostEditChangeProperty(e);
}
#endif

// Sets default values
ATestPolyVoxChunk::ATestPolyVoxChunk() : LastUpdatedPosition(0.f, 0.f, 0.f)  //, StopTaskCounter(0)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PropertiesChanged = false;

	// Properties defaults
	MeshScale = 100.f;
	NoiseScale = 0.008f;
	NoiseSeed = 5138008;
	Use3dNoise = false;

	// Create components
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	GeneratedMesh = CreateDefaultSubobject<UGeneratedMeshComponent>(TEXT("GeneratedMesh"));

	// Establish component relationships
	RootComponent = SceneComponent;
	if (SceneComponent->CanAttachAsChild(GeneratedMesh, "GeneratedMesh"))
	{
		/*FAttachmentTransformRules attachmentRules(EAttachmentRule::SnapToTarget,
		                                          EAttachmentRule::SnapToTarget,
		                                          EAttachmentRule::SnapToTarget, false);*/
		//GeneratedMesh->AttachToComponent(SceneComponent, attachmentRules, "GeneratedMesh");
		GeneratedMesh->SetupAttachment(SceneComponent, "GeneratedMesh");
	}

	// Set material (this must be run in constructor)
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> material(
	    TEXT("/Game/Materials/SolidGrass.SolidGrass"));
	//"/Game/Materials/TriplanarTest1.TriplanarTest1"
	GeneratedMesh->SetMaterial(0, material.Object);

	// Set collision
	SetActorEnableCollision(true);
}

void ATestPolyVoxChunk::Destroyed()
{
	FlushPersistentDebugLines(GetWorld());
}

// Construct the chunk and geo for its current world position
void ATestPolyVoxChunk::Construct()
{
	FVector worldPosition = SceneComponent->GetComponentLocation();
	float noiseScale = NoiseScale;
	int32 seed = NoiseSeed;
	float meshScale = MeshScale;
	bool use3dNoise = Use3dNoise;

	ConstructForPosition(worldPosition, noiseScale, seed, meshScale, use3dNoise);

	TimeSinceLastUpdate = 0.f;
	LastUpdatedPosition = worldPosition;
}

// Called when the game starts or when spawned
void ATestPolyVoxChunk::BeginPlay()
{
	Super::BeginPlay();

	FVector startPosition = SceneComponent->GetComponentLocation();

	UE_LOG(LogGalavantUnreal, Log, TEXT("Start Pos: %s"), *startPosition.ToString());

	// Construct chunk
	Construct();

	// Make sure that collision is updated. This is necessary for chunks made manually in the
	// editor, but doesn't seem to be necessary for chunks spawned by the chunk manager
	SetActorEnableCollision(true);
	GeneratedMesh->UpdateBodySetup();
	GeneratedMesh->UpdateCollision();
}

// Called every frame
void ATestPolyVoxChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float CHUNK_UPDATE_FREQUENCY = 0.125f;

	FVector worldPosition = SceneComponent->GetComponentLocation();

	TimeSinceLastUpdate += DeltaTime;

	// If we've changed positions in the last n ticks
	if ((worldPosition.Equals(LastUpdatedPosition, 0.1f) == false &&
	     TimeSinceLastUpdate >= CHUNK_UPDATE_FREQUENCY) ||
	    PropertiesChanged)
	{
		// Reset timer and update the position
		TimeSinceLastUpdate = 0.f;
		LastUpdatedPosition = worldPosition;
		PropertiesChanged = false;

		// Update the chunk
		Construct();

		// Debug noise
		FlushPersistentDebugLines(GetWorld());
		drawDebugPointsForPosition(GetWorld(), worldPosition, NoiseScale, NoiseSeed, MeshScale);
	}

	/*	// TESTING ONLY
	    static float timeSinceLastSeedUpdate = 0.f;
	    timeSinceLastSeedUpdate += DeltaTime;
	    if (timeSinceLastSeedUpdate > 0.5f)
	    {
	        timeSinceLastSeedUpdate = 0.f;
	        NoiseSeed += 237;
	        Construct();
	    }*/

	/*// This is just testing if I can move things around in game
	float speed = 100.f;
	FVector deltaLocation(speed * DeltaTime, 0.f, 0.f);
	//SceneComponent.AddWorldOffset(FVector DeltaLocation, bool bSweep, FHitResult*
	OutSweepHitResult, ETeleportType Teleport);
	SceneComponent->AddLocalOffset(deltaLocation, false, NULL, ETeleportType::TeleportPhysics);*/
}

FVector& ATestPolyVoxChunk::GetChunkSize()
{
	return ChunkSize;
}

void ATestPolyVoxChunk::SetUse3dNoise(bool newValue)
{
	Use3dNoise = newValue;
	PropertiesChanged = true;
}