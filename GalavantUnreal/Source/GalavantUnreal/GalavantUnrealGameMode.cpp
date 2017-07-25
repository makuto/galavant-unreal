// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GalavantUnreal.h"
#include "GalavantUnrealGameMode.h"
#include "Controllers/GalavantUnrealPlayerController.h"
#include "Characters/GalavantUnrealCharacter.h"
#include "Characters/GalavantUnrealFPCharacter.h"

// DEPRECATED; Using AGalavantUnrealMain instead

AGalavantUnrealGameMode::AGalavantUnrealGameMode() : Super()
{
	// use our custom PlayerController class
	// PlayerControllerClass = AGalavantUnrealPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	// static ConstructorHelpers::FClassFinder<APawn>
	// PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
	    TEXT("Pawn'/Game/FirstPersonCPP/Blueprints/"
	         "GalavantUnrealFPCharacterTrueBP.GalavantUnrealFPCharacterTrueBP_C'"));
	// TEXT("/Game/FirstPersonCPP/Blueprints/GalavantUnrealFPCharacterTrueBP"));
	if (PlayerPawnBPClass.Class)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	else
	{
		UE_LOG(LogGalavantUnreal, Log, TEXT("Player Pawn is NULL!"));
		DefaultPawnClass = AGalavantUnrealFPCharacter::StaticClass();
	}
}