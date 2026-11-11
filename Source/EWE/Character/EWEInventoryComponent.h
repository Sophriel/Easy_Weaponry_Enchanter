// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EWEInventoryComponent.generated.h"

// Forward declarations
class UEWEWeaponData;
class UEWEWeaponInstance;

// Inventory slot Model broadcasts. Native: only C++ (the view model) subscribes.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemSlotChangedEvent, int32 /*SlotIndex*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAcquireRejectedEvent, TSoftObjectPtr<class UEWEWeaponData>);

// Quick slot Model broadcasts. Native: only C++ (the view model) subscribes.
DECLARE_MULTICAST_DELEGATE(FOnQuickSlotsChangedEvent);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSelectedQuickSlotChangedEvent, int32 /*SlotIndex*/);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class EWE_API UEWEInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

#pragma region Lifecycle
public:
    UEWEInventoryComponent();

protected:
    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    /** True when the owning actor has authority (server). */
    bool HasOwnerAuthority() const { return GetOwner() && GetOwner()->HasAuthority(); }
#pragma endregion

#pragma region Inventory
public:
    /**
     * Creates a weapon instance matched to the given data and replicates it to the owner.
     * Server-only: the replicated Items list is mutated here.
     */
    void AcquireWeapon_ServerOnly(TSoftObjectPtr<class UEWEWeaponData> NewWeaponData);

    /**
     * Currently unused. Base work for external acquisition paths (transfer from another
     * owner, drop pickup); implemented with those features.
     */
    void AcquireWeapon_ServerOnly(class UEWEWeaponInstance *Instance);

    /** Removes a weapon instance. Server-only: mutates the replicated Items list. */
    void RemoveWeapon_ServerOnly(class UEWEWeaponInstance *Instance);

    /** The fixed-size inventory slot array (empty slots are nullptr). Read-only. */
    const TArray<TObjectPtr<class UEWEWeaponInstance>> &GetItemSlots() const { return Items; }

    /** Number of inventory slots (developer setting). */
    int32 GetInventorySlotCount() const { return InventorySlotCount; }

    /**
     * One-shot notification to the owning client that an acquisition was rejected because
     * the inventory is full. A Client RPC: a character with no owning client (bots) never
     * receives it. Currently unused — no receiving UI yet. Base work for the
     * inventory-full notification pipeline.
     */
    UFUNCTION(Client, Reliable, Category = "Inventory|Weapon")
    void Client_NotifyAcquireRejected(const TSoftObjectPtr<class UEWEWeaponData> &WeaponData);

public:
    // Model broadcast. The view model subscribes (native, C++ only).
    FOnItemSlotChangedEvent OnItemSlotChanged;

    /** Currently unused. Base work for the inventory-full notification UI, which will subscribe here. */
    FOnAcquireRejectedEvent OnAcquireRejected;

protected:
    /** RepNotify: the owner's item slots changed. Broadcasts per-slot change events. */
    UFUNCTION()
    void OnRep_Items(const TArray<class UEWEWeaponInstance *> &PreviousItems);

protected:
    /**
     * Fixed-size inventory slots (nullptr = empty). Sized to InventorySlotCount in
     * InitializeComponent; never re-ordered — removing a weapon leaves its slot null.
     * Replicated to the owner only.
     */
    UPROPERTY(ReplicatedUsing = OnRep_Items, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Weapon",
              meta = (AllowPrivateAccess = "true"))
    TArray<TObjectPtr<class UEWEWeaponInstance>> Items;

    /**
     * Number of inventory slots. Developer setting, not replicated: every machine sizes its
     * Items array to this in InitializeComponent.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Inventory|Weapon", meta = (AllowPrivateAccess = "true"))
    int32 InventorySlotCount = 32;
#pragma endregion

#pragma region InitialLoadout
protected:
    /** Creates one instance per InitialWeapons entry and seeds the quick slots. Server-only. */
    void CreateInitialWeaponInstances_ServerOnly();

    /**
     * Broadcasts the initial loadout once BeginPlay has run and the initial instances
     * have been created. Safe to call from either path regardless of which finishes first.
     */
    void TryApplyInitialLoadout_ServerOnly();

protected:
    /** Initial loadout. One instance is created per non-empty entry at start. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weapon", meta = (AllowPrivateAccess = "true"))
    TArray<TSoftObjectPtr<class UEWEWeaponData>> InitialWeapons;

    /** True once the initial instances have been created and the quick slots seeded. */
    bool bInitialSeeded = false;

    /** Guards TryApplyInitialLoadout_ServerOnly against running more than once. */
    bool bInitialLoadoutApplied = false;
#pragma endregion

#pragma region QuickSlot
public:
    // Quick slot Model access (read-only). The view model observes these.
    const TArray<TObjectPtr<class UEWEWeaponInstance>> &GetQuickSlots() const { return QuickSlots; }
    int32 GetSelectedQuickSlotIndex() const { return SelectedQuickSlotIndex; }
    int32 GetQuickSlotCount() const { return QuickSlotCount; }

    /**
     * Server single gate for slot content. Assigns the instance to the slot (moving it out of
     * any other slot, D4). Re-equips if the selected slot's content changed.
     */
    void AssignToQuickSlot_ServerOnly(class UEWEWeaponInstance *Instance, int32 SlotIndex);

    /** Server single gate: clears the slot. Re-equips (unequips) if it was the selected slot. */
    void ClearQuickSlot_ServerOnly(int32 SlotIndex);

    /** Server single gate: selects the slot and equips its content (or unequips if empty). */
    void SelectQuickSlot_ServerOnly(int32 SlotIndex);

    /** Server single gate: scrolls the selection. The index is computed here (server-side). */
    void ScrollQuickSlot_ServerOnly(float Direction);

    // Client requests. The owning client sends these to the server, which runs the gates.
    UFUNCTION(Server, Reliable, Category = "Inventory|QuickSlot")
    void Server_AssignToQuickSlot(class UEWEWeaponInstance *Instance, int32 SlotIndex);

    UFUNCTION(Server, Reliable, Category = "Inventory|QuickSlot")
    void Server_ClearQuickSlot(int32 SlotIndex);

    UFUNCTION(Server, Reliable, Category = "Inventory|QuickSlot")
    void Server_SelectQuickSlot(int32 SlotIndex);

    UFUNCTION(Server, Reliable, Category = "Inventory|QuickSlot")
    void Server_ScrollQuickSlot(float Direction);

public:
    // Model broadcasts. The view models subscribe (native, C++ only).
    FOnQuickSlotsChangedEvent OnQuickSlotsChanged;
    FOnSelectedQuickSlotChangedEvent OnSelectedQuickSlotChanged;

protected:
    /** RepNotify: the owner's quick slot contents changed. Broadcasts a full-sync event. */
    UFUNCTION()
    void OnRep_QuickSlots();

    /** RepNotify: the owner's selected slot index changed. Broadcasts the new index. */
    UFUNCTION()
    void OnRep_SelectedQuickSlotIndex();

protected:
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
    UPROPERTY(ReplicatedUsing = OnRep_SelectedQuickSlotIndex, VisibleAnywhere, BlueprintReadOnly,
              Category = "Inventory|QuickSlot", meta = (AllowPrivateAccess = "true"))
    int32 SelectedQuickSlotIndex = INDEX_NONE;
#pragma endregion

#pragma region Equip
public:
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

protected:
    TScriptInterface<class IEWECharacterInterface> OwnerCharacter;

    /** Guards TryInitialEquip_ServerOnly against running more than once. */
    bool bInitialEquipApplied = false;
#pragma endregion
};
