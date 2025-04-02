// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EWEPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class EWE_API AEWEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SelectSlot(const uint8 SlotIndex);
	void ScrollSlot(const float ScrollDirection);
};
