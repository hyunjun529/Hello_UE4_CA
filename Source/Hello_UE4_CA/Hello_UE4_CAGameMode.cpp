// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Hello_UE4_CAGameMode.h"
#include "Hello_UE4_CACharacter.h"
#include "UObject/ConstructorHelpers.h"

AHello_UE4_CAGameMode::AHello_UE4_CAGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
