# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# EWE — Easy Weaponry Enchanter

Unreal Engine 5 C++ Plugin.
무기에 **Trigger → Condition → Function** 조합을 부여하는 GAS 기반 인챈트 시스템.
"스킬 제작 툴"이 아니라 **이벤트 조합기**다.

---

## 🌐 언어 설정

글로벌 설정 (`~/.claude/settings.json`) 의 언어 설정을 따릅니다.
프로젝트 레벨에서 언어를 재정의하지 않습니다.

---

## 📁 프로젝트 구조

```
/Source/                                        ← 메인 프로젝트 C++ 코드
│   └── EWE/
│       └── Character/
│           └── EWECharacter.h                  ← ASC 소유 캐릭터 (핵심)
│
/Plugins/EasyWeaponryEnchanter/
│   └── Source/EasyWeaponryEnchanter/
│       ├── Ability/                            ← GA 구현체 (UEWEGA_*)
│       ├── AbilityTask/                        ← AT 구현체 (UEWEAT_*)
│       └── Weapon/                             ← 무기 클래스 (UEWEWeapon*)
│           └── EWEWeaponData.h                 ← 무기 데이터 구조
│
/Content/
    ├── Weapons/                                ← 무기 에셋
    └── Effects/                                ← 이펙트 에셋
```

---

## 📚 문서 맵

작업 전 반드시 해당 문서를 먼저 읽을 것.

| 작업 | 참조 문서 |
|------|-----------|
| 모듈 구조, GAS 컴포넌트, 플러그인 설정 | `Documents/MDs/architecture.md` |
| DataAsset 클래스, GA/AT 클래스 설계 | `Documents/MDs/enchant-system.md` |
| Trigger / Condition 구현 | `Documents/MDs/trigger-condition.md` |
| 새 Function 추가 (반드시 먼저 읽기) | `Documents/MDs/functions/overview.md` |
| Creation / Ejection / Rotation / Attach | `Documents/MDs/functions/basic.md` |
| TimeStop / Freeze / Teleport / Possess / Weather / StatusEffect | `Documents/MDs/functions/special.md` |
| 쿨타임 GE, 레코딩 시스템, 두루마리 | `Documents/MDs/cooldown-recording.md` |
| RPC, 서버 권한, 복제 정책 | `Documents/MDs/network.md` |
| HUD, 인벤토리, 인챈터 UI | `Documents/MDs/ui.md` |

---

## 🔧 개발 환경

| 항목 | 내용 |
|------|------|
| 엔진 버전 | Unreal Engine 5.5 |
| 필수 플러그인 | `GameplayAbilities`, `EnhancedInput` |
| 빌드 타겟 | `Win64`, `Editor`, `Development` |

### 빌드 및 실행

```
에디터 실행 후 게임 런:
  Editor → Play → Pi (Play in Editor)
```

### Hot Reload 워크플로우

```
1. 에디터 실행
2. C++ 코드 수정
3. Editor → Tools → Hot Reload
4. 수정 확인 (콘솔 로그 확인)
```

Build.cs 수정 시 Hot Reload 적용 안 됨 → 완전 재빌드 필요.

### 문서 간 관계

- `CLAUDE.md`: 한국어 가이드 (메인)
- `CLAUDE_EN.md`: 영문 가이드 (동기화 필요 시 함께 수정)

---

## ⚙️ 핵심 아키텍처 결정 (변경 금지)

### GAS 소유 구조
- **ASC, AttributeSet 모두 `AEWECharacter` 소유** (`/Source/EWE/Character/EWECharacter.h`)
- 무기(`UEWEWeaponData`)는 ASC를 소유하지 않음. `TSubclassOf<UGameplayAbility>` 목록만 보유
- 무기 장착 시 → `ASC->GiveAbility()`로 해당 무기의 GA들을 캐릭터 ASC에 등록
- 무기 교체 시 → 기존 GA 전부 Cancel + ASC에서 제거 후 신규 GA 등록

### 네트워크
- **모든 GA 발동은 Server Confirm 이후** 실행. Client Prediction 없음
- 서버 Confirm 전 클라이언트 피드백 없음
- 생성된 독립 오브젝트의 Owner = 서버. 영향 계산 기준 = 생성한 플레이어

### 모듈 분리
```
EWERuntime   → 핵심 런타임 (GA, GE, DataAsset, Function 등)
EWEEditor    → 에디터 전용 (WITH_EDITOR 엄격 분리)
```

---

## 🔒 절대 규칙 — 어기면 시스템이 깨진다

### Attribute 수정
```
Attribute는 반드시 GameplayEffect를 통해서만 수정.
직접 세터 호출 절대 금지.

// ❌ 절대 금지
Health -= Damage;
AttributeSet->SetHealth(100.f);

// ✅ 올바른 방법
ASC->ApplyGameplayEffectToSelf(DamageGE, ...);
```

### GE 스택
```
모든 GE는 Aggregate 방식으로 적용.
Override / Additive 사용 금지.
동일 GE 중복 적용 시 완전 독립 인스턴스로 동작해야 함.
```

### GA / AbilityTask 역할 분리
```
GameplayAbility (UEWEGA_*)  → 고수준 흐름 정의만 담당
AbilityTask     (UEWEAT_*)  → 실제 실행 단계 담당

GA에서 직접 Actor를 스폰하는 코드 작성 금지.
Actor 스폰은 반드시 전용 AbilityTask(UEWEAT_SpawnActor 등)를 통할 것.
```

### GA 종료 구분
```
EndAbility(bWasCancelled=false)  → 정상 종료. PrecedingEnchantEnd 조건 트리거로 인정.
CancelAbility / bWasCancelled=true → 강제 취소. 종료로 인정하지 않음.
두 경로를 절대 혼용하지 말 것.
```

### 무기 교체 시 처리 순서
```
1. 서버 Confirm 수신
2. 발동 중인 GA 전부 CancelAbility + ASC->ClearAbility()
3. AttributeSet을 신규 무기 기본값으로 재설정
   - 현재 HP 유지 (단, 신규 MaxHP 초과 시 MaxHP로 클램프)
   - Position / 속도 제외
   - 적용 중인 GE(쿨타임 포함) 유지 — 초기화 안 함
4. 신규 무기 GA를 ASC->GiveAbility()로 등록
```

### Attribute 참조
```
캐릭터 AttributeSet에 없는 Attribute를 참조하면:
  → UE_LOG(LogEWE, Warning, ...) 출력
  → 해당 항목 무시, 크래시 금지
```

### Function 확장
```
새 Function은 반드시 IEnchantFunction 인터페이스를 구현.
ExecuteFunction() 오버라이드만으로 기존 파이프라인에 통합됨.
Documents/MDs/functions/overview.md 먼저 읽을 것.
```

### Unreal 리플렉션 매크로
```
UCLASS, USTRUCT, UENUM, GENERATED_BODY 제거 또는 변경 금지.
UPROPERTY, UFUNCTION 스펙 제거 금지.
UE5에서 소유권이 있는 포인터는 TObjectPtr 사용.
```

### 코드 주석
```
모든 코드 주석은 영어로 작성.
// ❌ 잘못된 예: 체력을 감소시킨다
// ✅ 올바른 예: Reduces the character's health by the given amount
```

---

## 📐 주요 클래스 맵

```
// ── 데이터 ──────────────────────────────────────────────────────
UEnchantDataAsset               인챈트 1개 정의 (Trigger + Conditions + EventType + Functions + CooldownGE)
UEWEWeaponData                  무기 정의 (슬롯 수, Attribute 기본값, 인챈트 슬롯 배열)
  └─ EWEWeaponData.h            /Plugins/.../Weapon/EWEWeaponData.h

// ── GAS 핵심 ─────────────────────────────────────────────────────
AEWECharacter                   ASC / AttributeSet 소유 캐릭터
  └─ EWECharacter.h             /Source/EWE/Character/EWECharacter.h
UEWEGA_Base                     모든 GA의 베이스 클래스
  └─ EWEAbilityBase.h           /Plugins/.../Ability/EWEAbilityBase.h
UEWEAT_Base                     모든 AT의 베이스 클래스 (IEnchantFunction 구현체)
  └─ /Plugins/.../AbilityTask/

// ── 인터페이스 ───────────────────────────────────────────────────
IEWECharacterInterface          캐릭터-무기 계약
IEWEAttackAnimationInterface    공격 애니메이션 계약
IEnchantFunction                Function 확장 인터페이스 (ExecuteFunction 오버라이드)

// ── 구조체 ───────────────────────────────────────────────────────
FEnchantTrigger                 Trigger 데이터 구조체
FEnchantCondition               Condition 데이터 구조체
FEnchantFunction                Function 파라미터 래퍼 구조체
FEnchantContext                 실행 컨텍스트 (시전자, 부모 무기 참조)
```

---

## 🏷️ 명명 규칙

| 분류 | 접두사 | 예시 | 파일명 |
|------|--------|------|--------|
| GameplayAbility | `UEWEGA_` | `UEWEGA_Creation` | `EWEGA_Creation.h` |
| AbilityTask | `UEWEAT_` | `UEWEAT_SpawnActor` | `EWEAT_SpawnActor.h` |
| 무기 관련 클래스 | `UEWEWeapon*` | `UEWEWeaponData` | `EWEWeaponData.h` |
| 일반 UObject | `UEWE` | `UEWEAttributeSet` | `EWEAttributeSet.h` |
| AActor 계열 | `AEWE` | `AEWECharacter` | `EWECharacter.h` |
| 구조체 | `FEWE` | `FEnchantContext` | - |
| 로그 카테고리 | `LogEWE` | - | - |

> **파일명 규칙**: 클래스명 앞의 `U` / `A` 를 제거한 이름 사용.
> 예: `UEWEGA_Creation` → `EWEGA_Creation.h`

---

## 🚦 Trigger 타입 열거

```cpp
enum class EEnchantTriggerType : uint8
{
    OnSpawn,        // 오브젝트/액터 생성 시
    OnAttack,       // 히트 판정 발생 시 (입력 시점 아님)
    OnHit,          // 피격 시 (데미지 무관)
    OnKill,         // 처치 시
    OnDeath,        // 자신 사망 시
    OnInput,        // 키 입력 시 (Enhanced Input / IMC 기준)
    OnCombo,        // 상태 조건 + 입력 조건 AND
    OnCollision,    // 투사체 충돌 시
};
```

기본 발동 키: **LMB(좌클릭), RMB(우클릭)** 만. 추가 키는 개발자가 IMC 수정으로 확장.

---

## ⚡ 인챈트 실행 흐름

```
플레이어 입력
    │
    ▼
Client → Server RPC 전송
    │
    ▼
Server: Trigger 판정
    │
    ├─ Condition 불충족 → 거부
    ├─ CooldownGE 활성  → 거부 (클라이언트도 차단, 피드백 없음)
    │
    └─ 승인 → UEWEGA_* 활성화
                │
                └─ UEWEAT_* 동시 실행 (조건 없을 시)
                        │
                        └─ 모든 AT 완료 → GA EndAbility
```

---

## 🔁 인챈트 동시/순차 실행 규칙

```
기본: 동시 실행
순차: "PrecedingEnchantEnd" Condition이 있는 경우에만
      → 앞 인챈트의 EndAbility 시점에 발동
      → CancelAbility는 트리거 불가

하나의 GA 내 AbilityTask도 동일:
  기본 동시 실행. GA는 모든 AT 완료 후 EndAbility.
```

---

## 🌐 네트워크 처리 규칙

| 기능 | 처리 방식 |
|------|-----------|
| GA 발동 | Server Confirm 후 실행 |
| 독립 오브젝트 생성 | Server spawn, 영향 계산은 생성 플레이어 기준 |
| Teleport | 서버 목적지 보정 → 클라이언트 위치 전송 |
| Weather Change | 서버 요청 → 전 클라이언트 중계, Last-Write-Wins |
| Time Stop | 동일 틱 다수 요청 모두 처리, 다음 틱 적용 |
| 쿨타임 차단 | 클라이언트 + 서버 양쪽 차단 |

---

## ⚠️ 미결 항목 (구현 전 확인 필요)

```
[ ] 에디터 툴 방향 미정
    → Details 패널 커스터마이징 or 별도 Graph/Tree 에디터 창
    → 결정 전까지 런타임 기능에만 집중할 것

[ ] 오브젝트 자연 소멸 예외 케이스
    → DestroyConditions 미설정 시 Lifetime 외 소멸 없음 (현재 정책)
    → 추가 예외 케이스 발생 시 Documents/MDs/functions/basic.md 업데이트
```
