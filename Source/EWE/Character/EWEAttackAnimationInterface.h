// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EWEAttackAnimationInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEWEAttackAnimationInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class EWE_API IEWEAttackAnimationInterface
{
	GENERATED_BODY()

public:
	virtual void AttackHitCheck() = 0;
	virtual void AttackReleaseCheck() = 0;
};
