// Fill out your copyright notice in the Description page of Project Settings.

#include "EWETargetInfo.h"
#include "EWETargetInfoViewModel.h"

void UEWETargetInfo::SetViewModel(UEWETargetInfoViewModel *InViewModel)
{
	BindViewModel(InViewModel, TEXT("EWETargetInfoViewModel"));
}
