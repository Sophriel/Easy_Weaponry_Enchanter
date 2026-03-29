// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEAttributeBase.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UEWEAttributeBase::UEWEAttributeBase()
{
    InitHealth(100.f);
    InitMaxHealth(100.f);
    InitMana(50.f);
    InitMaxMana(50.f);
}

void UEWEAttributeBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UEWEAttributeBase, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UEWEAttributeBase, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UEWEAttributeBase, Mana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UEWEAttributeBase, MaxMana, COND_None, REPNOTIFY_Always);
}

void UEWEAttributeBase::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    // Prevent Max values from going below minimum
    if (Attribute == GetMaxHealthAttribute())
        NewValue = FMath::Max(NewValue, 1.f);
    else if (Attribute == GetMaxManaAttribute())
        NewValue = FMath::Max(NewValue, 0.f);
}

void UEWEAttributeBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
        SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
    else if (Data.EvaluatedData.Attribute == GetManaAttribute())
        SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
}

void UEWEAttributeBase::OnRep_Health(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UEWEAttributeBase, Health, OldValue);

    // Broadcast health change event
    // For replicated attributes, GetMax() returns the current value
    OnHealthChangedDelegate.Broadcast(static_cast<int32>(Health.GetCurrentValue()),
                                      static_cast<int32>(MaxHealth.GetCurrentValue()));
}

void UEWEAttributeBase::OnRep_MaxHealth(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UEWEAttributeBase, MaxHealth, OldValue);
}

void UEWEAttributeBase::OnRep_Mana(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UEWEAttributeBase, Mana, OldValue);

    // Broadcast mana change event
    // For replicated attributes, GetMax() returns the current value
    OnManaChangedDelegate.Broadcast(static_cast<int32>(Mana.GetCurrentValue()),
                                    static_cast<int32>(MaxMana.GetCurrentValue()));
}

void UEWEAttributeBase::OnRep_MaxMana(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UEWEAttributeBase, MaxMana, OldValue);
}