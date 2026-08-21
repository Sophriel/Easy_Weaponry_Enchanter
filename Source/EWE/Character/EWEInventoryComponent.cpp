// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEInventoryComponent.h"
#include "EWE/EWELog.h"
#include "EasyWeaponryEnchanter/Interface/EWECharacterInterface.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"
#include "Engine/AssetManager.h"

UEWEInventoryComponent::UEWEInventoryComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.
    // You can turn these features off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;
    bWantsInitializeComponent = true;
}

UEWEWeaponData *UEWEInventoryComponent::GetWeapon(int32 TargetWeapon)
{
    Weapons.RangeCheck(TargetWeapon);
    return Weapons[TargetWeapon];
}

void UEWEInventoryComponent::AcquireWeapon(UEWEWeaponData *NewWeapon)
{
    check(NewWeapon);

    // Check if weapon already exists (prevent duplicates)
    for (const auto &ExistingWeapon : Weapons)
    {
        if (ExistingWeapon == NewWeapon)
        {
            EWE_LOG(LogEWEInventory, Warning, TEXT("Weapon already exists: %s"), *NewWeapon->GetName());
            return;
        }
    }

    Weapons.Add(NewWeapon);

    // Broadcast event - subscribers will react
    OnWeaponAcquired.Broadcast(NewWeapon);

    EWE_LOG(LogEWEInventory, Log, TEXT("Weapon acquired: %s (Total: %d)"), *NewWeapon->GetName(), Weapons.Num());
}

void UEWEInventoryComponent::RemoveWeapon(int32 Index)
{
    if (!Weapons.IsValidIndex(Index))
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("Invalid weapon index: %d"), Index);
        return;
    }

    UEWEWeaponData *RemovedWeapon = Weapons[Index];
    Weapons.RemoveAt(Index);

    // Broadcast event
    OnWeaponRemoved.Broadcast(RemovedWeapon, Index);

    EWE_LOG(LogEWEInventory, Log, TEXT("Weapon removed: %s (Total: %d)"), *RemovedWeapon->GetName(), Weapons.Num());
}

void UEWEInventoryComponent::InitializeComponent()
{
    Super::InitializeComponent();

    LoadInitialWeaponsAsync();
}

void UEWEInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter.SetObject(GetOwner());
    OwnerCharacter.SetInterface(Cast<IEWECharacterInterface>(GetOwner()));
    check(OwnerCharacter);

    bBeginPlayReached = true;

    TryApplyInitialLoadout();
}

void UEWEInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (InitialWeaponsHandle.IsValid())
    {
        InitialWeaponsHandle->ReleaseHandle();
        InitialWeaponsHandle.Reset();
    }

    Weapons.Empty();

    Super::EndPlay(EndPlayReason);
}

void UEWEInventoryComponent::SyncInventoryUI()
{
    // Broadcast event to sync all weapons
    OnWeaponsSynced.Broadcast(Weapons, Weapons.Num());

    EWE_LOG(LogEWEInventory, Log, TEXT("Weapons synced: Total %d"), Weapons.Num());
}

void UEWEInventoryComponent::LoadInitialWeaponsAsync()
{
    TArray<FSoftObjectPath> WeaponPaths;
    WeaponPaths.Reserve(InitialWeapons.Num());

    for (const TSoftObjectPtr<UEWEWeaponData> &WeaponAsset : InitialWeapons)
    {
        if (WeaponAsset.IsNull())
            continue;

        WeaponPaths.Add(WeaponAsset.ToSoftObjectPath());
    }

    if (WeaponPaths.IsEmpty())
    {
        // Nothing to stream. Treat as immediately complete.
        OnInitialWeaponsLoaded();
        return;
    }

    FStreamableManager &StreamableManager = UAssetManager::Get().GetStreamableManager();
    InitialWeaponsHandle = StreamableManager.RequestAsyncLoad(
        WeaponPaths, FStreamableDelegate::CreateUObject(this, &UEWEInventoryComponent::OnInitialWeaponsLoaded));
}

void UEWEInventoryComponent::OnInitialWeaponsLoaded()
{
    for (const TSoftObjectPtr<UEWEWeaponData> &WeaponAsset : InitialWeapons)
    {
        if (!WeaponAsset.IsValid())
        {
            EWE_LOG(LogEWEInventory, Error, TEXT("Initial weapon failed to load: %s"), *WeaponAsset.ToString());
            continue;
        }

        UEWEWeaponData *LoadedWeapon = WeaponAsset.Get();
        LoadedWeapon->LoadWeaponAssets();

        // Single acquisition path. AcquireWeapon handles dedup and broadcasts OnWeaponAcquired.
        AcquireWeapon(LoadedWeapon);
    }

    bInitialWeaponsLoaded = true;
    TryApplyInitialLoadout();
}

void UEWEInventoryComponent::TryApplyInitialLoadout()
{
    if (bInitialLoadoutApplied)
        return;

    // Both gates must be satisfied. Either may complete first.
    if (!bBeginPlayReached || !bInitialWeaponsLoaded)
        return;

    bInitialLoadoutApplied = true;

    if (Weapons.IsEmpty())
        return;

    // Broadcast only. Subscribers decide on seeding, selection and equipping —
    // the inventory has no reason to know a slot index or to equip anything.
    SyncInventoryUI();

    EWE_LOG(LogEWEInventory, Log, TEXT("Initial loadout broadcast: %d weapons"), Weapons.Num());
}