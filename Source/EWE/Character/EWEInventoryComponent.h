// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "EWEInventoryComponent.generated.h"

// Forward declarations
class UEWEWeaponData;

// Delegate Declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponAcquiredEvent, UEWEWeaponData*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponsSyncedEvent, const TArray<UEWEWeaponData*>&, Weapons, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponRemovedEvent,UEWEWeaponData*, Weapon, int32, Index);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class EWE_API UEWEInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEWEInventoryComponent();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	class UEWEWeaponData* GetWeapon(int32 TargetWeapon);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	void AcquireWeapon(class UEWEWeaponData* NewWeapon);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	void RemoveWeapon(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	TArray<class UEWEWeaponData*> GetAllWeapons() const { return Weapons; }

	// Event Delegates (BlueprintAssignable for Blueprint connection)
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnWeaponAcquiredEvent OnWeaponAcquired;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnWeaponsSyncedEvent OnWeaponsSynced;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnWeaponRemovedEvent OnWeaponRemoved;

protected:
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

	void LoadInitialWeaponsAsync();
	void OnInitialWeaponsLoaded();
	void SyncInventoryUI();

protected:
	TScriptInterface<class IEWECharacterInterface> OwnerCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weapon", meta = (AllowPrivateAccess = "true"))
	TArray<TSoftObjectPtr<class UEWEWeaponData>> InitialWeapons;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Weapon", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<class UEWEWeaponData>> Weapons;

	TSharedPtr<FStreamableHandle> LoadHandle;
};
