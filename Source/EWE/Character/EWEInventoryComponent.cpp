// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEInventoryComponent.h"
#include "EWE/EWELog.h"
#include "EasyWeaponryEnchanter/Interface/EWECharacterInterface.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponBase.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"

UEWEInventoryComponent::UEWEInventoryComponent()
{
    // No TickComponent override exists, so ticking is disabled.
    PrimaryComponentTick.bCanEverTick = false;
    bWantsInitializeComponent = true;

    // The component replicates its Items list, and the weapon instances are registered as
    // this component's sub-objects. A registered sub-object replicates only when both the
    // owner actor and this component use the registered sub-object list — the actor flag
    // alone is not enough, so both must be set.
    SetIsReplicatedByDefault(true);
    bReplicateUsingRegisteredSubObjectList = true;
}

void UEWEInventoryComponent::AcquireWeapon_ServerOnly(TSoftObjectPtr<UEWEWeaponData> NewWeaponData)
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("AcquireWeapon: no authority"));
        return;
    }

    if (NewWeaponData.IsNull())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("AcquireWeapon: null weapon data"));
        return;
    }

    // The inventory is a fixed-size slot array. Find the first empty slot; a full
    // inventory rejects the acquisition instead of growing.
    int32 SlotIndex = INDEX_NONE;
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (!Items[i])
        {
            SlotIndex = i;
            break;
        }
    }

    if (SlotIndex == INDEX_NONE)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("AcquireWeapon: inventory full, rejecting %s"), *NewWeaponData.ToString());
        Client_NotifyAcquireRejected(NewWeaponData);
        return;
    }

    UEWEWeaponInstance *Instance = NewObject<UEWEWeaponInstance>(this);
    Instance->InitializeWeaponData(NewWeaponData);
    Items[SlotIndex] = Instance;
    AddReplicatedSubObject(Instance, COND_OwnerOnly);

    // The instance loads its own data and sub-assets. The server starts the
    // pipeline now; clients start it from OnRep_WeaponData.
    Instance->BeginLoad();

    OnItemSlotChanged.Broadcast(SlotIndex);

    EWE_LOG(LogEWEInventory, Log, TEXT("Weapon acquired: %s (slot %d)"), *NewWeaponData.ToString(), SlotIndex);
}

void UEWEInventoryComponent::AcquireWeapon_ServerOnly(UEWEWeaponInstance *Instance)
{
    // Currently unused. Base work for external acquisition paths (transfer from another
    // owner, drop pickup); implemented with those features.
}

void UEWEInventoryComponent::RemoveWeapon_ServerOnly(UEWEWeaponInstance *Instance)
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("RemoveWeapon: no authority"));
        return;
    }

    // With null slots in the array, IndexOfByKey(nullptr) would match the first empty
    // slot — reject null before the lookup.
    if (!Instance)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("RemoveWeapon: null instance is not a valid request"));
        return;
    }

    const int32 Index = Items.IndexOfByKey(Instance);
    if (Index == INDEX_NONE)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("RemoveWeapon: instance not in inventory"));
        return;
    }

    // Clear the slot and unequip before the sub-object registration is removed — after
    // that, the instance stops replicating and may be garbage collected. The gate handles
    // the unequip when the cleared slot is the selected one (invariant: the equipped
    // instance is the selected slot's content).
    const int32 SlotIndex = QuickSlots.IndexOfByKey(Instance);
    if (SlotIndex != INDEX_NONE)
    {
        ClearQuickSlot_ServerOnly(SlotIndex);
    }

    // Defense: if the equipped instance is still the removed one, the invariant was
    // broken — log it and force the unequip.
    UEWEWeaponInstance *EquippedInstance = OwnerCharacter ? OwnerCharacter->GetEquippedWeaponInstance() : nullptr;
    if (EquippedInstance == Instance)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("RemoveWeapon: equipped instance survived slot clear — invariant broken"));
        ApplyEquipTransition_ServerOnly(nullptr);
    }

    // Leave the slot null rather than re-ordering: the inventory is a fixed-size slot
    // array, so removing a weapon empties its slot instead of shifting the others.
    Items[Index] = nullptr;
    RemoveReplicatedSubObject(Instance);

    OnItemSlotChanged.Broadcast(Index);

    EWE_LOG(LogEWEInventory, Log, TEXT("Weapon removed (slot %d)"), Index);
}

void UEWEInventoryComponent::InitializeComponent()
{
    Super::InitializeComponent();

    // Every machine sizes the slot arrays to the developer settings, so the client arrays
    // are correct even before replication arrives.
    Items.Init(nullptr, InventorySlotCount);
    QuickSlots.Init(nullptr, QuickSlotCount);

    if (HasOwnerAuthority())
    {
        CreateInitialWeaponInstances_ServerOnly();
    }
}

void UEWEInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter.SetObject(GetOwner());
    OwnerCharacter.SetInterface(Cast<IEWECharacterInterface>(GetOwner()));
    check(OwnerCharacter);

    if (HasOwnerAuthority())
    {
        TryApplyInitialLoadout_ServerOnly();
    }
}

void UEWEInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Items.Empty();
    QuickSlots.Init(nullptr, QuickSlotCount);
    SelectedQuickSlotIndex = INDEX_NONE;

    Super::EndPlay(EndPlayReason);
}

void UEWEInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // The Model arrays are owner-only. The component itself replicates to all clients so the
    // sub-object (instance) conditions are evaluated; only these arrays are restricted.
    DOREPLIFETIME_CONDITION(UEWEInventoryComponent, Items, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UEWEInventoryComponent, QuickSlots, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UEWEInventoryComponent, SelectedQuickSlotIndex, COND_OwnerOnly);
}

void UEWEInventoryComponent::OnRep_Items(const TArray<UEWEWeaponInstance *> &PreviousItems)
{
    // Compare slot by slot. A slot whose instance has not arrived yet reads as null here;
    // the sub-object mapping re-invokes this OnRep once it lands, so the change is caught
    // on that second pass rather than dropped.
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        const UEWEWeaponInstance *Previous = PreviousItems.IsValidIndex(i) ? PreviousItems[i] : nullptr;
        const UEWEWeaponInstance *Current = Items[i].Get();
        if (Previous != Current)
        {
            OnItemSlotChanged.Broadcast(i);
        }
    }
}

void UEWEInventoryComponent::OnRep_QuickSlots()
{
    OnQuickSlotsChanged.Broadcast();
}

void UEWEInventoryComponent::OnRep_SelectedQuickSlotIndex()
{
    OnSelectedQuickSlotChanged.Broadcast(SelectedQuickSlotIndex);
}

void UEWEInventoryComponent::CreateInitialWeaponInstances_ServerOnly()
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("CreateInitialWeaponInstances: no authority"));
        return;
    }

    for (const TSoftObjectPtr<UEWEWeaponData> &WeaponAsset : InitialWeapons)
    {
        if (WeaponAsset.IsNull())
            continue;   // Empty slot in InitialWeapons — not an error.

        AcquireWeapon_ServerOnly(WeaponAsset);
    }

    // Seed the quick slots from the front, without waiting for loads. The inventory is a
    // fixed-size slot array with null gaps, so skip empty slots instead of breaking, and
    // stop once the quick slots are full.
    int32 SlotIndex = 0;
    for (UEWEWeaponInstance *Instance : Items)
    {
        if (!Instance)
        {
            continue;   // Empty inventory slot — skip, keep seeding later slots.
        }
        if (!QuickSlots.IsValidIndex(SlotIndex))
        {
            break;      // Quick slots are full.
        }
        QuickSlots[SlotIndex] = Instance;
        ++SlotIndex;
    }
    OnQuickSlotsChanged.Broadcast();

    bInitialSeeded = true;
}

void UEWEInventoryComponent::TryApplyInitialLoadout_ServerOnly()
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("TryApplyInitialLoadout: no authority"));
        return;
    }

    if (bInitialLoadoutApplied)
        return;

    // Both gates must be satisfied. Either may complete first.
    if (!HasBegunPlay() || !bInitialSeeded)
        return;

    bInitialLoadoutApplied = true;

    // Items is a fixed-size slot array, so IsEmpty() is always false; check for at least
    // one filled slot instead.
    bool bAnyWeapon = false;
    for (const TObjectPtr<UEWEWeaponInstance> &Slot : Items)
    {
        if (Slot)
        {
            bAnyWeapon = true;
            break;
        }
    }
    if (!bAnyWeapon)
        return;

    // The possession gate may not be met yet (BeginPlay runs before PossessedBy). The
    // character's PossessedBy calls TryInitialEquip_ServerOnly again once it is.
    TryInitialEquip_ServerOnly();
}

void UEWEInventoryComponent::AssignToQuickSlot_ServerOnly(UEWEWeaponInstance *Instance, int32 SlotIndex)
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("AssignToQuickSlot: no authority"));
        return;
    }

    if (!QuickSlots.IsValidIndex(SlotIndex))
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("AssignToQuickSlot: invalid slot index %d"), SlotIndex);
        return;
    }

    // nullptr is an invalid request, not a clear. The normal UI path never sends nullptr
    // (the widget blocks it before the RPC, and clearing has its own ClearQuickSlot path). A
    // nullptr here means an abnormal path — e.g. the server could not resolve the RPC's
    // NetGUID (a just-removed weapon sent by a client holding a stale copy) or a caller
    // bug. Treating it as a clear would turn an assignment request into a slot clear.
    if (!Instance)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("AssignToQuickSlot: null instance is not a valid request"));
        return;
    }

    // Ownership check: the instance must belong to this inventory. A client could send
    // another player's equipped instance (replicated to all clients) through the RPC.
    if (Items.IndexOfByKey(Instance) == INDEX_NONE)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("AssignToQuickSlot: instance not in inventory"));
        return;
    }

    // Move the instance out of any other slot.
    const int32 PrevIndex = QuickSlots.IndexOfByKey(Instance);
    const bool bMoving = (PrevIndex != INDEX_NONE && PrevIndex != SlotIndex);
    if (bMoving)
    {
        QuickSlots[PrevIndex] = nullptr;
    }

    const bool bSelectedSlotFilled = (SelectedQuickSlotIndex == SlotIndex);
    const bool bSelectedSlotEmptied = (SelectedQuickSlotIndex == PrevIndex);

    QuickSlots[SlotIndex] = Instance;
    OnQuickSlotsChanged.Broadcast();

    // The selected slot's content changed: re-equip the new content, or unequip if the
    // selected slot was the one emptied by the move.
    if (bSelectedSlotFilled)
    {
        ApplyEquipTransition_ServerOnly(Instance);
    }
    else if (bSelectedSlotEmptied)
    {
        ApplyEquipTransition_ServerOnly(nullptr);
    }
}

void UEWEInventoryComponent::ClearQuickSlot_ServerOnly(int32 SlotIndex)
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("ClearQuickSlot: no authority"));
        return;
    }

    if (!QuickSlots.IsValidIndex(SlotIndex))
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("ClearQuickSlot: invalid slot index %d"), SlotIndex);
        return;
    }

    if (!QuickSlots[SlotIndex])
    {
        return;   // Already empty — nothing to clear or unequip.
    }

    QuickSlots[SlotIndex] = nullptr;
    OnQuickSlotsChanged.Broadcast();

    if (SelectedQuickSlotIndex == SlotIndex)
    {
        ApplyEquipTransition_ServerOnly(nullptr);
    }
}

void UEWEInventoryComponent::SelectQuickSlot_ServerOnly(int32 SlotIndex)
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("SelectQuickSlot: no authority"));
        return;
    }

    if (!QuickSlots.IsValidIndex(SlotIndex))
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("SelectQuickSlot: invalid slot index %d"), SlotIndex);
        return;
    }

    if (SelectedQuickSlotIndex == SlotIndex)
    {
        return;   // Re-selecting the selected slot is a no-op (no transition).
    }

    SelectedQuickSlotIndex = SlotIndex;
    OnSelectedQuickSlotChanged.Broadcast(SelectedQuickSlotIndex);

    // An empty slot equips nothing (unequips the current weapon).
    ApplyEquipTransition_ServerOnly(QuickSlots[SlotIndex]);
}

void UEWEInventoryComponent::ScrollQuickSlot_ServerOnly(float Direction)
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("ScrollQuickSlot: no authority"));
        return;
    }

    if (Direction == 0.f || QuickSlots.Num() == 0)
    {
        return;
    }

    // Nothing selected yet: start at the first slot instead of wrapping from INDEX_NONE.
    if (!QuickSlots.IsValidIndex(SelectedQuickSlotIndex))
    {
        SelectQuickSlot_ServerOnly(0);
        return;
    }

    const int32 Count = QuickSlots.Num();
    const int32 NextIndex = (Direction > 0.f)
                                ? (SelectedQuickSlotIndex + 1) % Count
                                : (SelectedQuickSlotIndex - 1 + Count) % Count;

    SelectQuickSlot_ServerOnly(NextIndex);
}

void UEWEInventoryComponent::ApplyEquipTransition_ServerOnly(UEWEWeaponInstance *NewInstance)
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("ApplyEquipTransition: no authority"));
        return;
    }

    if (!OwnerCharacter)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("ApplyEquipTransition: owner character not set"));
        return;
    }

    UEWEWeaponInstance *PrevInstance = OwnerCharacter->GetEquippedWeaponInstance();
    if (PrevInstance == NewInstance)
    {
        return;
    }

    // ① Instance replication condition. The previous instance's copies on other clients
    // are not destroyed or released; they keep their hard asset references.
    if (PrevInstance)
    {
        RemoveReplicatedSubObject(PrevInstance);
        AddReplicatedSubObject(PrevInstance, COND_OwnerOnly);
    }
    if (NewInstance)
    {
        RemoveReplicatedSubObject(NewInstance);
        AddReplicatedSubObject(NewInstance, COND_None);
    }

    // ② Character state. Sets the equipped instance and refreshes the hand visual.
    OwnerCharacter->SetEquippedWeaponInstance_ServerOnly(NewInstance);

    // ③ GA grant. nullptr removes the current weapon GAs and clears any pending
    // deferred-grant subscription.
    OwnerCharacter->EquipWeapon_ServerOnly(NewInstance);
}

void UEWEInventoryComponent::TryInitialEquip_ServerOnly()
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("TryInitialEquip: no authority"));
        return;
    }

    if (bInitialEquipApplied)
    {
        return;
    }

    // Gate = initial seed done ∧ possession done. Either may complete first.
    // HasBegunPlay guards the OwnerCharacter set in BeginPlay, which the equip transition
    // below depends on.
    if (!HasBegunPlay() || !bInitialSeeded)
    {
        return;
    }

    // GetController() lives on APawn, not AActor — the owner is the character pawn.
    APawn *OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !OwnerPawn->GetController())
    {
        return;
    }

    bInitialEquipApplied = true;

    // Select slot 0 when nothing is selected yet.
    if (SelectedQuickSlotIndex == INDEX_NONE && QuickSlots.IsValidIndex(0))
    {
        SelectQuickSlot_ServerOnly(0);
    }
}

void UEWEInventoryComponent::Server_AssignToQuickSlot_Implementation(UEWEWeaponInstance *Instance, int32 SlotIndex)
{
    AssignToQuickSlot_ServerOnly(Instance, SlotIndex);
}

void UEWEInventoryComponent::Server_ClearQuickSlot_Implementation(int32 SlotIndex)
{
    ClearQuickSlot_ServerOnly(SlotIndex);
}

void UEWEInventoryComponent::Server_SelectQuickSlot_Implementation(int32 SlotIndex)
{
    SelectQuickSlot_ServerOnly(SlotIndex);
}

void UEWEInventoryComponent::Server_ScrollQuickSlot_Implementation(float Direction)
{
    ScrollQuickSlot_ServerOnly(Direction);
}

void UEWEInventoryComponent::Client_NotifyAcquireRejected_Implementation(const TSoftObjectPtr<UEWEWeaponData> &WeaponData)
{
    EWE_LOG(LogEWEInventory, Warning, TEXT("Acquire rejected: inventory full (%s)"), *WeaponData.ToString());
    OnAcquireRejected.Broadcast(WeaponData);
}
