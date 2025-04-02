// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEQuickSlot.h"
#include "Kismet/GameplayStatics.h"

UEWEQuickSlot::UEWEQuickSlot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentSlotIndex = 0;
}

void UEWEQuickSlot::NativeConstruct()
{
	Super::NativeConstruct();
	K2_SelectSlot(0);
}
