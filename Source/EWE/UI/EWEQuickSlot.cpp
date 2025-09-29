// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEQuickSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "EasyWeaponryEnchanter/Public/EasyWeaponryEnchanter.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"

UEWEQuickSlot::UEWEQuickSlot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentSlotIndex = 0;
}

void UEWEQuickSlot::NativeConstruct()
{
	Super::NativeConstruct();

	InitWeaponSlots();
	K2_InitSlot();
}

void UEWEQuickSlot::InitWeaponSlots()
{
	check(SlotReference);
	
	if (MaxSlotSize < 1)
		return;

	Weapons.Reserve(MaxSlotSize);
	WeaponSlots.Reserve(MaxSlotSize);

	for (int32 i = 0; i < MaxSlotSize; ++i)
	{
		TObjectPtr<UUserWidget> SlotWidget = CreateWidget<UUserWidget>(GetWorld(), SlotReference);
		WeaponSlots.Add(SlotWidget);

		// Slot Layout
		TObjectPtr<UHorizontalBoxSlot> SlotHandle = Slots->AddChildToHorizontalBox(SlotWidget);

		FSlateChildSize				   NewSize;
		NewSize.SizeRule = ESlateSizeRule::Fill;
		SlotHandle->SetSize(NewSize);
		SlotHandle->SetHorizontalAlignment(HAlign_Center);
		SlotHandle->SetVerticalAlignment(VAlign_Center);
	}
}

void UEWEQuickSlot::AddWeapon(UEWEWeaponData* Weapon)
{
	check(Weapon);
	for (int32 i = 0; i < MaxSlotSize; ++i)
	{
		if (WeaponSlots.IsValidIndex(i) == false || WeaponSlots[i] == nullptr)
			continue;

		if (Weapons.IsValidIndex(i) == true)
			continue;

		SetWeaponSlot(i, Weapon);
		break;
	}
}

void UEWEQuickSlot::SetWeaponSlot(int32 SlotIndex, UEWEWeaponData* Weapon)
{
	if (Weapon)
	{
		Weapons.Insert(Weapon, SlotIndex);
	}

	else
	{
		Weapons.RemoveAt(SlotIndex);
	}
}