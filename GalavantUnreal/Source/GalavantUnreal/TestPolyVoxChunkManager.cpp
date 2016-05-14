// Fill out your copyright notice in the Description page of Project Settings.

#include "GalavantUnreal.h"
#include "TestPolyVoxChunkManager.h"

#include "TestPolyVoxChunk.h"


// Sets default values
ATestPolyVoxChunkManager::ATestPolyVoxChunkManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

bool ATestPolyVoxChunkManager::ShouldTickIfViewportsOnly() const
{
	return true;
}

// Called when the game starts or when spawned
void ATestPolyVoxChunkManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Test creating a chunk
	ATestPolyVoxChunk *newChunk = (ATestPolyVoxChunk *) GetWorld()->SpawnActor(ATestPolyVoxChunk::StaticClass(), NAME_None, NULL, NULL, NULL, false, this, NULL, false, NULL, false);

	if (newChunk)
	{
		UE_LOG(LogTemp, Log, TEXT("Spawned actor named '%s'"), *newChunk->GetHumanReadableName());
	}

}

// Called every frame
void ATestPolyVoxChunkManager::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

