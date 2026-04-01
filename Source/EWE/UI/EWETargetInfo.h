// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EWETargetInfo.generated.h"

class UEWEAttributeBase;

/**
 * Target info HUD widget.
 * Displays the currently targeted character's name and health.
 * Binds to the target's AttributeSet and auto-updates via delegates.
 */
UCLASS()
class EWE_API UEWETargetInfo : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Binds to a target's AttributeSet for automatic health updates */
    void BindTargetAttributeSet(UEWEAttributeBase* AttributeSet, const FText& TargetName);

    /** Unbinds from the current target's AttributeSet and clears display */
    void UnbindTargetAttributeSet();

    /** Returns true if currently bound to a target */
    bool HasTarget() const { return CachedTargetAttributeSet != nullptr; }

protected:
    /** Attribute change handler */
    void HandleTargetHealthChange(int32 CurrentHP, int32 MaxHP);

    /** BlueprintImplementableEvent: Called when target data updates */
    UFUNCTION(BlueprintImplementableEvent, Category = TargetInfo, Meta = (DisplayName = "OnTargetUpdated"))
    void K2_OnTargetUpdated(const FText& TargetName, int32 CurrentHP, int32 MaxHP);

    /** BlueprintImplementableEvent: Called when target is cleared */
    UFUNCTION(BlueprintImplementableEvent, Category = TargetInfo, Meta = (DisplayName = "OnTargetCleared"))
    void K2_OnTargetCleared();

    /** Cached target AttributeSet reference */
    UPROPERTY()
    TObjectPtr<UEWEAttributeBase> CachedTargetAttributeSet;

    /** Delegate handle for unsubscribing */
    FDelegateHandle TargetHealthDelegateHandle;

    /** Cached target display name */
    FText CachedTargetName;
};
