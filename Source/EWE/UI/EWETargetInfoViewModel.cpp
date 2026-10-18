// Fill out your copyright notice in the Description page of Project Settings.

#include "EWETargetInfoViewModel.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h"
#include "EWE/EWELog.h"
#include "UObject/Object.h"
// #include "EWETargetInfo.h" // ViewModel never includes View class

void UEWETargetInfoViewModel::SetTarget(const FText &InTargetName)
{
    Unbind();
    SetTargetName(InTargetName);
    UE_MVVM_SET_PROPERTY_VALUE(bHasHPInfo, false);
    SetCurrentHP(0);
    SetMaxHP(0);

    EWE_LOG(LogEWE, Log, TEXT("UEWETargetInfoViewModel name-only mode: %s"), *InTargetName.ToString());
}

void UEWETargetInfoViewModel::SetTarget(const FText &InTargetName, UAbilitySystemComponent *InASC)
{
    Unbind();

    SetTargetName(InTargetName);
    UE_MVVM_SET_PROPERTY_VALUE(bHasHPInfo, IsValid(InASC));

    if (!IsValid(InASC))
    {
        return;
    }

    CachedASC = InASC;
    HealthHandle = InASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetHealthAttribute())
                       .AddUObject(this, &UEWETargetInfoViewModel::OnHealthChanged);
    MaxHealthHandle = InASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetMaxHealthAttribute())
                          .AddUObject(this, &UEWETargetInfoViewModel::OnMaxHealthChanged);

    SetCurrentHP(FMath::RoundToInt(InASC->GetNumericAttribute(UEWEAttributeBase::GetHealthAttribute())));
    SetMaxHP(FMath::RoundToInt(InASC->GetNumericAttribute(UEWEAttributeBase::GetMaxHealthAttribute())));

    EWE_LOG(LogEWE, Log, TEXT("UEWETargetInfoViewModel bound to target ASC"));
}

void UEWETargetInfoViewModel::ClearTarget()
{
    Unbind();
    SetTargetName(FText::GetEmpty());
    UE_MVVM_SET_PROPERTY_VALUE(bHasHPInfo, false);
    SetCurrentHP(0);
    SetMaxHP(0);
}

void UEWETargetInfoViewModel::Unbind()
{
    if (IsValid(CachedASC))
    {
        CachedASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetHealthAttribute())
            .Remove(HealthHandle);
        CachedASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetMaxHealthAttribute())
            .Remove(MaxHealthHandle);
    }

    CachedASC = nullptr;
    HealthHandle.Reset();
    MaxHealthHandle.Reset();

    EWE_LOG(LogEWE, Log, TEXT("UEWETargetInfoViewModel unbound"));
}

void UEWETargetInfoViewModel::OnHealthChanged(const FOnAttributeChangeData &Data)
{
    SetCurrentHP(FMath::RoundToInt(Data.NewValue));
}

void UEWETargetInfoViewModel::OnMaxHealthChanged(const FOnAttributeChangeData &Data)
{
    SetMaxHP(FMath::RoundToInt(Data.NewValue));
}

void UEWETargetInfoViewModel::SetTargetName(const FText &NewName) { UE_MVVM_SET_PROPERTY_VALUE(TargetName, NewName); }

void UEWETargetInfoViewModel::SetCurrentHP(int32 NewValue) { UE_MVVM_SET_PROPERTY_VALUE(CurrentHP, NewValue); }

void UEWETargetInfoViewModel::SetMaxHP(int32 NewValue) { UE_MVVM_SET_PROPERTY_VALUE(MaxHP, NewValue); }
