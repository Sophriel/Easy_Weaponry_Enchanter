# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# EWE — Easy Weaponry Enchanter

Unreal Engine 5 C++ Plugin.
무기에 **Trigger → Condition → Function** 조합을 부여하는 GAS 기반 인챈트 시스템.
"스킬 제작 툴"이 아니라 **이벤트 조합기**다.

---

## 🛠️ 스킬 활용 지침

**C++ 코드 작성, 수정, 리뷰 시 반드시 `/unreal-engine` 스킬을 활용할 것.**

이 프로젝트는 UE5 C++ + GAS 기반이다. 코드 작업 시 아래 순서를 따른다:

```
1. /unreal-engine 스킬 로드 (Pre-Flight Discovery, 레퍼런스 파일 참조)
2. 이 CLAUDE.md에서 EWE 고유 규칙 확인
3. 문서 맵에서 해당 작업의 상세 문서 읽기
```

스킬이 커버하는 영역 (CLAUDE.md에서 반복하지 않음):
- UE5 C++ 베스트 프랙티스 (GC, 리플렉션, 퍼포먼스)
- GAS 일반 패턴 (ASC 초기화, AttributeSet 생성, GE 스태킹, GA 생명주기, AbilityTask, 네트워크)
- Enhanced Input 바인딩
- Plugin 개발 (Runtime/Editor 모듈 분리, Build.cs)
- Epic 표준 명명 규칙 (U/A/F/E/I 접두사)

---

## 🌐 언어 설정

글로벌 설정 (`~/.claude/settings.json`) 의 언어 설정을 따릅니다.
프로젝트 레벨에서 언어를 재정의하지 않습니다.

---

## 📁 프로젝트 구조

```
/Source/EWE/
│   └── Character/
│       └── EWECharacter.h              ← ASC 소유 캐릭터 (핵심)
│
/Plugins/EasyWeaponryEnchanter/
│   └── Source/EasyWeaponryEnchanter/
│       ├── Ability/                    ← GA 구현체 (UEWEGA_*)
│       ├── AbilityTask/                ← AT 구현체 (UEWEAT_*)
│       └── Weapon/                     ← 무기 클래스 (UEWEWeapon*)
│
/Content/
    ├── Weapons/                        ← 무기 에셋
    └── Effects/                        ← 이펙트 에셋
```

---

## 📚 문서 맵

**코드를 읽거나 수정하기 전에 해당 문서를 먼저 읽을 것.**

| 작업 | 참조 문서 |
|------|-----------|
| 모듈 구조, GAS 컴포넌트, 무기 교체 흐름 | `Documents/MDs/architecture.md` |
| DataAsset, GA/AT 클래스, GA 생명주기, 실행 흐름 | `Documents/MDs/enchant-system.md` |
| Trigger 타입, Condition 판별, 조합 규칙 | `Documents/MDs/trigger-condition.md` |
| 새 Function 추가 **(반드시 먼저)** | `Documents/MDs/functions/overview.md` |
| Creation / Ejection / Rotation / Attach | `Documents/MDs/functions/basic.md` |
| TimeStop / Freeze / Teleport / Possess / Weather / StatusEffect | `Documents/MDs/functions/special.md` |
| 쿨타임, 레코딩, 두루마리, JSON Import/Export | `Documents/MDs/cooldown-recording.md` |
| RPC, 서버 권한, 복제 정책 | `Documents/MDs/network.md` |
| HUD, 인벤토리, 인챈터 UI | `Documents/MDs/ui.md` |

---

## 🔧 개발 환경

| 항목 | 내용 |
|------|------|
| 엔진 버전 | Unreal Engine 5.5 |
| 필수 플러그인 | `GameplayAbilities`, `EnhancedInput` |
| 빌드 타겟 | `Win64`, `Editor`, `Development` |

Hot Reload: `Editor → Tools → Hot Reload`. Build.cs 수정 시 완전 재빌드 필요.

### 문서 간 관계

- `CLAUDE.md`: 한국어 가이드 (메인)
- `CLAUDE_EN.md`: 영문 가이드 (동기화 필요 시 함께 수정)

---

## ⚙️ 아키텍처 결정 (변경 금지)

- **ASC, AttributeSet = `AEWECharacter` 소유.** 무기는 GA 목록만 보유. → `architecture.md`
- **Server Confirm 필수, Client Prediction 없음.** Confirm 전 피드백 없음. → `network.md`
- **모듈 분리:** `EWERuntime` / `EWEEditor` (`WITH_EDITOR` 엄격 분리)

---

## 🔒 EWE 프로젝트 규칙

> 일반 UE5/GAS 코드 패턴이 필요하면 `/unreal-engine` 스킬의 레퍼런스를 먼저 참조할 것.
> 아래는 **이 프로젝트에서만 적용되는 제약**이다. 일반 GAS 규칙과 충돌 시 아래가 우선.

### Attribute 수정 — GE 전용
```
이 프로젝트에서 Attribute 직접 세터는 절대 금지.
일반 GAS에선 허용되지만, EWE에서는 모든 수정을 GE로 통일한다.

// ❌ AttributeSet->SetHealth(100.f);
// ✅ ASC->ApplyGameplayEffectToSelf(DamageGE, ...);
```

### GE 스택 — Aggregate 전용
```
이 프로젝트에서 GE는 Aggregate 방식만 사용.
Override / Additive 금지. 동일 GE 중복 적용 시 완전 독립 인스턴스.
```

### GA / AbilityTask 역할 분리
```
UEWEGA_* (GA) → 고수준 흐름 정의만 담당
UEWEAT_* (AT) → 실제 실행 단계 담당

GA에서 직접 Actor 스폰 금지.
반드시 전용 AbilityTask를 통할 것.
```

### GA 종료 — PrecedingEnchantEnd 연동
```
EndAbility(bWasCancelled=false)만 PrecedingEnchantEnd 조건 트리거로 인정.
CancelAbility는 종료로 인정하지 않음.
이 두 경로를 혼용하면 인챈트 체인이 깨진다.
```

### Function 확장 — IEnchantFunction 필수
```
새 Function은 IEnchantFunction 인터페이스 구현 필수.
ExecuteFunction() 오버라이드로 파이프라인에 통합.
→ functions/overview.md 먼저 읽을 것.
```

### Attribute 참조 실패 처리
```
AttributeSet에 없는 Attribute 참조 시:
  → UE_LOG(LogEWE, Warning, ...) 출력 후 무시. 크래시 금지.
```

### 코드 주석
```
모든 코드 주석은 영어로 작성.
// ❌ 체력을 감소시킨다
// ✅ Reduces the character's health by the given amount
```

---

## 📐 핵심 클래스

```
UEnchantDataAsset       인챈트 1개 정의 (Trigger + Conditions + Functions + CooldownGE)
UEWEWeaponData          무기 정의 (슬롯 수, Attribute 기본값, 인챈트 슬롯 배열)
AEWECharacter           ASC / AttributeSet 소유 캐릭터
UEWEGA_Base             모든 GA의 베이스 클래스
UEWEAT_Base             모든 AT의 베이스 클래스 (IEnchantFunction 구현체)
IEnchantFunction        Function 확장 인터페이스
FEnchantContext         실행 컨텍스트 (시전자, 부모 무기 참조)
```

전체 클래스 맵 및 파일 위치 → `architecture.md`

---

## 🏷️ 명명 규칙

> Epic 표준 접두사(U/A/F/E/I)는 `/unreal-engine` 스킬 참조.
> 아래는 **EWE 프로젝트 전용 접두사**다.

| 분류 | 접두사 | 예시 | 파일명 |
|------|--------|------|--------|
| GameplayAbility | `UEWEGA_` | `UEWEGA_Creation` | `EWEGA_Creation.h` |
| AbilityTask | `UEWEAT_` | `UEWEAT_SpawnActor` | `EWEAT_SpawnActor.h` |
| 무기 관련 | `UEWEWeapon*` | `UEWEWeaponData` | `EWEWeaponData.h` |
| 일반 UObject | `UEWE` | `UEWEAttributeSet` | `EWEAttributeSet.h` |
| AActor 계열 | `AEWE` | `AEWECharacter` | `EWECharacter.h` |
| 구조체 | `FEWE` / `FEnchant*` | `FEnchantContext` | - |
| 로그 카테고리 | `LogEWE` | - | - |

**파일명**: 클래스명에서 `U` / `A` 제거. 예: `UEWEGA_Creation` → `EWEGA_Creation.h`

---

## ⚠️ 미결 항목

```
[ ] 에디터 툴 방향 미정
    → Details 패널 커스터마이징 or 별도 Graph/Tree 에디터 창
    → 결정 전까지 런타임 기능에만 집중할 것

[ ] 오브젝트 자연 소멸 예외 케이스
    → DestroyConditions 미설정 시 Lifetime 외 소멸 없음 (현재 정책)
    → 추가 예외 케이스 발생 시 functions/basic.md 업데이트
```
