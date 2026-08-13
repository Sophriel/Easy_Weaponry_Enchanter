// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "CoreMinimal.h"
#include "EnchantContext.generated.h"

class AEWEWeaponBase;

UENUM(BlueprintType)
enum class EEnchantFunctionType : uint8
{
	Creation UMETA(DisplayName = "Creation"),
	Ejection UMETA(DisplayName = "Ejection"),
	Rotation UMETA(DisplayName = "Rotation"),
	Attach UMETA(DisplayName = "Attach"),
	TimeStop UMETA(DisplayName = "Time Stop"),
	Freeze UMETA(DisplayName = "Freeze"),
	Teleport UMETA(DisplayName = "Teleport"),
	PossessObject UMETA(DisplayName = "Possess Object"),
	WeatherChange UMETA(DisplayName = "Weather Change"),
	StatusEffect UMETA(DisplayName = "Status Effect")
};

UENUM(BlueprintType)
enum class ELocationType : uint8
{
	Character UMETA(DisplayName = "Character Location"),
	View UMETA(DisplayName = "Character View (Forward)"),
	Camera UMETA(DisplayName = "Camera Location"),
	Target UMETA(DisplayName = "Target Actor/Point"),
	AnchorObject UMETA(DisplayName = "Placed Anchor Object")
};

USTRUCT(BlueprintType)
struct EASYWEAPONRYENCHANTER_API FEnchantFunctionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnchantFunction")
	EEnchantFunctionType FunctionType = EEnchantFunctionType::Creation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnchantFunction|Creation",
		meta = (EditCondition = "FunctionType == EEnchantFunctionType::Creation", EditConditionHides))
	TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnchantFunction|Creation",
		meta = (EditCondition = "FunctionType == EEnchantFunctionType::Creation", EditConditionHides))
	ELocationType LocationType = ELocationType::Character;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnchantFunction|Creation",
		meta = (EditCondition = "FunctionType == EEnchantFunctionType::Creation", EditConditionHides))
	FTransform TargetLocation = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnchantFunction|Creation",
		meta = (EditCondition = "FunctionType == EEnchantFunctionType::Creation && LocationType == ELocationType::Camera",
			EditConditionHides))
	float TargetDistance = 0.f;
};

using FEnchantFunction = FEnchantFunctionData;

USTRUCT(BlueprintType)
struct EASYWEAPONRYENCHANTER_API FEnchantContext
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TWeakObjectPtr<AActor> AvatarActor;

	UPROPERTY()
	FGameplayAbilityTargetDataHandle TargetData;

	UPROPERTY()
	TWeakObjectPtr<AEWEWeaponBase> ParentWeapon;
};
