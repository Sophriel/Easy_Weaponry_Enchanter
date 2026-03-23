// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EWEHUD.generated.h"

/**
 * HUD class for EWE.
 * Currently a minimal shell - QuickSlot UI is managed by UEWELocalUIManageSubsystem.
 */
UCLASS()
class EWE_API AEWEHUD : public AHUD
{
	GENERATED_BODY()

public:
	AEWEHUD();
};
