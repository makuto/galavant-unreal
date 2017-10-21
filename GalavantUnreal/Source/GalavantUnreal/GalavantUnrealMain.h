// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#pragma once

#include "GameFramework/Actor.h"

#include "GalavantMain.hpp"

#include "game/agent/htnTasks/MovementTasks.hpp"
#include "game/agent/htnTasks/InteractTasks.hpp"
#include "util/CallbackContainer.hpp"

#include "GalaEntityComponents/UnrealMovementComponent.hpp"
#include "CombatFx.hpp"

#include "GalavantUnrealMain.generated.h"

UCLASS()
class GALAVANTUNREAL_API AGalavantUnrealMain : public AGameMode
{
	GENERATED_BODY()

	gv::GalavantMain GalavantMain;

	//
	// World
	//
	gv::WorldStateManager WorldStateManager;

	//
	// Procedural World Params
	//
	// Seed used in all Noise generation
	UPROPERTY(EditAnywhere)
	int Seed;

	// Scale of a single tile (the smallest unit the Procedural World will generate). This assumes
	// the tiles are uniform
	UPROPERTY(EditAnywhere)
	float TileScale;

	UPROPERTY(EditAnywhere)
	float NoiseScale;

	UPROPERTY(EditAnywhere)
	float TestEntityCreationZ;

	UPROPERTY(EditAnywhere)
	float PlayerManhattanViewDistance;

	UnrealCombatFxHandler CombatFxHandler;

	//
	// Agent
	//
	// TODO: How should this work?
	gv::FindResourceTask FindResourceTask;
	gv::MoveToTask MoveToTask;
	gv::GetResourceTask GetResourceTask;
	gv::InteractPickupTask InteractPickupTask;
	gv::CallbackContainer<Htn::TaskEventCallback> TaskEventCallbacks;

	TSubclassOf<AActor> TestFoodActor;
	TSubclassOf<ACharacter> DefaultAgentCharacter;

	void InitializeGalavant();

public:
	// Sets default values for this actor's properties
	AGalavantUnrealMain();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e);
#endif

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void InitGame(const FString& MapName, const FString& Options,
	                      FString& ErrorMessage) override;

private:
	void InitializeEntityTests();
	void InitializeProceduralWorld();

	//
	// Commands
	//
public:
	UFUNCTION(Exec, Category = "Gala")
	void PauseGalaUpdate(bool pause);

	UFUNCTION(Exec, Category = "Gala")
	void HighDPI();
};
