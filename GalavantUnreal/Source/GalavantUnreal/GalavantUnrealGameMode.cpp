// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GalavantUnreal.h"
#include "GalavantUnrealGameMode.h"
#include "GalavantUnrealPlayerController.h"
#include "GalavantUnrealCharacter.h"

AGalavantUnrealGameMode::AGalavantUnrealGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = AGalavantUnrealPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}