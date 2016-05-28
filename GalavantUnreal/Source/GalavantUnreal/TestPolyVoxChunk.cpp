// Fill out your copyright notice in the Description page of Project Settings.

#include "GalavantUnreal.h"
#include "TestPolyVoxChunk.h"

#include "noise/noise.hpp"
#include "PolyVoxCore/CubicSurfaceExtractorWithNormals.h"
#include "PolyVoxCore/MarchingCubesSurfaceExtractor.h"
#include "PolyVoxCore/SurfaceMesh.h"
#include "PolyVoxCore/SimpleVolume.h"
#include <vector>
#include <iostream>
#include <stdlib.h>

const int CELL_X = 32;
const int CELL_Y = 32;
const int CELL_Z = 64;  // In Unreal, Z is up axis

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

void createHeightNoise(PolyVox::SimpleVolume<uint8_t>* volData, float xOffset, float yOffset,
					   float zOffset, float scale, int seed)
{
	std::cout << "Using noise\n";
	Noise2d testNoise(seed);
	for (int y = 0; y < volData->getHeight(); y++)
	{
		for (int z = 0; z < volData->getDepth(); z++)
		{
			for (int x = 0; x < volData->getWidth(); x++)
			{
				float noiseValue =
					testNoise.scaledOctaveNoise2d((x + xOffset) * scale, (y + yOffset) * scale, 0,
												  volData->getHeight(), 10, 0.1f, 0.55f, 2);

				if (z < noiseValue)
					volData->setVoxelAt(x, y, z, 255);
				else
					volData->setVoxelAt(x, y, z, 0);

				/*const int GRID_SIZE = 6;
				bool GRID = false;
				if (GRID && x % GRID_SIZE == 0 && y % GRID_SIZE == 0 && z % GRID_SIZE == 0)
					volData->setVoxelAt(x, y, z, 255);*/
			}
		}
	}
}

class cell
{
public:
	float x;
	float y;
	float z;
	PolyVox::SimpleVolume<uint8_t>* volData;
	// mesh probably leaking memory like crazy
	PolyVox::SurfaceMesh<PolyVox::PositionMaterialNormal> mesh;

	cell()
	{
		std::cout << "Creating volume\n";
		volData = new PolyVox::SimpleVolume<uint8_t>(PolyVox::Region(
			PolyVox::Vector3DInt32(0, 0, 0), PolyVox::Vector3DInt32(CELL_X, CELL_Y, CELL_Z)));
		std::cout << "done\n";
	}
	~cell()
	{
		mesh.clear();
		delete volData;
	}

	int generate(float newX, float newY, float newZ, float scale, int seed)
	{
		if (!volData)
			return -1;

		std::cout << "Generating Noise " << newX << " , " << newY << " , " << newZ << "\n";
		const float gridSize = 100.f;  // to make things easier to work with, make it so 100 unreal
									   // units (cm) = 1 noise/grid unit
		x = newX / gridSize;
		y = newY / gridSize;
		z = newZ / gridSize;
		// createSphereInVolume(*volData, 30);
		// createRandomInSphereVolume(*volData, 30);
		createHeightNoise(volData, x, y, z, scale, seed);
		std::cout << "done\n";
		std::cout << "Creating surface extrator\n";
		// PolyVox::CubicSurfaceExtractorWithNormals< PolyVox::SimpleVolume<uint8_t> >
		// surfaceExtractor(volData, volData->getEnclosingRegion(), &mesh);
		// PolyVox::CubicSurfaceExtractor< PolyVox::SimpleVolume<uint8_t> >
		// surfaceExtractor(volData, volData->getEnclosingRegion(), &mesh);
		mesh.clear();
		PolyVox::MarchingCubesSurfaceExtractor<PolyVox::SimpleVolume<uint8_t> > surfaceExtractor(
			volData, volData->getEnclosingRegion(), &mesh);
		std::cout << "done\n";

		std::cout << "Executing surface extrator\n";
		surfaceExtractor.execute();
		std::cout << "done\n";

		return mesh.getIndices().size() / 3;
	}
};

void setSurfaceMeshToRender(
	const PolyVox::SurfaceMesh<PolyVox::PositionMaterialNormal>& surfaceMesh,
	TArray<FGeneratedMeshTriangle>* Triangles, float scale)
{
	if (!Triangles)
		return;

	std::cout << "Surf mesh start\n";

	// Convienient access to the vertices and indices
	const std::vector<uint32_t>& vecIndices = surfaceMesh.getIndices();
	const std::vector<PolyVox::PositionMaterialNormal>& vecVertices = surfaceMesh.getVertices();

	std::cout << "num vertices: " << vecVertices.size() << "\n";
	std::cout << "num indices: " << vecIndices.size() << "\n";

	int iCurrentTriangle = 0;
	int triangleType = 1;

	for (std::vector<uint32_t>::const_iterator it = vecIndices.begin(); it != vecIndices.end();
		 ++it)  // note that the inner loop will move forward two (three indices = 1 triangle)
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
		// This doesn't work! I think I will have to do texture mapping in the material/shader
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

				pCurrentTriangle->Vertex2.U = 1.0f;
				pCurrentTriangle->Vertex2.V = 0.0f;

				pCurrentTriangle->Vertex1.U = 1.0f;
				pCurrentTriangle->Vertex1.V = 1.0f;
				triangleType = 1;
				break;
		}

		iCurrentTriangle++;
	}

	std::cout << "Surf mesh done\n";
}

void ATestPolyVoxChunk::ConstructForPosition(FVector Position, float noiseScale, int seed,
											 float meshScale)
{
	FGeneratedMeshTriangle emptyTriangle;

	std::cout << "Creating ATestPolyVoxChunk\n";
	cell* newCell = new cell;

	if (!newCell)
		return;

	// Generate surface mesh for position
	int numVertices = newCell->generate(Position.X, Position.Y, Position.Z, noiseScale, seed);

	// Initialize Unreal triangle buffer
	Triangles.Init(emptyTriangle, numVertices);

	std::cout << "Generation finished\n";

	// Get mesh data from voxel surface mesh and put it in Unreal triangle buffer
	setSurfaceMeshToRender(newCell->mesh, &Triangles, meshScale);

	// Give the triangles to Unreal Engine
	GeneratedMesh->SetGeneratedMeshTriangles(Triangles);

	// Free voxel data and surface mesh (this won't happen in actual game, but we don't need it in
	// this case)
	delete newCell;

	// Set ChunkSize, which can be used to know how large the chunk is in-game (for positioning)
	ChunkSize.Set(CELL_X * meshScale, CELL_Y * meshScale, CELL_Z * meshScale);

	std::cout << "Creating ATestPolyVoxChunk done\n";
}

bool ATestPolyVoxChunk::ShouldTickIfViewportsOnly() const { return true; }
// Sets default values
ATestPolyVoxChunk::ATestPolyVoxChunk() : LastUpdatedPosition(0.f, 0.f, 0.f)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create components
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	GeneratedMesh = CreateDefaultSubobject<UGeneratedMeshComponent>(TEXT("GeneratedMesh"));

	// Establish component relationships
	RootComponent = SceneComponent;
	if (SceneComponent->CanAttachAsChild(GeneratedMesh, "GeneratedMesh"))
		GeneratedMesh->AttachTo(SceneComponent, "GeneratedMesh",
								EAttachLocation::SnapToTargetIncludingScale, false);

	// Set material (this must be run in constructor)
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
		TEXT("/Game/StarterContent/Materials/M_Ceramic_Tile_Checker.M_Ceramic_Tile_Checker"));
	GeneratedMesh->SetMaterial(0, Material.Object);

	// Set collision
	SetActorEnableCollision(true);
}

// Construct the chunk and geo for its current world position
void ATestPolyVoxChunk::Construct()
{
	FVector worldPosition = SceneComponent->GetComponentLocation();
	int testSeed = 5318008;
	ConstructForPosition(worldPosition, 0.1f, testSeed, 100.f);
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
	if (worldPosition.Equals(LastUpdatedPosition, 0.1f) == false &&
		TimeSinceLastUpdate >= CHUNK_UPDATE_FREQUENCY)
	{
		// Reset timer and update the position
		TimeSinceLastUpdate = 0.f;
		LastUpdatedPosition = worldPosition;

		// Update the chunk
		Construct();
	}

	/*// This is just testing if I can move things around in game
	float speed = 100.f;
	FVector deltaLocation(speed * DeltaTime, 0.f, 0.f);
	//SceneComponent.AddWorldOffset(FVector DeltaLocation, bool bSweep, FHitResult*
	OutSweepHitResult, ETeleportType Teleport);
	SceneComponent->AddLocalOffset(deltaLocation, false, NULL, ETeleportType::TeleportPhysics);*/
}

FVector& ATestPolyVoxChunk::GetChunkSize() { return ChunkSize; }