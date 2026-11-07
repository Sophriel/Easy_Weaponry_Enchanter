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

bool UEWEInventoryComponent::IsValidWeaponIndex(int32 Index) const
{
    if (!Items.IsValidIndex(Index))
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("Invalid weapon index: %d"), Index);
        return false;
    }
    return true;
}

UEWEWeaponInstance *UEWEInventoryComponent::GetWeapon(int32 TargetWeapon)
{
    if (!IsValidWeaponIndex(TargetWeapon))
        return nullptr;

    return Items[TargetWeapon];
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

    UEWEWeaponInstance *Instance = NewObject<UEWEWeaponInstance>(this);
    Instance->InitializeWeaponData(NewWeaponData);
    Items.Add(Instance);
    AddReplicatedSubObject(Instance, COND_OwnerOnly);

    // The instance loads its own data and sub-assets. The server starts the
    // pipeline now; clients start it from OnRep_WeaponData.
    Instance->BeginLoad();

    OnWeaponAcquired.Broadcast(Instance);

    EWE_LOG(LogEWEInventory, Log, TEXT("Weapon acquired: %s (Total: %d)"), *NewWeaponData.ToString(), Items.Num());
}

void UEWEInventoryComponent::AcquireWeapon_ServerOnly(UEWEWeaponInstance *Instance)
{
    EWE_LOG(LogEWEInventory, Warning, TEXT("AcquireWeapon(Instance) is not implemented"));
}

void UEWEInventoryComponent::RemoveWeapon_ServerOnly(UEWEWeaponInstance *Instance)
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("RemoveWeapon: no authority"));
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
        ClearSlot_ServerOnly(SlotIndex);
    }

    // Defense: if the equipped instance is still the removed one, the invariant was
    // broken — log it and force the unequip.
    UEWEWeaponInstance *EquippedInstance = OwnerCharacter ? OwnerCharacter->GetEquippedWeaponInstance() : nullptr;
    if (EquippedInstance == Instance)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("RemoveWeapon: equipped instance survived slot clear — invariant broken"));
        ApplyEquipTransition_ServerOnly(nullptr);
    }

    Items.RemoveAt(Index);
    RemoveReplicatedSubObject(Instance);

    OnWeaponRemoved.Broadcast(Instance, Index);

    EWE_LOG(LogEWEInventory, Log, TEXT("Weapon removed (Total: %d)"), Items.Num());
}

void UEWEInventoryComponent::InitializeComponent()
{
    Super::InitializeComponent();

    // Every machine sizes the quick slot array to the developer setting, so the client
    // array is correct even before replication arrives.
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
    SelectedSlotIndex = INDEX_NONE;

    Super::EndPlay(EndPlayReason);
}

void UEWEInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // The Model arrays are owner-only. The component itself replicates to all clients so the
    // sub-object (instance) conditions are evaluated; only these arrays are restricted.
    DOREPLIFETIME_CONDITION(UEWEInventoryComponent, Items, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UEWEInventoryComponent, QuickSlots, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UEWEInventoryComponent, SelectedSlotIndex, COND_OwnerOnly);
}

void UEWEInventoryComponent::OnRep_Items()
{
    SyncInventoryUI();
}

void UEWEInventoryComponent::OnRep_QuickSlots()
{
    OnQuickSlotsChanged.Broadcast();
}

void UEWEInventoryComponent::OnRep_SelectedSlotIndex()
{
    OnSelectedSlotChanged.Broadcast(SelectedSlotIndex);
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

    // Seed the quick slots from the front, without waiting for loads.
    int32 SlotIndex = 0;
    for (UEWEWeaponInstance *Instance : Items)
    {
        if (!Instance || !QuickSlots.IsValidIndex(SlotIndex))
        {
            break;
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

    if (Items.IsEmpty())
        return;

    // Broadcast only. Subscribers decide on seeding, selection and equipping —
    // the inventory has no reason to know a slot index or to equip anything.
    SyncInventoryUI();

    EWE_LOG(LogEWEInventory, Log, TEXT("Initial loadout broadcast: %d weapons"), Items.Num());

    // The possession gate may not be met yet (BeginPlay runs before PossessedBy). The
    // character's PossessedBy calls TryInitialEquip_ServerOnly again once it is.
    TryInitialEquip_ServerOnly();
}

void UEWEInventoryComponent::SyncInventoryUI()
{
    OnWeaponsSynced.Broadcast(Items, Items.Num());

    EWE_LOG(LogEWEInventory, Log, TEXT("Weapons synced: Total %d"), Items.Num());
}

void UEWEInventoryComponent::AssignToSlot_ServerOnly(UEWEWeaponInstance *Instance, int32 SlotIndex)
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("AssignToSlot: no authority"));
        return;
    }

    if (!QuickSlots.IsValidIndex(SlotIndex))
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("AssignToSlot: invalid slot index %d"), SlotIndex);
        return;
    }

    // nullptr is an invalid request, not a clear. The normal UI path never sends nullptr
    // (the widget blocks it before the RPC, and clearing has its own ClearSlot path). A
    // nullptr here means an abnormal path — e.g. the server could not resolve the RPC's
    // NetGUID (a just-removed weapon sent by a client holding a stale copy) or a caller
    // bug. Treating it as a clear would turn an assignment request into a slot clear.
    if (!Instance)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("AssignToSlot: null instance is not a valid request"));
        return;
    }

    // Ownership check: the instance must belong to this inventory. A client could send
    // another player's equipped instance (replicated to all clients) through the RPC.
    if (Items.IndexOfByKey(Instance) == INDEX_NONE)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("AssignToSlot: instance not in inventory"));
        return;
    }

    // Move the instance out of any other slot.
    const int32 PrevIndex = QuickSlots.IndexOfByKey(Instance);
    const bool bMoving = (PrevIndex != INDEX_NONE && PrevIndex != SlotIndex);
    if (bMoving)
    {
        QuickSlots[PrevIndex] = nullptr;
    }

    const bool bSelectedSlotFilled = (SelectedSlotIndex == SlotIndex);
    const bool bSelectedSlotEmptied = (SelectedSlotIndex == PrevIndex);

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

void UEWEInventoryComponent::ClearSlot_ServerOnly(int32 SlotIndex)
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("ClearSlot: no authority"));
        return;
    }

    if (!QuickSlots.IsValidIndex(SlotIndex))
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("ClearSlot: invalid slot index %d"), SlotIndex);
        return;
    }

    if (!QuickSlots[SlotIndex])
    {
        return;   // Already empty — nothing to clear or unequip.
    }

    QuickSlots[SlotIndex] = nullptr;
    OnQuickSlotsChanged.Broadcast();

    if (SelectedSlotIndex == SlotIndex)
    {
        ApplyEquipTransition_ServerOnly(nullptr);
    }
}

void UEWEInventoryComponent::SelectSlot_ServerOnly(int32 SlotIndex)
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("SelectSlot: no authority"));
        return;
    }

    if (!QuickSlots.IsValidIndex(SlotIndex))
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("SelectSlot: invalid slot index %d"), SlotIndex);
        return;
    }

    if (SelectedSlotIndex == SlotIndex)
    {
        return;   // Re-selecting the selected slot is a no-op (no transition).
    }

    SelectedSlotIndex = SlotIndex;
    OnSelectedSlotChanged.Broadcast(SelectedSlotIndex);

    // An empty slot equips nothing (unequips the current weapon).
    ApplyEquipTransition_ServerOnly(QuickSlots[SlotIndex]);
}

void UEWEInventoryComponent::ScrollSlot_ServerOnly(float Direction)
{
    if (!HasOwnerAuthority())
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("ScrollSlot: no authority"));
        return;
    }

    if (Direction == 0.f || QuickSlots.Num() == 0)
    {
        return;
    }

    // Nothing selected yet: start at the first slot instead of wrapping from INDEX_NONE.
    if (!QuickSlots.IsValidIndex(SelectedSlotIndex))
    {
        SelectSlot_ServerOnly(0);
        return;
    }

    const int32 Count = QuickSlots.Num();
    const int32 NextIndex = (Direction > 0.f)
                                ? (SelectedSlotIndex + 1) % Count
                                : (SelectedSlotIndex - 1 + Count) % Count;

    SelectSlot_ServerOnly(NextIndex);
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
    if (SelectedSlotIndex == INDEX_NONE && QuickSlots.IsValidIndex(0))
    {
        SelectSlot_ServerOnly(0);
    }
}

void UEWEInventoryComponent::Server_AssignToSlot_Implementation(UEWEWeaponInstance *Instance, int32 SlotIndex)
{
    AssignToSlot_ServerOnly(Instance, SlotIndex);
}

void UEWEInventoryComponent::Server_ClearSlot_Implementation(int32 SlotIndex)
{
    ClearSlot_ServerOnly(SlotIndex);
}

void UEWEInventoryComponent::Server_SelectSlot_Implementation(int32 SlotIndex)
{
    SelectSlot_ServerOnly(SlotIndex);
}

void UEWEInventoryComponent::Server_ScrollSlot_Implementation(float Direction)
{
    ScrollSlot_ServerOnly(Direction);
}
