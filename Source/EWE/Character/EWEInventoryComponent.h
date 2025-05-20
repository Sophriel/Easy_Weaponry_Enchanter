// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EWEInventoryComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class EWE_API UEWEInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UEWEInventoryComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	void AcquireWeapon(class AEWEWeaponBase* NewWeapon);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AEWEWeaponBase> StartWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<class AEWEWeaponBase>> Weapons;
};
