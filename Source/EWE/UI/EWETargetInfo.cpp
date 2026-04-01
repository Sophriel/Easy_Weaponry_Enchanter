// Fill out your copyright notice in the Description page of Project Settings.

#include "EWETargetInfo.h"
#include "EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h"
#include "EWE/EWELog.h"

void UEWETargetInfo::BindTargetAttributeSet(UEWEAttributeBase* AttributeSet, const FText& TargetName)
{
    if (!AttributeSet)
    {
        EWE_LOG(LogEWE, Warning, TEXT("BindTargetAttributeSet: AttributeSet is null"));
        return;
    }

    // Unbind previous target
    UnbindTargetAttributeSet();

    CachedTargetAttributeSet = AttributeSet;
    CachedTargetName = TargetName;

    // Subscribe to health delegate
    TargetHealthDelegateHandle = AttributeSet->OnHealthChangedDelegate.AddUObject(
        this, &UEWETargetInfo::HandleTargetHealthChange);

    // Initial sync
    HandleTargetHealthChange(
        static_cast<int32>(AttributeSet->GetHealth()),
        static_cast<int32>(AttributeSet->GetMaxHealth()));

    EWE_LOG(LogEWE, Log, TEXT("UEWETargetInfo bound to target: %s"), *TargetName.ToString());
}

void UEWETargetInfo::UnbindTargetAttributeSet()
{
    if (CachedTargetAttributeSet)
    {
        if (TargetHealthDelegateHandle.IsValid())
        {
            CachedTargetAttributeSet->OnHealthChangedDelegate.Remove(TargetHealthDelegateHandle);
        }

        CachedTargetAttributeSet = nullptr;
        TargetHealthDelegateHandle.Reset();
        CachedTargetName = FText::GetEmpty();

        EWE_LOG(LogEWE, Log, TEXT("UEWETargetInfo unbound from target"));
    }

    K2_OnTargetCleared();
}

void UEWETargetInfo::HandleTargetHealthChange(int32 CurrentHP, int32 MaxHP)
{
    K2_OnTargetUpdated(CachedTargetName, CurrentHP, MaxHP);
}
