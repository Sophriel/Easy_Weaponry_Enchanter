#pragma once

#include "CoreMinimal.h"
#include "EasyWeaponryEnchanter/Function/EnchantContext.h"

class UGameplayAbility;
class UEWEAT_Base;

class EASYWEAPONRYENCHANTER_API FEWEFunctionTaskFactory
{
public:
	static UEWEAT_Base* CreateTask(
		UGameplayAbility* OwningAbility,
		const FEnchantFunction& FunctionData,
		const FEnchantContext& EnchantContext);

private:
	static FGameplayAbilityTargetDataHandle BuildCreationTargetData(
		UGameplayAbility* OwningAbility,
		const FEnchantFunction& FunctionData,
		const FEnchantContext& EnchantContext);
};
