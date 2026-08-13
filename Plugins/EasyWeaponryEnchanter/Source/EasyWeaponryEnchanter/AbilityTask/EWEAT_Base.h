// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "CoreMinimal.h"
#include "EasyWeaponryEnchanter/Function/EnchantContext.h"
#include "EasyWeaponryEnchanter/Interface/EnchantFunction.h"
#include "EWEAT_Base.generated.h"

UCLASS(Abstract)
class EASYWEAPONRYENCHANTER_API UEWEAT_Base : public UAbilityTask, public IEnchantFunction
{
	GENERATED_BODY()

public:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	virtual void ExecuteFunction(const FEnchantContext& InContext) override PURE_VIRTUAL(UEWEAT_Base::ExecuteFunction, );
	virtual void NotifyCompleted() override;
	virtual void InterruptFunction() override;
	virtual bool ShouldExecuteOnServer() const override { return true; }

protected:
	void InitializeEnchantContext(FGameplayAbilityTargetDataHandle InTargetData);
	void RefreshEnchantContextFromAbility();

	const FEnchantContext& GetEnchantContext() const { return EnchantContext; }
	FEnchantContext& GetMutableEnchantContext() { return EnchantContext; }

	UPROPERTY()
	FEnchantContext EnchantContext;

private:
	bool bHasCompleted = false;
};
