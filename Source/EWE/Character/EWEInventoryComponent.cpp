// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEInventoryComponent.h"
#include "EasyWeaponryEnchanter/Interface/EWECharacterInterface.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"
#include "Engine/AssetManager.h"

UEWEInventoryComponent::UEWEInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

UEWEWeaponData* UEWEInventoryComponent::GetWeapon(uint32 TargetWeapon)
{
	Weapons.RangeCheck(TargetWeapon);
	return Weapons[TargetWeapon];
}

void UEWEInventoryComponent::AcquireWeapon(UEWEWeaponData* NewWeapon)
{
	check(NewWeapon);
	Weapons.Add(NewWeapon);

	// Add to Quickslot UI
	OwnerCharacter->AcquireWeapon(NewWeapon);
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

	for (const TSoftObjectPtr<UEWEWeaponData>& WeaponData : InitialWeapons)
	{
		if (WeaponData.IsValid() == false)
			continue;

		if (WeaponData.IsPending() == true)
			WeaponData.LoadSynchronous();

		AcquireWeapon(WeaponData.Get());
	}

	if (Weapons.IsEmpty() == true)
		return;

	OwnerCharacter->EquipWeapon(Weapons[0]);
}

void UEWEInventoryComponent::LoadInitialWeaponsAsync()
{
	FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();

	TArray<FSoftObjectPath> WeaponAssets;
	for (const TSoftObjectPtr<UEWEWeaponData>& WeaponData : InitialWeapons)
	{
		if (WeaponData.IsPending() == true)
			WeaponAssets.AddUnique(WeaponData.ToSoftObjectPath());
	}

	LoadHandle = StreamableManager.RequestAsyncLoad(
		WeaponAssets,
		FStreamableDelegate::CreateUObject(this, &UEWEInventoryComponent::OnInitialWeaponsLoaded));
}

void UEWEInventoryComponent::OnInitialWeaponsLoaded()
{
}
