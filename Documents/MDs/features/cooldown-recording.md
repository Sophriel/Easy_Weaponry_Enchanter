# Cooldown & Recording System

> 쿨타임 GE 정책, 레코딩 시스템, 두루마리 아이템, JSON Export/Import를 다룬다.

---

## 쿨타임 시스템

### 구조

```
쿨타임 = GE (GameplayEffect) 로 구현.
각 인챈트 DataAsset에서 CooldownGE를 개별 설정.
CooldownGE가 없으면 쿨타임 없음.
```

### 쿨타임 GE 설정 방법

```
UEnchantDataAsset.CooldownGE에 GE 클래스 할당.
GE는 Duration 타입으로 설정.
Stacking Policy: None (GE 자체는 쿨타임용이므로 중첩 불필요).
```

### 발동 차단 정책

```
인챈트 발동 시도 시:
  1. 서버에서 CooldownGE 활성 여부 확인
  2. 클라이언트에서도 동일하게 확인 (양쪽 차단)
  3. 활성 중이면 GA 발동 거부
  4. 피드백 없음 (클라이언트에 어떤 신호도 보내지 않음)
```

### CooldownGE 취소 처리

```
CooldownGE가 외부 요인으로 Cancel 되면:
  → EndAbility와 동일하게 처리
  → 즉, GE가 Cancel되면 쿨타임이 종료된 것으로 간주
  → 이를 이용해 "쿨타임 초기화" 인챈트 구현 가능
```

### 무기 교체와 쿨타임

```
무기 교체 시 쿨타임 GE는 초기화되지 않음.
무기를 바꿔도 이전 무기의 쿨타임이 그대로 유지됨.
(AttributeSet만 재설정되고 GE는 유지)
```

### 구현 패턴

```cpp
bool UEWEGameplayAbility::CheckCooldown(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (SourceEnchant && SourceEnchant->CooldownGE)
    {
        // 쿨타임 태그가 활성화되어 있으면 발동 거부
        const UGameplayEffect* CDGE = SourceEnchant->CooldownGE.GetDefaultObject();
        FGameplayTagContainer CDTags;
        CDGE->GetGrantedTags(CDTags);

        if (ActorInfo->AbilitySystemComponent->HasAnyMatchingGameplayTags(CDTags))
        {
            return false;
        }
    }
    return true;
}
```

---

## 레코딩 시스템

### 레코딩 북

```
레코딩 북 = Recording GE가 부여된 특수 무기 아이템.
일반 무기와 동일한 AEWEWeaponActor 상속 구조.
QuickSlot[0]에 고정 배치.
```

### 레코딩 흐름

```
1. 레코딩 북 장착
2. LMB 입력
   → 레코딩 북의 GA 발동
   → 서버에서 Recording GE를 캐릭터에 적용
   → RecordingComponent.StartRecording()

3. Recording GE 활성 상태에서 발동하는 모든 GA가 기록됨
   → 무기 교체 후에도 기록 유지
   → 중복 GA도 그대로 기록 (필터링 없음)

4. 기록 종료 조건:
   a) 레코딩 북 재장착 + LMB 입력 → 두루마리 생성 후 Recording GE 해제
   b) 슬롯 초과 → 즉시 Recording GE 해제 (두루마리 생성)
```

### UEWERecordingComponent

```cpp
UCLASS()
class EWERUNTIME_API UEWERecordingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    void StartRecording();
    void StopRecording();         // 두루마리 생성 후 종료
    void OnGAActivated(UEWEGameplayAbility* GA);  // GA 발동 시 호출됨

private:
    bool bIsRecording = false;

    // 기록된 GA 목록 (순서 유지)
    TArray<TObjectPtr<UEnchantDataAsset>> RecordedEnchants;

    // 슬롯 초과 확인
    bool IsSlotLimitReached() const;

    // 두루마리 아이템 생성
    AEWEScrollItem* CreateScrollItem();
};
```

### GA 기록 훅

```cpp
// UEWEGameplayAbility::ActivateAbility 내에서 호출
void UEWEGameplayAbility::ActivateAbility(...)
{
    // 레코딩 중이면 기록
    if (UEWERecordingComponent* Rec = GetOwner()->FindComponentByClass<UEWERecordingComponent>())
    {
        if (Rec->IsRecording())
        {
            Rec->OnGAActivated(this);

            // 슬롯 초과 시 자동 종료
            if (Rec->IsSlotLimitReached())
            {
                Rec->StopRecording();
            }
        }
    }

    // 이후 일반 발동 로직 계속
}
```

---

## 두루마리 아이템

### 구조

```
두루마리 1개 = 기록된 UEnchantDataAsset 배열 전체.
인벤토리 아이템으로 존재.
소모 없이 무기에 반복 부여 가능.
```

### 무기 부여 로직

```cpp
void UEWEScrollItem::ApplyToWeapon(UWeaponDataAsset* WeaponData)
{
    int32 AvailableSlots = WeaponData->MaxEnchantSlots - WeaponData->Enchants.Num();
    int32 ApplyCount = FMath::Min(RecordedEnchants.Num(), AvailableSlots);

    // 앞에서부터 슬롯 범위 내 GA만 순차 부여
    for (int32 i = 0; i < ApplyCount; i++)
    {
        WeaponData->Enchants.Add(RecordedEnchants[i]);
    }

    if (RecordedEnchants.Num() > ApplyCount)
    {
        UE_LOG(LogEWE, Warning,
            TEXT("[ScrollItem] %d enchants skipped due to slot limit."),
            RecordedEnchants.Num() - ApplyCount);
    }

    // 원본 두루마리 소모 없음 (재사용 가능)
}
```

---

## JSON Export / Import

### Export 형식

```json
{
  "ewe_version": "1.0",
  "enchants": [
    {
      "asset_path": "/Game/Data/Enchants/DA_FireBolt.DA_FireBolt",
      "slot_index": 0
    },
    {
      "asset_path": "/Game/Data/Enchants/DA_Burn.DA_Burn",
      "slot_index": 1
    }
  ]
}
```

### Import 정책

```
위변조 허용: JSON을 그대로 읽음 (보안 검증 없음, 오픈 공유 설계).

서버에 없는 클래스:
  → 해당 항목만 스킵
  → Warning 출력: "[EWE] Enchant class not found: {path}. Skipped."
  → 나머지 항목은 정상 로드

버전 불일치:
  → 우선 로드 시도
  → Warning 출력: "[EWE] Version mismatch: {file_version} vs {current_version}."
  → 파싱 실패 시에만 해당 항목 스킵
```

### Import 구현 패턴

```cpp
bool UEWEJsonImporter::ImportFromFile(
    const FString& FilePath,
    UWeaponDataAsset* TargetWeapon)
{
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *FilePath))
    {
        UE_LOG(LogEWE, Warning, TEXT("[EWE] Failed to read file: %s"), *FilePath);
        return false;
    }

    TSharedPtr<FJsonObject> JsonObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
    {
        UE_LOG(LogEWE, Warning, TEXT("[EWE] Failed to parse JSON: %s"), *FilePath);
        return false;
    }

    // 버전 체크 (불일치 허용, Warning만)
    FString FileVersion = JsonObj->GetStringField("ewe_version");
    if (FileVersion != CURRENT_EWE_VERSION)
    {
        UE_LOG(LogEWE, Warning, TEXT("[EWE] Version mismatch: %s vs %s"),
            *FileVersion, *CURRENT_EWE_VERSION);
    }

    const TArray<TSharedPtr<FJsonValue>>& EnchantArr =
        JsonObj->GetArrayField("enchants");

    for (const TSharedPtr<FJsonValue>& Entry : EnchantArr)
    {
        FString AssetPath = Entry->AsObject()->GetStringField("asset_path");
        UEnchantDataAsset* Asset = Cast<UEnchantDataAsset>(
            StaticLoadObject(UEnchantDataAsset::StaticClass(), nullptr, *AssetPath));

        if (!Asset)
        {
            UE_LOG(LogEWE, Warning,
                TEXT("[EWE] Enchant class not found: %s. Skipped."), *AssetPath);
            continue;
        }

        TargetWeapon->Enchants.Add(Asset);
    }

    return true;
}
```
