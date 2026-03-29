// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEInventoryComponent.h"
#include "EWE/EWELog.h"
#include "EWE/Character/EWEPlayerController.h"
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

    APawn *OwnerPawn = Cast<APawn>(GetOwner());
    AEWEPlayerController *EWEPC = Cast<AEWEPlayerController>(OwnerPawn->GetController());
    check(EWEPC);

    uint8 WeaponIndex = 0;
    for (const TSoftObjectPtr<UEWEWeaponData> &WeaponData : InitialWeapons)
    {
        if (WeaponData.IsValid() == false)
            continue;

        if (WeaponData.IsPending() == true)
            WeaponData.LoadSynchronous();

        UEWEWeaponData *LoadedWeapon = WeaponData.Get();
        AcquireWeapon(LoadedWeapon);

        // @@@@@ TEMP: Init QuickSlot Data with Initial Weapons
        EWEPC->SetSlot(WeaponIndex, LoadedWeapon);

        ++WeaponIndex;
    }

    if (Weapons.IsEmpty() == true)
        return;

    // Sync all weapons with inventory UI
    SyncInventoryUI();

    OwnerCharacter->EquipWeapon(Weapons[0]);
    EWEPC->SelectSlot(0);
}

void UEWEInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (TObjectPtr<UEWEWeaponData> &Weapon : Weapons)
    {
        Weapon = nullptr;
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
    for (TSoftObjectPtr<UEWEWeaponData> &WeaponAsset : InitialWeapons)
    {
        LoadWeaponAsync(WeaponAsset);
    }
}

void UEWEInventoryComponent::LoadWeaponAsync(TSoftObjectPtr<UEWEWeaponData> WeaponAsset)
{
    if (!WeaponAsset.IsPending())
    {
        if (WeaponAsset.IsValid())
        {
            WeaponAsset->LoadWeaponAssets();
        }
        return;
    }

    FStreamableManager &StreamableManager = UAssetManager::Get().GetStreamableManager();
    StreamableManager.RequestAsyncLoad(
        WeaponAsset.ToSoftObjectPath(),
        FStreamableDelegate::CreateUObject(this, &UEWEInventoryComponent::OnWeaponLoaded, WeaponAsset));
}

void UEWEInventoryComponent::OnWeaponLoaded(TSoftObjectPtr<UEWEWeaponData> WeaponAsset)
{
    if (!WeaponAsset.IsValid())
    {
        EWE_LOG(LogEWEInventory, Error, TEXT("Asset %s AsyncLoad Failed"), *WeaponAsset.GetAssetName());
        return;
    }

    WeaponAsset->LoadWeaponAssets();
    Weapons.Add(WeaponAsset.Get());
}