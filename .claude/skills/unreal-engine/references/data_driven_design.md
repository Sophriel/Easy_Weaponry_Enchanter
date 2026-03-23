# Data-Driven Design Reference

## Overview

DataAsset과 DataTable을 활용한 데이터 주도 게임플레이 설계 패턴.
무기, 스킬, 아이템 등 게임플레이 파라미터를 코드와 분리하여 에셋으로 관리할 때 사용한다.

---

## UDataAsset vs UPrimaryDataAsset

| 클래스 | 특징 | 사용 시점 |
|--------|------|----------|
| `UDataAsset` | 단순 데이터 컨테이너, 하드 레퍼런스로 로드 | 항상 메모리에 있어도 되는 소규모 데이터 |
| `UPrimaryDataAsset` | AssetManager 통합, 번들/청크 관리 가능 | 런타임 동적 로딩이 필요한 대규모 데이터 |

일반적으로 소규모 프로젝트는 `UDataAsset`, 대규모 프로젝트나 DLC/패치가 있는 프로젝트는 `UPrimaryDataAsset`을 사용한다.

---

## UDataAsset 기본 패턴

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

Content Browser에서 우클릭 → Miscellaneous → Data Asset → `UWeaponDataAsset` 선택으로 인스턴스 생성.

### DataAsset 참조 방식

```cpp
// 하드 레퍼런스 — 소유 액터 로드 시 함께 로드됨
UPROPERTY(EditDefaultsOnly, Category = "Weapon")
TObjectPtr<UWeaponDataAsset> WeaponData;

// 소프트 레퍼런스 — 필요 시점에 수동 로드
UPROPERTY(EditDefaultsOnly, Category = "Weapon")
TSoftObjectPtr<UWeaponDataAsset> WeaponDataSoft;

// 소프트 레퍼런스 로드
if (WeaponDataSoft.IsPending())
{
    UWeaponDataAsset* Loaded = WeaponDataSoft.LoadSynchronous();
}
```

### 중첩 DataAsset 패턴

DataAsset이 다른 DataAsset을 참조하는 구조:

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

// 무기 DataAsset이 인챈트 DataAsset 배열을 보유
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

대규모 프로젝트에서 DataAsset을 동적으로 로드/언로드해야 할 때 사용.

```cpp
UCLASS(BlueprintType)
class UItemDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // AssetManager가 이 에셋을 식별하는 데 사용
    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Item", GetFName());
    }

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
    TSoftObjectPtr<UTexture2D> Icon;  // 소프트 레퍼런스로 아이콘 참조

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
    FGameplayTagContainer ItemTags;
};
```

### AssetManager 등록

`DefaultGame.ini`에 에셋 타입을 등록:

```ini
[/Script/Engine.AssetManagerSettings]
+PrimaryAssetTypesToScan=(PrimaryAssetType="Item",AssetBaseClass="/Script/MyGame.ItemDataAsset",bHasBlueprintClasses=false,Directories=((Path="/Game/Data/Items")))
```

### 런타임 로딩

```cpp
// 에셋 목록 조회
UAssetManager& Manager = UAssetManager::Get();
TArray<FPrimaryAssetId> AssetIds;
Manager.GetPrimaryAssetIdList("Item", AssetIds);

// 비동기 로드
FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(
    this, &AMyActor::OnAssetsLoaded);
Manager.LoadPrimaryAssets(AssetIds, {}, Delegate);

// 콜백에서 에셋 사용
void AMyActor::OnAssetsLoaded()
{
    UAssetManager& Manager = UAssetManager::Get();
    for (const FPrimaryAssetId& Id : CachedAssetIds)
    {
        UItemDataAsset* Item = Manager.GetPrimaryAssetObject<UItemDataAsset>(Id);
        if (Item)
        {
            // 에셋 사용
        }
    }
}
```

### 언로드

```cpp
Manager.UnloadPrimaryAssets(AssetIds);
```

### PrimaryDataAsset 참조 시 주의사항

`UPrimaryDataAsset`은 AssetManager를 통해 비동기 로드되는 것이 전제이므로, 다른 클래스에서 이를 참조할 때 `TObjectPtr` 하드 레퍼런스 대신 `TSoftObjectPtr`를 사용해야 한다. 하드 레퍼런스로 보유하면 소유 액터 로드 시 해당 에셋까지 강제 로드되어 AssetManager의 비동기 로딩 이점이 사라진다.

```cpp
// ❌ 하드 레퍼런스 — PrimaryDataAsset의 비동기 로딩 의미가 없어짐
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
TArray<TObjectPtr<UWeaponDataAsset>> Weapons;

// ✅ 소프트 레퍼런스 — 필요 시점에 로드
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
TArray<TSoftObjectPtr<UWeaponDataAsset>> Weapons;
```

소프트 레퍼런스 컬렉션의 요소에 접근할 때는 로드 여부를 확인하고, 미로드 시 명시적으로 로드해야 한다:

```cpp
UWeaponDataAsset* GetWeaponData(int32 Index)
{
    if (!Weapons.IsValidIndex(Index))
    {
        return nullptr;
    }

    // 이미 로드되어 있으면 바로 반환
    if (Weapons[Index].IsValid())
    {
        return Weapons[Index].Get();
    }

    // 미로드 시 동기 로드 후 반환
    return Weapons[Index].LoadSynchronous();
}
```

> **`UDataAsset` vs `UPrimaryDataAsset` 참조 방식 비교**:
> `UDataAsset`은 항상 메모리에 상주하는 것이 전제이므로 `TObjectPtr` 하드 레퍼런스가 적합하다.
> `UPrimaryDataAsset`은 AssetManager를 통한 동적 로딩이 전제이므로 `TSoftObjectPtr`가 적합하다.

---

## DataTable (FTableRowBase)

구조화된 대량 데이터(레벨 테이블, 드롭 테이블, 적 스탯 등)에는 DataTable을 사용.

### Row 구조체 정의

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

Content Browser에서 우클릭 → Miscellaneous → DataTable → `FEnemyStatsRow` 선택으로 테이블 생성.

### 런타임 조회

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

### 전체 행 순회

```cpp
TArray<FEnemyStatsRow*> AllRows;
Table->GetAllRows<FEnemyStatsRow>(TEXT(""), AllRows);

for (FEnemyStatsRow* Row : AllRows)
{
    // 각 행 처리
}
```

### CSV/JSON Import

DataTable은 CSV 또는 JSON 파일에서 import할 수 있어 기획자가 에디터 외부에서 데이터를 편집하고 반영할 수 있다.

에디터에서 DataTable 에셋 → 우클릭 → Reimport로 외부 파일 변경사항을 반영.

CSV 형식:
```
---,Health,MoveSpeed,AttackDamage,EnemyTypeTag
Goblin,50,400,5,"Enemy.Melee"
Dragon,1000,200,100,"Enemy.Boss"
Archer,80,350,15,"Enemy.Ranged"
```

첫 번째 열(`---`)은 RowName으로 사용된다.

---

## DataAsset vs DataTable 선택 기준

| 기준 | DataAsset | DataTable |
|------|-----------|-----------|
| 데이터 형태 | 개별 에셋 (무기 1개 = DataAsset 1개) | 테이블 행 (적 100종 = 테이블 1개) |
| 레퍼런스 방식 | 직접 참조 또는 AssetManager | RowName 문자열 조회 |
| 에디터 UX | 개별 에셋 더블클릭으로 편집 | 스프레드시트 형태로 일괄 편집 |
| 외부 import | 불가 (에디터 전용) | CSV/JSON import 가능 |
| 복잡한 중첩 구조 | 적합 (다른 DataAsset 참조 가능) | 부적합 (flat row 구조) |
| 블루프린트 참조 | 에셋 직접 선택 가능 | RowName 문자열 입력 필요 |
| GAS 연동 | GA, GE 클래스를 직접 참조 | TSubclassOf를 열에 포함 가능하나 불편 |

**경험적 가이드라인**: 개별 에셋이 고유한 구조(중첩 배열, 다른 에셋 참조 등)를 가지면 DataAsset, 동일 스키마의 데이터가 수십~수백 개면 DataTable.

---

## 에디터 유효성 검증

DataAsset에 에디터에서 입력 오류를 잡는 검증 로직을 추가:

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
