// Fill out your copyright notice in the Description page of Project Settings.


#include "EWEHUD.h"
#include "EWEQuickSlot.h"

AEWEHUD::AEWEHUD()
{
}

void AEWEHUD::AddWeapon(UEWEWeaponData* Weapon)
{
	ensure(QuickSlotWidget);
	QuickSlotWidget->AddWeapon(Weapon);
}

void AEWEHUD::SetSlot(int32 SlotIndex, UEWEWeaponData* Weapon)
{
	ensure(QuickSlotWidget);
	QuickSlotWidget->SetWeaponSlot(SlotIndex, Weapon);
}

void AEWEHUD::SelectSlot(int32 SlotIndex)
{
	ensure(QuickSlotWidget);
	QuickSlotWidget->K2_SelectSlot(SlotIndex);
}

void AEWEHUD::ScrollSlot(float ScrollDirection)
{
	ensure(QuickSlotWidget);
	QuickSlotWidget->K2_ScrollSlot(ScrollDirection);
}
