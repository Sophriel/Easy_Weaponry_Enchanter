// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EasyWeaponryEnchanter/Function/EnchantContext.h"
#include "EnchantFunction.generated.h"

UINTERFACE(BlueprintType)
class EASYWEAPONRYENCHANTER_API UEnchantFunction : public UInterface
{
	GENERATED_BODY()
};

class EASYWEAPONRYENCHANTER_API IEnchantFunction
{
	GENERATED_BODY()

public:
	virtual void ExecuteFunction(const FEnchantContext& InContext) = 0;
	virtual void NotifyCompleted() = 0;
	virtual void InterruptFunction() {}
	virtual bool ShouldExecuteOnServer() const { return true; }
};
