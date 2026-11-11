// Fill out your copyright notice in the Description page of Project Settings.

#include "EWETargetInfoViewModel.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h"
#include "EWE/EWELog.h"
#include "UObject/Object.h"

void UEWETargetInfoViewModel::SetTarget(const FText &InTargetName)
{
    Unbind();
    UE_MVVM_SET_PROPERTY_VALUE(bHasTarget, true);
    SetTargetName(InTargetName);
    UE_MVVM_SET_PROPERTY_VALUE(bHasHPInfo, false);
    SetCurrentHP(0);
    SetMaxHP(0);

    EWE_LOG(LogEWE, Log, TEXT("UEWETargetInfoViewModel name-only mode: %s"), *InTargetName.ToString());
}

void UEWETargetInfoViewModel::SetTarget(const FText &InTargetName, UAbilitySystemComponent *InASC)
{
    Unbind();
    UE_MVVM_SET_PROPERTY_VALUE(bHasTarget, true);

    SetTargetName(InTargetName);
    UE_MVVM_SET_PROPERTY_VALUE(bHasHPInfo, IsValid(InASC));

    if (!IsValid(InASC))
    {
        return;
    }

    CachedTargetASC = InASC;
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
    UE_MVVM_SET_PROPERTY_VALUE(bHasTarget, false);
    SetTargetName(FText::GetEmpty());
    UE_MVVM_SET_PROPERTY_VALUE(bHasHPInfo, false);
    SetCurrentHP(0);
    SetMaxHP(0);
}

FText UEWETargetInfoViewModel::GetHPText() const
{
    // No target (or a name-only target) has MaxHP of 0 — show nothing rather than "0/0".
    if (MaxHP <= 0)
    {
        return FText::GetEmpty();
    }

    return FText::Format(FText::AsCultureInvariant(TEXT("{0}/{1}")), FText::AsNumber(CurrentHP),
                         FText::AsNumber(MaxHP));
}

void UEWETargetInfoViewModel::Unbind()
{
    UAbilitySystemComponent *TargetASC = CachedTargetASC.Get();
    if (TargetASC)
    {
        TargetASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetHealthAttribute())
            .Remove(HealthHandle);
        TargetASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetMaxHealthAttribute())
            .Remove(MaxHealthHandle);
    }

    CachedTargetASC = nullptr;
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

void UEWETargetInfoViewModel::SetCurrentHP(int32 NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(CurrentHP, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHPText);
    }
}

void UEWETargetInfoViewModel::SetMaxHP(int32 NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(MaxHP, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHPText);
    }
}
