// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "AgentCharacter.h"

#include "Controllers/AgentAIController.h"

#include "util/Logging.hpp"

#include "entityComponentSystem/EntityComponentManager.hpp"
#include "entityComponentSystem/ComponentTypes.hpp"
#include "game/agent/AgentComponentManager.hpp"

// Sets default values
AAgentCharacter::AAgentCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance
	// if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AAgentAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Configure character movement
	{
		// Character moves in the direction of input...
		GetCharacterMovement()->bOrientRotationToMovement = true;
		// ...at this rotation rate
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		GetCharacterMovement()->JumpZVelocity = 600.f;
		GetCharacterMovement()->AirControl = 0.2f;
	}
}

// Called when the game starts or when spawned
void AAgentCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AAgentCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Entity)
	{
		gv::EntityComponentManager* entityComponentManager =
		    gv::EntityComponentManager::GetSingleton();
		if (entityComponentManager)
		{
			gv::EntityList entitiesToUnsubscribe{Entity};
			entityComponentManager->MarkDestroyEntities(entitiesToUnsubscribe);
		}
	}
}

// Called every frame
void AAgentCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Debugging: Show the state of the agent above head
	if (Entity)
	{
		gv::EntityComponentManager* entityComponentManager =
		    gv::EntityComponentManager::GetSingleton();
		if (entityComponentManager)
		{
			gv::AgentComponentManager* agentComponentManager =
			    static_cast<gv::AgentComponentManager*>(
			        entityComponentManager->GetComponentManagerForType(gv::ComponentType::Agent));

			if (agentComponentManager)
			{
				gv::AgentConsciousStateList consciousStates;
				gv::EntityList entities{Entity};
				agentComponentManager->GetAgentConsciousStates(entities, consciousStates);
				if (!consciousStates.empty() &&
				    (*consciousStates.begin()) != gv::AgentConsciousState::None)
				{
					gv::AgentConsciousState consciousState = (*consciousStates.begin());

					USceneComponent* sceneComponent = GetRootComponent();
					FVector debugTextPosition;
					if (sceneComponent)
						debugTextPosition = sceneComponent->GetComponentLocation();
					debugTextPosition.Z += 100.f;
					// DrawDebugPoint(world, point, POINT_SCALE, FColor(255, 0, 0), true, 0.f);
					switch (consciousState)
					{
						case gv::AgentConsciousState::None:
							DrawDebugString(GetWorld(), debugTextPosition, TEXT("None"), this,
							                FColor(255, 0, 0), 0.f, false);
							break;

						case gv::AgentConsciousState::Conscious:
							DrawDebugString(GetWorld(), debugTextPosition, TEXT("Conscious"), this,
							                FColor(255, 0, 0), 0.f, false);
							break;

						case gv::AgentConsciousState::Unconscious:
							DrawDebugString(GetWorld(), debugTextPosition, TEXT("Unconscious"),
							                this, FColor(255, 0, 0), 0.f, false);
							break;

						case gv::AgentConsciousState::Sleeping:
							DrawDebugString(GetWorld(), debugTextPosition, TEXT("Sleeping"), this,
							                FColor(255, 0, 0), 0.f, false);
							break;

						case gv::AgentConsciousState::Dead:
							DrawDebugString(GetWorld(), debugTextPosition, TEXT("Dead"), this,
							                FColor(255, 0, 0), 0.f, false);
							break;

						default:
							DrawDebugString(GetWorld(), debugTextPosition,
							                TEXT("Unrecognized state"), this, FColor(255, 0, 0),
							                0.f, false);
							break;
					}
				}
			}
		}
	}
}

// Called to bind functionality to input
void AAgentCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	LOGD << "Player input set?";
}
