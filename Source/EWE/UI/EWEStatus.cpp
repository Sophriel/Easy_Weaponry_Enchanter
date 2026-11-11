// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEStatus.h"
#include "EWEStatusViewModel.h"

void UEWEStatus::SetViewModel(UEWEStatusViewModel *InViewModel)
{
    BindViewModel(InViewModel, TEXT("EWEStatusViewModel"));
}
