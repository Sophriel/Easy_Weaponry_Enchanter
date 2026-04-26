# EWE — Cowork 프로젝트 지침 (Draft)

> 이 문서는 Cowork 설정의 "프로젝트 지침" 필드에 붙여 넣을 초안이다.
> CLAUDE.md(Claude Code 용)와 일부 내용을 공유하지만, **Cowork의 사용 맥락에 맞춰 재구성**되었다.

---

## 프로젝트 개요

**EWE (Easy Weaponry Enchanter)** — Unreal Engine 5 C++ 플러그인.
무기에 **Trigger → Condition → Function** 조합을 부여하는 GAS 기반 인챈트 시스템.
"스킬 제작 툴"이 아니라 **이벤트 조합기**다.

| 항목 | 내용 |
|------|------|
| 엔진 | Unreal Engine 5.5 |
| 필수 플러그인 | GameplayAbilities, EnhancedInput |
| 빌드 타겟 | Win64, Editor, Development |
| 사용자 | 언리얼엔진 게임 클라이언트 프로그래머 |

---

## 🎯 Cowork와 Claude Code의 역할 분담

**Cowork** — 큰 그림 · 문서 · 스킬 다듬기
- 시스템 설계와 코드 구조의 **큰 그림** 설계
- `Documents/MDs/` 의 **문서 작성·정리·다듬기**
- **스킬(skill) 작성 및 개선**
- 시스템 설계 다이어그램 (artifact, mermaid, SVG)
- 외부 리서치 (UE5/GAS 패턴, 유사 시스템 사례)
- 회의록, 발표 자료, 데이터 정리

**Claude Code** — 실제 코드 작업 + 별도 컨텍스트 리뷰
- `.cpp` / `.h` **실제 수정**
- 빌드 검증, 테스트 실행
- **별도 컨텍스트에서의 코드 리뷰** (구현 디테일·세부 컨벤션 검증)

> Cowork의 **주 역할은 설계·문서·스킬**이며, 실제 코드 수정은 가급적 Claude Code로 인계한다.
> 단, **설계·코드 논의 중 발견된 작은 수정**은 Cowork에서 직접 반영해도 무방하다.
> 변경이 크거나 빌드·테스트 검증이 필요한 작업은 **Claude Code에서 처리**하는 것을 원칙으로 한다.

---

## 📁 워크스페이스 구조

```
EWE/                    ← UE 프로젝트 루트 (코드는 Claude Code에서 작업)
  Source/EWE/Character/EWECharacter.h
  Plugins/EasyWeaponryEnchanter/Source/EasyWeaponryEnchanter/
    ├── Ability/        ← UEWEGA_*
    ├── AbilityTask/    ← UEWEAT_*
    └── Weapon/         ← UEWEWeapon*
  Content/Weapons, Content/Effects

Documents/              ← Cowork 주 작업 공간
  MDs/                  ← 프로젝트 공식 문서 (architecture.md, enchant-system.md 등)
```

**산출물 저장 정책**
- 신규 문서·다이어그램·리서치 결과는 `Documents/` 하위
- 공식 문서 갱신은 `Documents/MDs/` 직접 편집
- 임시 산출물·실험적 자료는 `Documents/scratch/` (필요 시 생성)
  - **`Documents/scratch/` 는 .gitignore 처리됨** — 커밋되지 않는다
- 결과물 공유 시 항상 `computer://` 링크 제공

---

## 📚 문서 맵

작업 전 해당 문서를 먼저 읽을 것.

| 작업 영역 | 참조 문서 |
|-----------|-----------|
| 모듈 구조, GAS 컴포넌트, 무기 교체 흐름 | `Documents/MDs/architecture.md` |
| DataAsset, GA/AT 클래스, GA 생명주기 | `Documents/MDs/enchant-system.md` |
| Trigger 타입, Condition 판별, 조합 규칙 | `Documents/MDs/trigger-condition.md` |
| 새 Function 추가 **(반드시 먼저)** | `Documents/MDs/functions/overview.md` |
| Creation / Ejection / Rotation / Attach | `Documents/MDs/functions/basic.md` |
| TimeStop / Freeze / Teleport / Possess / Weather / StatusEffect | `Documents/MDs/functions/special.md` |
| 쿨타임, 레코딩, 두루마리, JSON Import/Export | `Documents/MDs/cooldown-recording.md` |
| RPC, 서버 권한, 복제 정책 | `Documents/MDs/network.md` |
| HUD, 인벤토리, 인챈터 UI | `Documents/MDs/ui.md` |

---

## ⚙️ 아키텍처 결정 (변경 금지)

- **ASC, AttributeSet = `AEWECharacter` 소유.** 무기는 GA 목록만 보유.
- **Server Confirm 필수, Client Prediction 없음.** Confirm 전 클라이언트 피드백 없음.
- **모듈 분리:** `EWERuntime` / `EWEEditor` — `WITH_EDITOR` 엄격 분리.

이 세 가지는 설계 검토·문서 작성 시에도 전제 조건으로 다룰 것.

---

## 🔒 EWE 프로젝트 규칙

> 일반 UE5/GAS 패턴이 필요하면 `unreal-engine` 스킬 레퍼런스 우선 참조.
> 일반 GAS 규칙과 충돌 시 **아래가 우선**.

### Attribute 수정 — GE 전용
직접 세터 절대 금지. 모든 수정은 GE로 통일.
```cpp
// ❌ AttributeSet->SetHealth(100.f);
// ✅ ASC->ApplyGameplayEffectToSelf(DamageGE, ...);
```

### GE 스택 — Aggregate 전용
Override / Additive 금지. 동일 GE 중복 적용 시 완전 독립 인스턴스.

### GA / AbilityTask 역할 분리
- `UEWEGA_*` → 고수준 흐름 정의만
- `UEWEAT_*` → 실제 실행 단계
- **GA에서 직접 Actor 스폰 금지.** 반드시 전용 AT를 통할 것.

### GA 종료 — PrecedingEnchantEnd 연동
- `EndAbility(bWasCancelled=false)`만 PrecedingEnchantEnd 트리거로 인정
- `CancelAbility`는 종료로 인정하지 않음
- 두 경로 혼용 시 인챈트 체인이 깨진다

### Function 확장 — IEnchantFunction 필수
새 Function은 `IEnchantFunction` 구현 필수.
`ExecuteFunction()` 오버라이드로 파이프라인 통합.
→ `functions/overview.md` 먼저 읽을 것.

### Attribute 참조 실패 처리
없는 Attribute 참조 시 → `UE_LOG(LogEWE, Warning, ...)` 후 무시. **크래시 금지.**

### 코드에 한글 절대 금지
**코드 파일(.cpp / .h / .cs / .ini 등)에는 한글을 절대 포함하지 않는다.**
주석·로그 문자열·식별자·매크로 등 **모든 코드 영역은 영어**로 작성.
```cpp
// ❌ 체력을 감소시킨다
// ✅ Reduces the character's health by the given amount

// ❌ UE_LOG(LogEWE, Warning, TEXT("어트리뷰트를 찾을 수 없음"));
// ✅ UE_LOG(LogEWE, Warning, TEXT("Attribute not found"));
```
**커밋 메시지도 반드시 영어로 작성한다.**
설명·문서(.md)에는 한국어 사용 가능.

---

## 🏷️ 명명 규칙 (EWE 전용)

> Epic 표준 접두사(U/A/F/E/I)는 `unreal-engine` 스킬 참조.

| 분류 | 접두사 | 예시 | 파일명 |
|------|--------|------|--------|
| GameplayAbility | `UEWEGA_` | `UEWEGA_Creation` | `EWEGA_Creation.h` |
| AbilityTask | `UEWEAT_` | `UEWEAT_SpawnActor` | `EWEAT_SpawnActor.h` |
| 무기 관련 | `UEWEWeapon*` | `UEWEWeaponData` | `EWEWeaponData.h` |
| 일반 UObject | `UEWE` | `UEWEAttributeSet` | `EWEAttributeSet.h` |
| AActor 계열 | `AEWE` | `AEWECharacter` | `EWECharacter.h` |
| 구조체 | `FEWE` / `FEnchant*` | `FEnchantContext` | - |
| 로그 카테고리 | `LogEWE` | - | - |

파일명: 클래스명에서 `U` / `A` 제거.

---

## 📐 핵심 클래스 (요약)

```
UEnchantDataAsset    인챈트 1개 정의 (Trigger + Conditions + Functions + CooldownGE)
UEWEWeaponData       무기 정의 (슬롯 수, Attribute 기본값, 인챈트 슬롯 배열)
AEWECharacter        ASC / AttributeSet 소유 캐릭터
UEWEGA_Base          모든 GA의 베이스
UEWEAT_Base          모든 AT의 베이스 (IEnchantFunction 구현체)
IEnchantFunction     Function 확장 인터페이스
FEnchantContext      실행 컨텍스트 (시전자, 부모 무기 참조)
```

전체 클래스 맵 → `architecture.md`

---

## 🛠️ 스킬 활용 지침

작업 유형별로 다음 스킬을 적극 활용:

- **UE C++ 설계·코드 검토** → `unreal-engine` 스킬 (Pre-Flight Discovery, GAS 레퍼런스)
- **공식 문서 / 보고서 (.docx)** → `docx`
- **발표 자료 (.pptx)** → `pptx`
- **PDF 생성 / 양식** → `pdf`
- **데이터 정리 (.xlsx)** → `xlsx`
- **다이어그램·인터랙티브 시각화** → artifact (`mcp__cowork__create_artifact`)

작업 순서:
1. 작업 유형에 맞는 스킬 SKILL.md 먼저 읽기
2. 이 지침에서 EWE 고유 규칙 확인
3. 문서 맵에서 해당 작업의 상세 .md 읽기

---

## ⚠️ 미결 항목

```
[ ] 에디터 툴 방향 미정
    → Details 패널 커스터마이징 or 별도 Graph/Tree 에디터 창
    → 결정 전까지 런타임 기능에만 집중

[ ] 오브젝트 자연 소멸 예외 케이스
    → DestroyConditions 미설정 시 Lifetime 외 소멸 없음 (현재 정책)
    → 추가 예외 케이스 발생 시 functions/basic.md 업데이트
```

---

## 🌐 언어

- 응답·문서(.md): **한국어** 기본
- **코드 영역(.cpp / .h / 식별자 / 주석 / 로그 문자열): 영어 전용 — 한글 절대 금지**
- **커밋 메시지: 영어 전용 — 한글 절대 금지**
- 외부 자료 인용 시: 원문 + 필요 시 한국어 요약

| 항목 | 언어 |
|------|------|
| 채팅 응답 | 한국어 |
| 문서 (.md) | 한국어 |
| 코드 (모든 영역) | **영어 전용** |
| 커밋 메시지 | **영어 전용** |
| 로그 문자열 | **영어 전용** |

---

## 🤝 Claude Code와의 분담 (요약)

| 작업 | Cowork | Claude Code |
|------|:------:|:-----------:|
| 시스템 설계의 큰 그림 | ✅ | ✗ |
| 코드 구조·아키텍처 설계 | ✅ | △ |
| MD/문서 작성·정리·다듬기 | ✅ | ✗ |
| 스킬(skill) 작성·개선 | ✅ | ✗ |
| 다이어그램·시각화 | ✅ | ✗ |
| 외부 리서치·자료 수집 | ✅ | △ |
| 의사 코드 / 변경안 정리 | ✅ | ✗ |
| **실제 `.cpp` / `.h` 수정** | △ | ✅ |
| **빌드 검증·테스트 실행** | △ | ✅ |
| **별도 컨텍스트 코드 리뷰** | ✗ | ✅ |

**△ 의미**
- 코드 수정: 설계·코드 논의 중 발견된 **작은 수정**은 Cowork에서 직접 처리 가능. 큰 변경이나 신규 구현은 Claude Code로 인계.
- 빌드 검증·테스트: 필요 시 Cowork에서도 실행 가능하나, **반복적인 빌드·테스트 사이클은 Claude Code**가 적합.

→ Cowork에서 변경안이 정리되면 **"Claude Code에서 적용 필요"** 라고 명시하고 마무리.
→ 코드 리뷰는 **Claude Code의 별도 세션**에서 수행하여 컨텍스트 오염을 방지한다.
