// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EWEWeaponBase.generated.h"

UCLASS()
class EASYWEAPONRYENCHANTER_API AEWEWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AEWEWeaponBase();

	FORCEINLINE class USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon)
	TArray<TSubclassOf<class UGameplayAbility>> Abilities;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USkeletalMeshComponent> WeaponMesh;
};
