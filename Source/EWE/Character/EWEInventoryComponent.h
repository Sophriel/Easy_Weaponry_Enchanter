// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "EWEInventoryComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class EWE_API UEWEInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEWEInventoryComponent();

	class UEWEWeaponData* GetWeapon(uint32 TargetWeapon);
	void				  AcquireWeapon(class UEWEWeaponData* NewWeapon); // Acquire Loaded Asset

protected:
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

	void		 LoadInitialWeaponsAsync();
	void		 OnInitialWeaponsLoaded();

protected:
	TScriptInterface<class IEWECharacterInterface> OwnerCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TArray<TSoftObjectPtr<class UEWEWeaponData>> InitialWeapons;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<class UEWEWeaponData>> Weapons;

	TSharedPtr<FStreamableHandle> LoadHandle;
};
