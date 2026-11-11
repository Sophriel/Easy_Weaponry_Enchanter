// Fill out your copyright notice in the Description page of Project Settings.

#include "EWESlotItem.h"
#include "EWE/UI/EWESlotItemViewModel.h"

void UEWESlotItem::SetViewModel(UEWESlotItemViewModel *InViewModel)
{
    BindViewModel(InViewModel, TEXT("EWESlotItemViewModel"));
}
