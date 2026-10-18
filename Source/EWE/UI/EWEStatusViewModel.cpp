// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEStatusViewModel.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h"
#include "EWE/EWELog.h"
// #include "EWEStatus.h" // ViewModel never includes View class

void UEWEStatusViewModel::SetSource(UAbilitySystemComponent *InASC)
{
    Unbind();
    if (!IsValid(InASC))
    {
        return;
    }

    CachedASC = InASC;

    HealthHandle = InASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetHealthAttribute())
                       .AddUObject(this, &UEWEStatusViewModel::OnHealthChanged);
    MaxHealthHandle = InASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetMaxHealthAttribute())
                          .AddUObject(this, &UEWEStatusViewModel::OnMaxHealthChanged);
    ManaHandle = InASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetManaAttribute())
                     .AddUObject(this, &UEWEStatusViewModel::OnManaChanged);
    MaxManaHandle = InASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetMaxManaAttribute())
                        .AddUObject(this, &UEWEStatusViewModel::OnMaxManaChanged);

    // Initial synchronization — read current attribute values.
    SetCurrentHP(FMath::RoundToInt(InASC->GetNumericAttribute(UEWEAttributeBase::GetHealthAttribute())));
    SetMaxHP(FMath::RoundToInt(InASC->GetNumericAttribute(UEWEAttributeBase::GetMaxHealthAttribute())));
    SetCurrentMP(FMath::RoundToInt(InASC->GetNumericAttribute(UEWEAttributeBase::GetManaAttribute())));
    SetMaxMP(FMath::RoundToInt(InASC->GetNumericAttribute(UEWEAttributeBase::GetMaxManaAttribute())));

    EWE_LOG(LogEWE, Log, TEXT("UEWEStatusViewModel bound to ASC"));
}

FText UEWEStatusViewModel::GetHPText() const
{
    return FText::Format(FText::AsCultureInvariant(TEXT("{0}/{1}")), FText::AsNumber(CurrentHP),
                         FText::AsNumber(MaxHP));
}

FText UEWEStatusViewModel::GetMPText() const
{
    return FText::Format(FText::AsCultureInvariant(TEXT("{0}/{1}")), FText::AsNumber(CurrentMP),
                         FText::AsNumber(MaxMP));
}

void UEWEStatusViewModel::Unbind()
{
    if (IsValid(CachedASC))
    {
        CachedASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetHealthAttribute())
            .Remove(HealthHandle);
        CachedASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetMaxHealthAttribute())
            .Remove(MaxHealthHandle);
        CachedASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetManaAttribute()).Remove(ManaHandle);
        CachedASC->GetGameplayAttributeValueChangeDelegate(UEWEAttributeBase::GetMaxManaAttribute())
            .Remove(MaxManaHandle);
    }

    CachedASC = nullptr;
    HealthHandle.Reset();
    MaxHealthHandle.Reset();
    ManaHandle.Reset();
    MaxManaHandle.Reset();

    EWE_LOG(LogEWE, Log, TEXT("UEWEStatusViewModel unbound"));
}

void UEWEStatusViewModel::OnHealthChanged(const FOnAttributeChangeData &Data)
{
    SetCurrentHP(FMath::RoundToInt(Data.NewValue));
}

void UEWEStatusViewModel::OnMaxHealthChanged(const FOnAttributeChangeData &Data)
{
    SetMaxHP(FMath::RoundToInt(Data.NewValue));
}

void UEWEStatusViewModel::OnManaChanged(const FOnAttributeChangeData &Data)
{
    SetCurrentMP(FMath::RoundToInt(Data.NewValue));
}

void UEWEStatusViewModel::OnMaxManaChanged(const FOnAttributeChangeData &Data)
{
    SetMaxMP(FMath::RoundToInt(Data.NewValue));
}

void UEWEStatusViewModel::SetCurrentHP(int32 NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(CurrentHP, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHPText);
    }
}

void UEWEStatusViewModel::SetMaxHP(int32 NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(MaxHP, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHPText);
    }
}

void UEWEStatusViewModel::SetCurrentMP(int32 NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(CurrentMP, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetMPText);
    }
}

void UEWEStatusViewModel::SetMaxMP(int32 NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(MaxMP, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetMPText);
    }
}
