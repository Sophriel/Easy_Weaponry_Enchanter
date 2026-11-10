// Fill out your copyright notice in the Description page of Project Settings.

#include "EWETargetInfo.h"
#include "EWETargetInfoViewModel.h"
#include "View/MVVMView.h"
#include "EWE/EWELog.h"

void UEWETargetInfo::SetViewModel(UEWETargetInfoViewModel *InViewModel)
{
	if (!InViewModel)
	{
		EWE_LOG(LogEWE, Warning, TEXT("SetViewModel: target info view model is null"));
		return;
	}

	UMVVMView *MVVMView = GetExtension<UMVVMView>();
	if (!MVVMView)
	{
		EWE_LOG(LogEWE, Warning, TEXT("Target info widget has no MVVMView extension. Enable ViewModel in WBP Class Settings."));
		return;
	}

	// The name must match the view model declared in the WBP (BP_TargetInfo), or the
	// binding silently does nothing.
	MVVMView->SetViewModel(FName(TEXT("EWETargetInfoViewModel")), InViewModel);
}
