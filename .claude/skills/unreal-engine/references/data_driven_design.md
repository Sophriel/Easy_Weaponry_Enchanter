# Data-Driven Design Reference

## Overview

Data-driven gameplay design patterns using DataAsset and DataTable.
Use when separating gameplay parameters (weapons, skills, items, etc.) from code into asset-based management.

---

## UDataAsset vs UPrimaryDataAsset

| Class | Characteristics | When to Use |
|--------|------|----------|
| `UDataAsset` | Simple data container, loaded via hard reference | Small-scale data that can always reside in memory |
| `UPrimaryDataAsset` | AssetManager integration, bundle/chunk management | Large-scale data requiring runtime dynamic loading |

Generally, small projects use `UDataAsset`, while large projects or those with DLC/patches use `UPrimaryDataAsset`.

---

## UDataAsset Basic Pattern

```cpp
UCLASS(BlueprintType)
class MYGAME_API UWeaponDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    FName WeaponName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    float BaseDamage = 10.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    TSubclassOf<UGameplayEffect> DamageEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    int32 MaxEnchantSlots = 3;
};
```

Create an instance via Content Browser → Right-click → Miscellaneous → Data Asset → select `UWeaponDataAsset`.

### DataAsset Reference Patterns

```cpp
// Hard reference — loaded together with the owning actor
UPROPERTY(EditDefaultsOnly, Category = "Weapon")
TObjectPtr<UWeaponDataAsset> WeaponData;

// Soft reference — manually loaded when needed
UPROPERTY(EditDefaultsOnly, Category = "Weapon")
TSoftObjectPtr<UWeaponDataAsset> WeaponDataSoft;

// Load soft reference
if (WeaponDataSoft.IsPending())
{
    UWeaponDataAsset* Loaded = WeaponDataSoft.LoadSynchronous();
}
```

### Nested DataAsset Pattern

A structure where DataAssets reference other DataAssets:

```cpp
UCLASS(BlueprintType)
class UEnchantDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Enchant")
    EEnchantTriggerType TriggerType;

    UPROPERTY(EditDefaultsOnly, Category = "Enchant")
    TSubclassOf<UGameplayEffect> CooldownGE;

    UPROPERTY(EditDefaultsOnly, Category = "Enchant")
    TArray<TSubclassOf<UGameplayAbility>> Functions;
};

// Weapon DataAsset holds an array of Enchant DataAssets
UCLASS(BlueprintType)
class UWeaponDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Enchants")
    TArray<TObjectPtr<UEnchantDataAsset>> EnchantSlots;
};
```

---

## UPrimaryDataAsset + AssetManager

Use when DataAssets need to be dynamically loaded/unloaded in large projects.

```cpp
UCLASS(BlueprintType)
class UItemDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // Used by AssetManager to identify this asset
    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Item", GetFName());
    }

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
    TSoftObjectPtr<UTexture2D> Icon;  // Soft reference for icon

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
    FGameplayTagContainer ItemTags;
};
```

### AssetManager Registration

Register the asset type in `DefaultGame.ini`:

```ini
[/Script/Engine.AssetManagerSettings]
+PrimaryAssetTypesToScan=(PrimaryAssetType="Item",AssetBaseClass="/Script/MyGame.ItemDataAsset",bHasBlueprintClasses=false,Directories=((Path="/Game/Data/Items")))
```

### Runtime Loading

```cpp
// Query asset list
UAssetManager& Manager = UAssetManager::Get();
TArray<FPrimaryAssetId> AssetIds;
Manager.GetPrimaryAssetIdList("Item", AssetIds);

// Async load
FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(
    this, &AMyActor::OnAssetsLoaded);
Manager.LoadPrimaryAssets(AssetIds, {}, Delegate);

// Use assets in callback
void AMyActor::OnAssetsLoaded()
{
    UAssetManager& Manager = UAssetManager::Get();
    for (const FPrimaryAssetId& Id : CachedAssetIds)
    {
        UItemDataAsset* Item = Manager.GetPrimaryAssetObject<UItemDataAsset>(Id);
        if (Item)
        {
            // Use the asset
        }
    }
}
```

### Unload

```cpp
Manager.UnloadPrimaryAssets(AssetIds);
```

### PrimaryDataAsset Reference Considerations

`UPrimaryDataAsset` is designed to be loaded asynchronously via AssetManager, so other classes should reference it with `TSoftObjectPtr` instead of `TObjectPtr` hard references. Hard references force-load the asset when the owning actor loads, negating the async loading benefits of AssetManager.

```cpp
// ❌ Hard reference — defeats the purpose of PrimaryDataAsset async loading
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
TArray<TObjectPtr<UWeaponDataAsset>> Weapons;

// ✅ Soft reference — load when needed
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
TArray<TSoftObjectPtr<UWeaponDataAsset>> Weapons;
```

When accessing elements of a soft reference collection, check load status and explicitly load if not yet loaded:

```cpp
UWeaponDataAsset* GetWeaponData(int32 Index)
{
    if (!Weapons.IsValidIndex(Index))
    {
        return nullptr;
    }

    // Already loaded — return directly
    if (Weapons[Index].IsValid())
    {
        return Weapons[Index].Get();
    }

    // Not loaded — synchronous load and return
    return Weapons[Index].LoadSynchronous();
}
```

> **`UDataAsset` vs `UPrimaryDataAsset` reference comparison**:
> `UDataAsset` is expected to always reside in memory, so `TObjectPtr` hard references are appropriate.
> `UPrimaryDataAsset` is expected to be dynamically loaded via AssetManager, so `TSoftObjectPtr` is appropriate.

---

## DataTable (FTableRowBase)

Use DataTable for structured bulk data (level tables, drop tables, enemy stats, etc.).

### Row Struct Definition

```cpp
USTRUCT(BlueprintType)
struct FEnemyStatsRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float Health = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float MoveSpeed = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float AttackDamage = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags")
    FGameplayTag EnemyTypeTag;
};
```

Create via Content Browser → Right-click → Miscellaneous → DataTable → select `FEnemyStatsRow`.

### Runtime Lookup

```cpp
UPROPERTY(EditDefaultsOnly, Category = "Data")
TSoftObjectPtr<UDataTable> EnemyStatsTable;

void AMyActor::LookupEnemyStats(FName RowName)
{
    if (UDataTable* Table = EnemyStatsTable.LoadSynchronous())
    {
        if (FEnemyStatsRow* Row = Table->FindRow<FEnemyStatsRow>(RowName, TEXT("")))
        {
            float Health = Row->Health;
            float Speed = Row->MoveSpeed;
        }
    }
}
```

### Iterating All Rows

```cpp
TArray<FEnemyStatsRow*> AllRows;
Table->GetAllRows<FEnemyStatsRow>(TEXT(""), AllRows);

for (FEnemyStatsRow* Row : AllRows)
{
    // Process each row
}
```

### CSV/JSON Import

DataTables can be imported from CSV or JSON files, allowing designers to edit data outside the editor and reimport.

In editor: DataTable asset → Right-click → Reimport to apply external file changes.

CSV format:
```
---,Health,MoveSpeed,AttackDamage,EnemyTypeTag
Goblin,50,400,5,"Enemy.Melee"
Dragon,1000,200,100,"Enemy.Boss"
Archer,80,350,15,"Enemy.Ranged"
```

The first column (`---`) is used as the RowName.

---

## DataAsset vs DataTable Selection Criteria

| Criteria | DataAsset | DataTable |
|------|-----------|-----------|
| Data shape | Individual assets (1 weapon = 1 DataAsset) | Table rows (100 enemies = 1 table) |
| Reference method | Direct reference or AssetManager | RowName string lookup |
| Editor UX | Double-click individual assets to edit | Spreadsheet-style bulk editing |
| External import | Not supported (editor only) | CSV/JSON import supported |
| Complex nested structures | Suitable (can reference other DataAssets) | Not suitable (flat row structure) |
| Blueprint reference | Direct asset selection | RowName string input required |
| GAS integration | Direct reference to GA/GE classes | TSubclassOf can be in columns but cumbersome |

**Rule of thumb**: If individual assets have unique structures (nested arrays, references to other assets), use DataAsset. If dozens to hundreds of entries share the same schema, use DataTable.

---

## Editor Validation

Add validation logic to catch input errors in the editor for DataAssets:

```cpp
#if WITH_EDITOR
void UWeaponDataAsset::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);

    if (BaseDamage < 0.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] BaseDamage is negative (%.1f), clamping to 0"),
            *GetName(), BaseDamage);
        BaseDamage = 0.f;
    }

    if (MaxEnchantSlots < 0 || MaxEnchantSlots > 10)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] MaxEnchantSlots out of range (%d), clamping"),
            *GetName(), MaxEnchantSlots);
        MaxEnchantSlots = FMath::Clamp(MaxEnchantSlots, 0, 10);
    }
}
#endif
```
