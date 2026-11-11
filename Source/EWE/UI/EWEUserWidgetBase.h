// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EWEUserWidgetBase.generated.h"

class UMVVMViewModelBase;

/**
 * Common base for EWE widgets. Currently provides the MVVM view model binding helper.
 */
UCLASS(Abstract)
class EWE_API UEWEUserWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:
    UEWEUserWidgetBase(const FObjectInitializer& ObjectInitializer);

protected:
    /**
     * Binds a view model to this widget's own MVVM view. Pass the view model name declared
     * in the WBP; when no name can be given (NAME_None), the view model is matched by class.
     * Logs with this widget's class and returns false when the widget has no MVVM view,
     * the view model is null, or no matching settable view model is declared in the WBP.
     */
    bool BindViewModel(UMVVMViewModelBase *ViewModel, FName ViewModelName = NAME_None);
};
