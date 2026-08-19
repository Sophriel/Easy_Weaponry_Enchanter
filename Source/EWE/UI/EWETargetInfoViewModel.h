// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "EWETargetInfoViewModel.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

UCLASS(BlueprintType)
class EWE_API UEWETargetInfoViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    /** Target without ASC — only display the name */
    void SetTarget(const FText &InTargetName);
    /** Target with ASC — subscribe to live HP updates */
    void SetTarget(const FText &InTargetName, UAbilitySystemComponent *InASC);
    void ClearTarget();

    UFUNCTION(BlueprintPure, FieldNotify) bool HasHPInfo() const { return bHasHPInfo; }
    UFUNCTION(BlueprintPure, FieldNotify) FText GetTargetName() const { return TargetName; }
    UFUNCTION(BlueprintPure, FieldNotify) int32 GetCurrentHP() const { return CurrentHP; }
    UFUNCTION(BlueprintPure, FieldNotify) int32 GetMaxHP() const { return MaxHP; }

protected:
    void Unbind();

    void OnHealthChanged(const FOnAttributeChangeData &Data);
    void OnMaxHealthChanged(const FOnAttributeChangeData &Data);

    void SetTargetName(const FText &NewName);
    void SetCurrentHP(int32 NewValue);
    void SetMaxHP(int32 NewValue);

    UPROPERTY(BlueprintReadOnly, FieldNotify) FText TargetName;
    UPROPERTY(BlueprintReadOnly, FieldNotify) int32 CurrentHP = 0;
    UPROPERTY(BlueprintReadOnly, FieldNotify) int32 MaxHP = 0;
    UPROPERTY(BlueprintReadOnly, FieldNotify) bool bHasHPInfo = false;

    UPROPERTY(Transient) TObjectPtr<UAbilitySystemComponent> CachedASC;
    FDelegateHandle HealthHandle;
    FDelegateHandle MaxHealthHandle;
};
