// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EWESlotItem.generated.h"

/**
 * Slot widget — draws one cell of the quick slot bar or inventory grid.
 * When the parent widget passes a slot item view model, it binds its own view to it.
 * The Blueprint-side view model name stays inside this class.
 */
UCLASS()
class EWE_API UEWESlotItem : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Injects the slot item view model and binds it to this widget's own view. */
    void SetViewModel(class UEWESlotItemViewModel *InViewModel);
};
