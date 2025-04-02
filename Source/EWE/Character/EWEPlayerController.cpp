// Fill out your copyright notice in the Description page of Project Settings.


#include "EWEPlayerController.h"
#include "EWE/UI/EWEHUD.h"

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
