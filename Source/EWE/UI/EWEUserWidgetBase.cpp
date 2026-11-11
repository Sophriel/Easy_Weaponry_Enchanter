// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEUserWidgetBase.h"
#include "MVVMViewModelBase.h"
#include "View/MVVMView.h"
#include "EWE/EWELog.h"

UEWEUserWidgetBase::UEWEUserWidgetBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

bool UEWEUserWidgetBase::BindViewModel(UMVVMViewModelBase *ViewModel, FName ViewModelName)
{
    if (!ViewModel)
    {
        EWE_LOG(LogEWE, Warning, TEXT("%s: view model is null"), *GetClass()->GetName());
        return false;
    }

    UMVVMView *MVVMView = GetExtension<UMVVMView>();
    if (!MVVMView)
    {
        EWE_LOG(LogEWE, Warning, TEXT("%s: no MVVMView extension. Enable ViewModel in WBP Class Settings."), *GetClass()->GetName());
        return false;
    }

    const bool bBound = ViewModelName != NAME_None
        ? MVVMView->SetViewModel(ViewModelName, ViewModel)
        : MVVMView->SetViewModelByClass(ViewModel);

    if (!bBound)
    {
        const FString WidgetName = GetClass()->GetName();
        if (ViewModelName != NAME_None)
        {
            EWE_LOG(LogEWE, Warning, TEXT("%s: no settable view model named %s is declared in the WBP."), *WidgetName, *ViewModelName.ToString());
        }
        else
        {
            EWE_LOG(LogEWE, Warning, TEXT("%s: no settable view model of class %s is declared in the WBP."), *WidgetName, *ViewModel->GetClass()->GetName());
        }
        return false;
    }

    return true;
}
