// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "EWEStatusViewModel.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/**
 * Status ViewModel for HP/MP display.
 * Takes a UAbilitySystemComponent as Source and subscribes to standard GAS change notifications.
 */
UCLASS(BlueprintType)
class EWE_API UEWEStatusViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    /** Connect to an ASC. Passing nullptr automatically unbinds. */
    void SetSource(UAbilitySystemComponent *InASC);

    UFUNCTION(BlueprintPure, FieldNotify) int32 GetCurrentHP() const { return CurrentHP; }
    UFUNCTION(BlueprintPure, FieldNotify) int32 GetMaxHP() const { return MaxHP; }
    UFUNCTION(BlueprintPure, FieldNotify) int32 GetCurrentMP() const { return CurrentMP; }
    UFUNCTION(BlueprintPure, FieldNotify) int32 GetMaxMP() const { return MaxMP; }

    UFUNCTION(BlueprintPure, FieldNotify) FText GetHPText() const;
    UFUNCTION(BlueprintPure, FieldNotify) FText GetMPText() const;

protected:
    void Unbind();

    void OnHealthChanged(const FOnAttributeChangeData &Data);
    void OnMaxHealthChanged(const FOnAttributeChangeData &Data);
    void OnManaChanged(const FOnAttributeChangeData &Data);
    void OnMaxManaChanged(const FOnAttributeChangeData &Data);

    void SetCurrentHP(int32 NewValue);
    void SetMaxHP(int32 NewValue);
    void SetCurrentMP(int32 NewValue);
    void SetMaxMP(int32 NewValue);

    UPROPERTY(Transient)
    TObjectPtr<UAbilitySystemComponent> CachedASC;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 CurrentHP = 0;
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 MaxHP = 0;
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 CurrentMP = 0;
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 MaxMP = 0;

    FDelegateHandle HealthHandle;
    FDelegateHandle MaxHealthHandle;
    FDelegateHandle ManaHandle;
    FDelegateHandle MaxManaHandle;
};
