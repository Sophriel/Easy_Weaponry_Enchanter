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

// Policy for how a dependent attribute's Base is corrected when its Max attribute changes.
// Clamp: the dependent Base is only reduced when it exceeds the new Max; it is not raised when Max rises.
// KeepRatio: the dependent Base keeps its ratio to the old Max (50/100 -> 75/150 -> 50/100);
//            if the old Max is <= 0, the dependent is filled to the new Max.
UENUM(BlueprintType)
enum class EEWEMaxChangePolicy : uint8
{
    Clamp,
    KeepRatio,
};

// A single Max-attribute -> policy binding, editable on a GE's Max Change Policy component.
USTRUCT(BlueprintType)
struct EASYWEAPONRYENCHANTER_API FEWEMaxChangePolicyEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Policy")
    FGameplayAttribute MaxAttribute;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Policy")
    EEWEMaxChangePolicy Policy = EEWEMaxChangePolicy::Clamp;
};

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

    // --- Replication ---
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    // --- Max Change Policy ---
    // Registers (or overwrites) the policy for a Max attribute. Re-applying is idempotent.
    // If MaxAttribute has no dependent attribute, logs a warning and ignores.
    void SetMaxChangePolicy(const FGameplayAttribute &MaxAttribute, EEWEMaxChangePolicy Policy);
    // Returns the policy for a Max attribute, or Clamp when none is registered.
    EEWEMaxChangePolicy GetMaxChangePolicy(const FGameplayAttribute &MaxAttribute) const;
    // Returns the dependent (current) attribute for a Max attribute, or an invalid attribute.
    // MaxHealth -> Health, MaxMana -> Mana. Static so editor validation can reuse it.
    static FGameplayAttribute GetDependentAttribute(const FGameplayAttribute &MaxAttribute);

protected:
    // Clamp the base value before it changes (Base range).
    // Convention: the engine applies Instant GE modifiers to Base in array order, so place
    // Max* modifiers first within a GE. Modifier magnitudes are computed in bulk before the
    // loop, so a % modifier that depends on Max* is evaluated against the previous Max;
    // prefer a fixed amount or a separate GE for those.
    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
    // Clamp the aggregated current value only; base value and modifiers are preserved (non-destructive).
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    // Correct the dependent attribute's Base when its Max attribute changes (authority-only).
    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
    // Side effects after GE executes (death check, etc.). Clamping is handled by the Pre* hooks.
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    // Returns the valid [min, max] range for an attribute, or false when it has no range.
    // The min/max rules live only here, shared by the Base and Current clamp hooks.
    bool GetClampRange(const FGameplayAttribute& Attribute, float& OutMin, float& OutMax) const;
    // Clamps NewValue into the attribute's range (no-op when the attribute has no range).
    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_Mana(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_MaxMana(const FGameplayAttributeData& OldValue);

private:
    // Authority-only, non-replicated. FGameplayAttribute holds a TObjectPtr, so it needs
    // UPROPERTY for GC visibility. Lifetime: the AttributeSet is a character CreateDefaultSubobject,
    // so it survives re-possess; on respawn (new actor) PossessedBy's DefaultGE re-application
    // re-registers the policies.
    UPROPERTY(Transient)
    TArray<FEWEMaxChangePolicyEntry> MaxChangePolicies;
};
