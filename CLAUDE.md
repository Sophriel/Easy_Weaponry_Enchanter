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

응답·문서 언어는 글로벌 설정 (`~/.claude/settings.json`) 의 언어 설정을 따릅니다.
프로젝트 레벨에서 언어를 재정의하지 않습니다.

| 항목 | 언어 |
|------|------|
| 채팅 응답 | 글로벌 설정 (한국어) |
| 문서 (.md) | 한국어 |
| 코드 (모든 영역) | **영어 전용 — 한글 절대 금지** (`@@@` 마커 주석만 예외) |
| 커밋 메시지 | **영어 전용 — 한글 절대 금지** |
| 로그 문자열 | **영어 전용** |

> 상세는 아래 "🔒 EWE 프로젝트 규칙 → 코드에 한글 절대 금지 / 커밋 메시지" 참조.

---

## 📁 프로젝트 구조

```
/Source/EWE/
│   ├── Character/
│   │   └── EWECharacter.h              ← ASC 소유 캐릭터 (핵심)
│   └── UI/                             ← HUD (MVVM). View + ViewModel + 관리 서브시스템
│
/Plugins/EasyWeaponryEnchanter/
│   └── Source/EasyWeaponryEnchanter/
│       ├── Ability/                    ← GA 구현체 (UEWEGA_*)
│       ├── AbilityTask/                ← AT 구현체 (UEWEAT_*)
│       └── Weapon/                     ← 무기 클래스 (UEWEWeapon*)
│
/Content/
│   ├── Weapons/                        ← 무기 에셋
│   └── Effects/                        ← 이펙트 에셋
│
/Documents/MDs/                         ← 모든 프로젝트 md 문서
    ├── core/                           ← 시스템 구조·설계
    ├── functions/                      ← Function 카탈로그
    ├── features/                       ← 부가 시스템
    ├── guides/                         ← 작업 프로세스
    └── plans/                          ← 작업 플랜 (gitignore)
```

---

## 📚 문서 맵

**코드를 읽거나 수정하기 전에 해당 문서를 먼저 읽을 것.**

| 작업 | 참조 문서 |
|------|-----------|
| 모듈 구조, GAS 컴포넌트, 무기 교체 흐름 | `Documents/MDs/core/architecture.md` |
| **UI 계층 (MVVM), ViewModel 소유·초기화 순서** | `Documents/MDs/core/architecture.md` → "UI 계층 (MVVM)" |
| DataAsset, GA/AT 클래스, GA 생명주기, 실행 흐름 | `Documents/MDs/core/enchant-system.md` |
| Trigger 타입, Condition 판별, 조합 규칙 | `Documents/MDs/core/trigger-condition.md` |
| RPC, 서버 권한, 복제 정책 | `Documents/MDs/core/network.md` |
| 새 Function 추가 **(반드시 먼저)** | `Documents/MDs/functions/overview.md` |
| Creation / Ejection / Rotation / Attach | `Documents/MDs/functions/basic.md` |
| TimeStop / Freeze / Teleport / Possess / Weather / StatusEffect | `Documents/MDs/functions/special.md` |
| 쿨타임, 레코딩, 두루마리, JSON Import/Export | `Documents/MDs/features/cooldown-recording.md` |
| HUD, 인벤토리, 인챈터 UI | `Documents/MDs/features/ui.md` |
| 커밋 **(커밋 시도 전 반드시)** | `Documents/MDs/guides/commit-guide.md` |

---

## 📄 문서 배치 규칙

### md 파일 위치

**저장소 루트에 두는 md 는 아래 4개뿐이다. 그 외 모든 md 는 `Documents/MDs/` 하위에 만든다.**

| 파일 | 용도 |
|------|------|
| `CLAUDE.md` | Claude 용 프로젝트 가이드 (한국어, 메인) |
| `CLAUDE_EN.md` | 동일 내용 영문판 |
| `AGENTS.md` | Codex 용 가이드 |
| `README.md` | 저장소 소개 |

새 md 를 저장소 루트나 `Documents/` 최상단에 만들지 않는다.
`Documents/` 최상단은 docx / pdf 등 비-md 산출물 전용이다.

### MDs 하위 폴더

| 폴더 | 넣는 것 |
|------|---------|
| `core/` | 시스템 구조·설계 — 아키텍처, 인챈트 시스템, Trigger/Condition, 네트워크 |
| `functions/` | Function 카탈로그 |
| `features/` | 부가 시스템 — 쿨타임/레코딩, UI |
| `guides/` | 작업 프로세스 — 커밋 가이드 등 |
| `plans/` | 작업 플랜 (아래 참조) |

- 어느 폴더에도 맞지 않으면 **새 폴더를 임의로 만들지 말고 먼저 사용자에게 물을 것**
- 문서를 추가·이동하면 위 **문서 맵** 표와 `CLAUDE_EN.md` / `AGENTS.md` 를 함께 갱신한다

### 작업 플랜 — `plans/` 전용

코드 적용 계획, 리팩터링 계획, 버그 수정 계획 등
**"앞으로 할 작업"을 기술하는 md 는 예외 없이 `Documents/MDs/plans/` 에 만든다.**

```
❌ Documents/plan.md
❌ Documents/MDs/quickslot-plan.md
❌ Documents/MDs/core/quickslot-viewmodel.md
✅ Documents/MDs/plans/quickslot-viewmodel.md
```

- **파일명**: kebab-case 영어. 작업 대상이 드러나게 (`fix-inventory-load-race.md`)
- **개정**: 이전 판을 지우지 않고 `-v2`, `-v3` 접미사로 남긴다 (판단 이력 보존)
- **`plans/` 는 `.gitignore` 처리됨 — 커밋되지 않는 로컬 전용이다.**
  플랜 내용을 영구 보존하려면 완료 후 해당 설계 문서(`core/` 등)에 반영할 것
- 플랜은 md 이므로 한국어로 작성한다. "코드에 한글 금지" 규칙 대상이 아니다

---

## 🔧 개발 환경

| 항목 | 내용 |
|------|------|
| 엔진 버전 | Unreal Engine 5.5 |
| 필수 플러그인 | `GameplayAbilities`, `EnhancedInput`, `ModelViewViewModel` |
| 빌드 타겟 | `Win64`, `Editor`, `Development` |

### 빌드 검증

C++ 파일(.h 또는 .cpp)을 수정한 후, 다음 작업으로 넘어가기 전에 아래 빌드 명령어를 실행하고 성공 여부를 확인할 것.

```
"`"C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat`" EWEEditor Win64 Development `"c:\Users\nsa24\Desktop\Projects\Unreal Projects\EWE\EWE.uproject`" -WaitMutex"
```

빌드 실패 시:
1. 에러 출력을 읽는다
2. 해당 파일의 문제를 수정한다
3. 성공할 때까지 재빌드한다
4. 성공 후에만 다음 작업으로 진행한다

여러 파일을 한꺼번에 수정하지 말고, 논리적 변경 단위마다 빌드를 수행할 것.

### 문서 간 관계

- `CLAUDE.md`: 한국어 가이드 (메인)
- `CLAUDE_EN.md`: 영문 가이드 (동기화 필요 시 함께 수정)

---

## ⚙️ 아키텍처 결정 (변경 금지)

- **ASC, AttributeSet = `AEWECharacter` 소유.** 무기는 GA 목록만 보유. → `architecture.md`
- **Server Confirm 필수, Client Prediction 없음.** Confirm 전 피드백 없음. → `network.md`
- **모듈 분리:** `EWERuntime` / `EWEEditor` (`WITH_EDITOR` 엄격 분리)
- **HUD 는 MVVM.** 위젯은 데이터를 소유하지 않는다. → `architecture.md` "UI 계층"
- **초기화 순서를 가정하지 않는다.** 인벤토리 로드 · 폰 빙의 · 위젯 부착은 독립된 축이다.
  준비되면 알리고, 받는 쪽이 조건을 판정한다. → 아래 "UI 계층 규칙"

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

### UI 계층 규칙 — MVVM

```
위젯은 데이터를 소유하지 않는다. 바인딩만 한다.
ViewModel 은 View 를 참조하지 않는다. View → ViewModel → Model 이 유일한 방향.

ViewModel 생성과 이벤트 구독은 서브시스템 Initialize() 에서 1회.
  ❌ Bind*() 안에서 구독 — 조기 반환 분기가 많아 0회 또는 2회가 된다
  ✅ Initialize() 에서 NewObject + AddUObject

Bind*() 는 몇 번 불려도 안전해야 한다. 이미 채워졌으면 덮어쓰지 않는다.

데이터가 아직 없는 상태는 에러가 아니라 기대 상태다.
  → Warning 이 아니라 Verbose 로 남기고 조용히 넘어간다.

BP 프로퍼티 바인딩 금지 (DefaultEditor.ini: PropertyBindingRule=Prevent).
반드시 ViewModel 바인딩을 쓴다.
```

### 퀵슬롯 ViewModel — SetSlot 단일 관문

```
슬롯 내용을 바꾸는 모든 경로는 UEWEQuickSlotViewModel::SetSlot() 을 거친다.
  SeedFromInventory / AddWeapon / ClearSlot → 전부 SetSlot 경유

SlotItem->SetWeapon() 직접 호출 금지.
SetSlot() 이 "선택 중인 슬롯의 내용이 바뀌면 재장착 브로드캐스트" 를 담당하므로,
우회하면 그 보호가 조용히 사라진다.

// ❌ SlotItems[i]->SetWeapon(Weapon);
// ✅ SetSlot(i, Weapon);
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

### 코드에 한글 절대 금지
```
코드 파일(.cpp / .h / .cs / .ini 등)에는 한글을 절대 포함하지 않는다.
주석·로그 문자열·식별자·매크로 등 모든 코드 영역은 영어로 작성.

// ❌ 체력을 감소시킨다
// ✅ Reduces the character's health by the given amount

// ❌ UE_LOG(LogEWE, Warning, TEXT("어트리뷰트를 찾을 수 없음"));
// ✅ UE_LOG(LogEWE, Warning, TEXT("Attribute not found"));

설명·문서(.md)에는 한국어 사용 가능. 코드 안에서만 금지.

예외: `@@@` 마커 주석 (아래 "@@@ 마커 주석" 참조)
```

### `@@@` 마커 주석 — 규칙 예외 / 커밋 금지
```
`@@@` (개수 무관: @@@, @@@@, @@@@@ 등) 로 시작하는 주석은
사용자가 직접 확인하고 처리하기 위해 의도적으로 남긴 개인 마커다.

  // @@@ Will be moved later
  // @@@@@ TODO: 무기에 의한 GE 제거

특성:
  - 한글 사용 허용 — "코드에 한글 절대 금지" 규칙의 유일한 예외
  - 문장 형식·톤·서식 규칙을 따르지 않아도 된다
  - 작성자는 사용자 본인이다

Claude의 처리 방침:
  1. 임의로 수정·삭제·번역·재작성하지 않는다
  2. 한글이라는 이유로 지적하거나 영어 전환을 제안하지 않는다
  3. 해당 마커가 가리키는 작업을 완료했을 때만 마커를 제거한다
  4. 커밋 전 반드시 잔존 여부를 확인한다

커밋 금지:
  `@@@` 마커가 남아 있는 파일은 커밋하지 않는다.
  스테이징된 변경에서 마커를 발견하면 커밋을 중단하고 사용자에게 보고할 것.

  확인 명령 (추가된 줄만 검사):
    git diff --cached | grep -n "^+.*@@@"
```

### 커밋 — `commit-guide.md` 필수 참조
```
커밋을 요청받으면 작업 전에 반드시 읽을 것:
  → Documents/MDs/guides/commit-guide.md

커밋 메시지는 반드시 영어로 작성. 한글 절대 금지.

// ❌ "체력 회복 GE 추가"
// ✅ "Add health recovery GE"

핵심 규칙 (상세는 commit-guide.md):
  - author/committer = Claude Opus 5 <noreply@anthropic.com>
  - Co-Authored-By: Lee Jae Hyeong <nsa24604@naver.com>
  - 논리적 변경 1개 = 커밋 1개. 정규화와 기능 변경 혼합 금지
  - git push 금지 — 사용자가 직접 수행
  - Cowork 환경에서는 커밋 전 .git/*.lock 을 rename 으로 정리
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

### UI 계층

```
UEWELocalUIManageSubsystem      ViewModel 소유 · 위젯 생성 · 게임플레이 명령 중계
UEWEStatusViewModel             ASC 구독 → HP / MP
UEWETargetInfoViewModel         타겟 ASC 구독
UEWEQuickSlotViewModel          슬롯 배열 · 선택 인덱스 · OnSlotSelected
UEWEQuickSlotItemViewModel      슬롯 1칸 (Weapon / bSelected / SlotIndex)
UEWEQuickSlot / UEWEStatus / UEWETargetInfo   View. 데이터 미소유
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
| ViewModel | `UEWE*ViewModel` | `UEWEQuickSlotViewModel` | `EWEQuickSlotViewModel.h` |
| UI 위젯 (View) | `UEWE*` | `UEWEQuickSlot` | `EWEQuickSlot.h` |
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

[ ] 인벤토리 위젯 MVVM 미전환
    → UEWEInventory 만 아직 서브시스템을 직접 조회한다
    → 퀵슬롯 배정 경로가 둘 (EWEInventory 직접 / PC::AssignSelectedInventoryItemToSlot)
    → 인벤토리 UI 작업 시 단일화

[ ] 입력 라우팅 3-hop 체인
    → AEWECharacter → PC → Subsystem → ViewModel
    → 포워딩 함수 자체는 유지하기로 결정 (근거: architecture.md "UI 계층")
    → 체인 길이는 별도 검토 사항

[ ] architecture.md 앞부분이 목표 구조와 실제 구조를 섞어 서술
    → "플러그인 모듈 구조" / "주요 클래스 파일 위치" 절이 실제 코드와 다름
    → 대조 후 정리 필요
```
