// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EWEStatus.generated.h"

// Forward declaration
class UEWEAttributeBase;

/**
 * Status widget that subscribes directly to character attributes.
 * Handles HP/MP display and automatic updates via Delegate subscription.
 */
UCLASS()
class EWE_API UEWEStatus : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Binds to a character's AttributeSet for automatic updates */
    void BindAttributeSet(UEWEAttributeBase* AttributeSet);

    /** Unbinds from the current AttributeSet */
    void UnbindAttributeSet();

protected:
    /** Attribute change handlers */
    void HandleHealthChange(int32 CurrentHP, int32 MaxHP);
    void HandleManaChange(int32 CurrentMP, int32 MaxMP);

    /** BlueprintImplementableEvent: Called when HP changes */
    UFUNCTION(BlueprintImplementableEvent, Category = Status, Meta = (DisplayName = "SetHP"))
    void K2_SetHP(int32 CurrentHP, int32 MaxHP = 100);

    /** BlueprintImplementableEvent: Called when MP changes */
    UFUNCTION(BlueprintImplementableEvent, Category = Status, Meta = (DisplayName = "SetMP"))
    void K2_SetMP(int32 CurrentMP, int32 MaxMP = 100);

    /** Cached AttributeSet reference */
    TObjectPtr<UEWEAttributeBase> CachedAttributeSet;

    /** Delegate handles for unsubscribing */
    FDelegateHandle HealthChangeDelegateHandle;
    FDelegateHandle ManaChangeDelegateHandle;
};
