// Copyright Epic Games, Inc. All Rights Reserved.

#include "EWEGameMode.h"
#include "Character/EWECharacter.h"
#include "UObject/ConstructorHelpers.h"

AEWEGameMode::AEWEGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Characters/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
