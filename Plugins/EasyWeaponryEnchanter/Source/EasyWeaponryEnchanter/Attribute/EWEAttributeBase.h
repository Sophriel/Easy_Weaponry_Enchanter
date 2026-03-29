// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "EWEAttributeBase.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Delegate: Broadcast when Health attribute changes
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, int32, int32); // CurrentHP, MaxHP

/**
 * Delegate: Broadcast when Mana attribute changes
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnManaChanged, int32, int32); // CurrentMP, MaxMP

/**
 *
 */
UCLASS()
class EASYWEAPONRYENCHANTER_API UEWEAttributeBase : public UAttributeSet
{
    GENERATED_BODY()

public:
    UEWEAttributeBase();

    // --- Vital Attributes ---
    UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UEWEAttributeBase, Health)

    UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UEWEAttributeBase, MaxHealth)

    UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_Mana)
    FGameplayAttributeData Mana;
    ATTRIBUTE_ACCESSORS(UEWEAttributeBase, Mana)

    UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_MaxMana)
    FGameplayAttributeData MaxMana;
    ATTRIBUTE_ACCESSORS(UEWEAttributeBase, MaxMana)

    // --- Delegates for UI Updates ---
    FOnHealthChanged OnHealthChangedDelegate;
    FOnManaChanged OnManaChangedDelegate;

    // --- Replication ---
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    // Clamp before current value changes
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    // Game logic after GE executes (death check, final clamp)
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_Mana(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_MaxMana(const FGameplayAttributeData& OldValue);
};
