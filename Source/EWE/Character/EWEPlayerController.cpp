// Fill out your copyright notice in the Description page of Project Settings.


#include "EWEPlayerController.h"
#include "EWE/UI/EWEHUD.h"
#include "EWECharacter.h"
#include "AbilitySystemComponent.h"

void AEWEPlayerController::AddWeapon(UEWEWeaponData* Weapon)
{
	AEWEHUD* EWEHUD = GetHUD<AEWEHUD>();
	ensure(EWEHUD);

	EWEHUD->AddWeapon(Weapon);
}

void AEWEPlayerController::SetSlot(const uint8 SlotIndex, UEWEWeaponData* Weapon)
{
	AEWEHUD* EWEHUD = GetHUD<AEWEHUD>();
	ensure(EWEHUD);

	EWEHUD->SetSlot(SlotIndex, Weapon);
}

void AEWEPlayerController::SelectSlot(const uint8 SlotIndex)
{
	 AEWEHUD* EWEHUD = GetHUD<AEWEHUD>();
	 ensure(EWEHUD);

	 EWEHUD->SelectSlot(SlotIndex);
}

void AEWEPlayerController::ScrollSlot(const float ScrollDirection)
{
	AEWEHUD* EWEHUD = GetHUD<AEWEHUD>();
	ensure(EWEHUD);

	EWEHUD->ScrollSlot(ScrollDirection);
}

void AEWEPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);
	//AEWECharacter* CharacterBase = Cast<AEWECharacter>(P);

	//if (CharacterBase)
	//{
	//	CharacterBase->GetAbilitySystemComponent()->InitAbilityActorInfo(CharacterBase, CharacterBase);
	//}
}
