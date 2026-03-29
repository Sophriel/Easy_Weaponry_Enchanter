// Fill out your copyright notice in the Description page of Project Settings.


#include "EWEStatus.h"
#include "EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h"
#include "EWE/EWELog.h"

void UEWEStatus::BindAttributeSet(UEWEAttributeBase* AttributeSet)
{
    if (!AttributeSet)
    {
        EWE_LOG(LogEWE, Warning, TEXT("BindAttributeSet: AttributeSet is null"));
        return;
    }

    // Unbind from previous AttributeSet
    UnbindAttributeSet();

    // Cache the AttributeSet
    CachedAttributeSet = AttributeSet;

    // Subscribe to delegates
    HealthChangeDelegateHandle = AttributeSet->OnHealthChangedDelegate.AddUObject(
        this, &UEWEStatus::HandleHealthChange);
    ManaChangeDelegateHandle = AttributeSet->OnManaChangedDelegate.AddUObject(
        this, &UEWEStatus::HandleManaChange);

    // Initial sync
    HandleHealthChange(static_cast<int32>(AttributeSet->GetHealth()),
                       static_cast<int32>(AttributeSet->GetMaxHealth()));
    HandleManaChange(static_cast<int32>(AttributeSet->GetMana()),
                     static_cast<int32>(AttributeSet->GetMaxMana()));

    EWE_LOG(LogEWE, Log, TEXT("UEWEStatus bound to AttributeSet"));
}

void UEWEStatus::UnbindAttributeSet()
{
    if (CachedAttributeSet)
    {
        // Only attempt removal if delegate handles are valid
        if (HealthChangeDelegateHandle.IsValid())
        {
            CachedAttributeSet->OnHealthChangedDelegate.Remove(HealthChangeDelegateHandle);
        }
        if (ManaChangeDelegateHandle.IsValid())
        {
            CachedAttributeSet->OnManaChangedDelegate.Remove(ManaChangeDelegateHandle);
        }

        CachedAttributeSet = nullptr;

        // Reset handles to prevent invalid handle usage
        HealthChangeDelegateHandle.Reset();
        ManaChangeDelegateHandle.Reset();

        EWE_LOG(LogEWE, Log, TEXT("UEWEStatus unbound from AttributeSet"));
    }
}

void UEWEStatus::HandleHealthChange(int32 CurrentHP, int32 MaxHP)
{
    K2_SetHP(CurrentHP, MaxHP);
}

void UEWEStatus::HandleManaChange(int32 CurrentMP, int32 MaxMP)
{
    K2_SetMP(CurrentMP, MaxMP);
}
