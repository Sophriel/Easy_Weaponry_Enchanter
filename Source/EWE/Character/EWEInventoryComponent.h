// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "EWEInventoryComponent.generated.h"

// Forward declarations
class UEWEWeaponData;

// Delegate Declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponAcquiredEvent, UEWEWeaponData *, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponsSyncedEvent, const TArray<UEWEWeaponData *> &, Weapons, int32,
                                             NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponRemovedEvent, UEWEWeaponData *, Weapon, int32, Index);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class EWE_API UEWEInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEWEInventoryComponent();

    UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
    class UEWEWeaponData *GetWeapon(int32 TargetWeapon);

    UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
    void AcquireWeapon(class UEWEWeaponData *NewWeapon);

    UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
    void RemoveWeapon(int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
    TArray<class UEWEWeaponData *> GetAllWeapons() const { return Weapons; }

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
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Kicks off a single batched async load for all InitialWeapons entries. */
    void LoadInitialWeaponsAsync();

    /** Streamable completion callback for the batched initial weapon load. */
    void OnInitialWeaponsLoaded();

    /**
     * Broadcasts the initial loadout once BeginPlay has run and the initial load
     * has finished. Safe to call from either path regardless of which finishes first.
     */
    void TryApplyInitialLoadout();

    void SyncInventoryUI();

protected:
    TScriptInterface<class IEWECharacterInterface> OwnerCharacter;

    // Added from Savedata or Editor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weapon", meta = (AllowPrivateAccess = "true"))
    TArray<TSoftObjectPtr<class UEWEWeaponData>> InitialWeapons;

    // Loaded Weapon Data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Weapon", meta = (AllowPrivateAccess = "true"))
    TArray<TObjectPtr<class UEWEWeaponData>> Weapons;

    /** Logs and returns false when the index does not address a weapon. */
    bool IsValidWeaponIndex(int32 Index) const;

    /** Handle for the batched initial weapon load. Released on EndPlay. */
    TSharedPtr<struct FStreamableHandle> InitialWeaponsHandle;

    /** True once the batched initial load has finished (or was a no-op). */
    bool bInitialWeaponsLoaded = false;

    /** True once BeginPlay has run. */
    bool bBeginPlayReached = false;

    /** Guards TryApplyInitialLoadout against running more than once. */
    bool bInitialLoadoutApplied = false;
};
