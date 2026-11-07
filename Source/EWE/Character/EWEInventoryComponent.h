// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EWEInventoryComponent.generated.h"

// Forward declarations
class UEWEWeaponData;
class UEWEWeaponInstance;

// Delegate Declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponAcquiredEvent, UEWEWeaponInstance *, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponsSyncedEvent, const TArray<UEWEWeaponInstance *> &, Weapons, int32,
                                             NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponRemovedEvent, UEWEWeaponInstance *, Weapon, int32, Index);

// Quick slot Model broadcasts. Native: only C++ (the view model) subscribes.
DECLARE_MULTICAST_DELEGATE(FOnQuickSlotsChangedEvent);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSelectedSlotChangedEvent, int32 /*SlotIndex*/);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class EWE_API UEWEInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEWEInventoryComponent();

    UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
    class UEWEWeaponInstance *GetWeapon(int32 TargetWeapon);

    /**
     * Creates a weapon instance matched to the given data and replicates it to the owner.
     * Server-only: the replicated Items list is mutated here.
     */
    void AcquireWeapon_ServerOnly(TSoftObjectPtr<class UEWEWeaponData> NewWeaponData);

    /**
     * Reserved for external acquisition paths (transfer from another owner, drop pickup, etc.).
     * Declared but not implemented yet.
     */
    void AcquireWeapon_ServerOnly(class UEWEWeaponInstance *Instance);

    /** Removes a weapon instance. Server-only: mutates the replicated Items list. */
    void RemoveWeapon_ServerOnly(class UEWEWeaponInstance *Instance);

    UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
    TArray<class UEWEWeaponInstance *> GetAllWeapons() const { return Items; }

    // Quick slot Model access (read-only). The view model observes these.
    const TArray<TObjectPtr<class UEWEWeaponInstance>> &GetQuickSlots() const { return QuickSlots; }
    int32 GetSelectedSlotIndex() const { return SelectedSlotIndex; }
    int32 GetQuickSlotCount() const { return QuickSlotCount; }

    // Model broadcasts. The quick slot view model subscribes (native, C++ only).
    FOnQuickSlotsChangedEvent OnQuickSlotsChanged;
    FOnSelectedSlotChangedEvent OnSelectedSlotChanged;

    /**
     * Server single gate for slot content. Assigns the instance to the slot (moving it out of
     * any other slot, D4). Re-equips if the selected slot's content changed.
     */
    void AssignToSlot_ServerOnly(class UEWEWeaponInstance *Instance, int32 SlotIndex);

    /** Server single gate: clears the slot. Re-equips (unequips) if it was the selected slot. */
    void ClearSlot_ServerOnly(int32 SlotIndex);

    /** Server single gate: selects the slot and equips its content (or unequips if empty). */
    void SelectSlot_ServerOnly(int32 SlotIndex);

    /** Server single gate: scrolls the selection. The index is computed here (server-side). */
    void ScrollSlot_ServerOnly(float Direction);

    /**
     * Server-only equip transition. Every path that changes the equipped instance (select,
     * scroll, selected-slot content change, removal, initial equip) goes through this one
     * function: ① instance replication condition · ② character state · ③ GA grant.
     */
    void ApplyEquipTransition_ServerOnly(class UEWEWeaponInstance *NewInstance);

    /**
     * Initial equip gate (server-only). Condition = initial seed done ∧ possession done.
     * Called from TryApplyInitialLoadout_ServerOnly and from the character's PossessedBy.
     * Selects slot 0 when nothing is selected yet.
     */
    void TryInitialEquip_ServerOnly();

    // Client requests. The owning client sends these to the server, which runs the gates.
    UFUNCTION(Server, Reliable, Category = "Inventory|QuickSlot")
    void Server_AssignToSlot(class UEWEWeaponInstance *Instance, int32 SlotIndex);

    UFUNCTION(Server, Reliable, Category = "Inventory|QuickSlot")
    void Server_ClearSlot(int32 SlotIndex);

    UFUNCTION(Server, Reliable, Category = "Inventory|QuickSlot")
    void Server_SelectSlot(int32 SlotIndex);

    UFUNCTION(Server, Reliable, Category = "Inventory|QuickSlot")
    void Server_ScrollSlot(float Direction);

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
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    /** RepNotify: the owner's item list changed. Broadcasts a single full-sync event. */
    UFUNCTION()
    void OnRep_Items();

    /** Creates one instance per InitialWeapons entry and seeds the quick slots. Server-only. */
    void CreateInitialWeaponInstances_ServerOnly();

    /**
     * Broadcasts the initial loadout once BeginPlay has run and the initial instances
     * have been created. Safe to call from either path regardless of which finishes first.
     */
    void TryApplyInitialLoadout_ServerOnly();

    void SyncInventoryUI();

    /** RepNotify: the owner's quick slot contents changed. Broadcasts a full-sync event. */
    UFUNCTION()
    void OnRep_QuickSlots();

    /** RepNotify: the owner's selected slot index changed. Broadcasts the new index. */
    UFUNCTION()
    void OnRep_SelectedSlotIndex();

protected:
    TScriptInterface<class IEWECharacterInterface> OwnerCharacter;

    /** Initial loadout. One instance is created per non-empty entry at start. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weapon", meta = (AllowPrivateAccess = "true"))
    TArray<TSoftObjectPtr<class UEWEWeaponData>> InitialWeapons;

    /** Weapons acquired at runtime. Replicated to the owner only. */
    UPROPERTY(ReplicatedUsing = OnRep_Items, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Weapon",
              meta = (AllowPrivateAccess = "true"))
    TArray<TObjectPtr<class UEWEWeaponInstance>> Items;

    /**
     * Number of quick slots. Developer setting, not replicated: every machine sizes its
     * QuickSlots array to this in InitializeComponent, so the client array is correct even
     * before replication arrives.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Inventory|QuickSlot", meta = (AllowPrivateAccess = "true"))
    int32 QuickSlotCount = 5;

    /**
     * Quick slot contents (nullptr = empty). Fixed size = QuickSlotCount. Replicated to the
     * owner only.
     */
    UPROPERTY(ReplicatedUsing = OnRep_QuickSlots, VisibleAnywhere, BlueprintReadOnly,
              Category = "Inventory|QuickSlot", meta = (AllowPrivateAccess = "true"))
    TArray<TObjectPtr<class UEWEWeaponInstance>> QuickSlots;

    /** Selected quick slot index. INDEX_NONE until the first selection. Owner-only. */
    UPROPERTY(ReplicatedUsing = OnRep_SelectedSlotIndex, VisibleAnywhere, BlueprintReadOnly,
              Category = "Inventory|QuickSlot", meta = (AllowPrivateAccess = "true"))
    int32 SelectedSlotIndex = INDEX_NONE;

    /** Logs and returns false when the index does not address a weapon. */
    bool IsValidWeaponIndex(int32 Index) const;

    /** True when the owning actor has authority (server). */
    bool HasOwnerAuthority() const { return GetOwner() && GetOwner()->HasAuthority(); }

    /** True once the initial instances have been created and the quick slots seeded. */
    bool bInitialSeeded = false;

    /** Guards TryApplyInitialLoadout_ServerOnly against running more than once. */
    bool bInitialLoadoutApplied = false;

    /** Guards TryInitialEquip_ServerOnly against running more than once. */
    bool bInitialEquipApplied = false;
};
