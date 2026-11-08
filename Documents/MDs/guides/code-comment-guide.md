# 코드 주석 가이드

> C++ 코드(`.h` / `.cpp`)를 쓰거나 고칠 때 이 문서의 기준으로 주석을 단다.
> 언어 규칙(영어 전용)과 `@@@` 마커 정책의 원본은 `CLAUDE.md` 다. 여기서는 반복하지 않는다.

---

## 1. 판단 기준

> **이 주석을 지웠을 때, 코드와 이름만 보고는 되찾을 수 없는 정보가 사라지는가?**
> 아니라면 쓰지 않는다.

코드가 **무엇을** 하는지는 코드가 말한다.
주석은 **왜** 그렇게 하는지, 또는 코드만 봐서는 **무엇을 놓치게** 되는지를 쓴다.

툴팁 주석(§4)은 이 기준의 예외다. 읽는 사람이 코드를 보지 않기 때문이다.

---

## 2. 쓰는 주석

| 종류 | 무엇을 쓰는가 |
|------|---------------|
| **이유** | 이렇게 짠 이유. 특히 더 자연스러워 보이는 다른 방법을 쓰지 않은 이유 |
| **제약 · 순서 의존성** | 엔진 동작, 초기화 순서, 서버 권한처럼 원인이 코드 밖에 있는 것 |
| **의도된 이상함** | 빈 함수, 일부러 하는 nullptr 브로드캐스트, 0이어도 실행하는 분기 — 버그처럼 보이지만 아닌 것 |
| **알려진 함정** | 다음 사람이 밟을 지뢰. 빌드 구성·플랫폼별 차이 등 |

실제 코드의 예:

**이유** — Base 가 아니라 Current 를 쓰는 이유 (`EWEAttributeBase.cpp`, `GetClampRange`)
```cpp
// The upper bound is the Max's Current value, not its Base, so a MaxHealth
// equip buff raises the recovery cap immediately.
OutMax = GetMaxHealth();
```

**제약 · 순서 의존성** (`EWECharacter.cpp`, `BeginPlay`)
```cpp
// Subscribe before Super::BeginPlay(), which dispatches component BeginPlay.
// The inventory can broadcast its initial sync from there.
```

**의도된 이상함** (`EWEAttributeBase.cpp`, `PostAttributeChange` / `EWECharacter.cpp`, `AcquireWeapon`)
```cpp
// Always apply, even when the delta is zero: it re-runs the dependent's aggregator so
// PreAttributeChange re-clamps Current against the new Max.

// Intentionally empty. Required by IEWECharacterInterface.
// Acquisition is owned by UEWEInventoryComponent::AcquireWeapon, ...
```

**알려진 함정** (`EWECharacter.cpp`)
```cpp
// GetActorLabel() is declared inside #if WITH_EDITOR, so this code does not
// compile for non-editor targets (Shipping / Game).
```

---

## 3. 쓰지 않는 주석

| 종류 | 예 | 대신 |
|------|----|------|
| **뜻풀이** — 코드를 영어 문장으로 다시 읽은 것 | `// Clamp Health into the valid range.` | 쓰지 않는다 |
| **함수 본문 섹션 라벨** | `// Mesh`, `// Ability`, `// Jumping` | 라벨이 필요할 만큼 길면 함수를 나눌 신호 |
| **반복** | 선언부 설명을 정의부에서 한 번 더 | §5 |
| **주석 처리된 코드** | `// else if (Attribute == GetMoveSpeedAttribute())` 같은 예시 코드 포함 | 예시가 필요하면 `Documents/MDs/` 문서에, 옛 코드는 커밋 이력에 |
| **변경 이력 · 작업 서술** | `// Changed to weak pointer`, `// Added for the quick slot fix` | 커밋 메시지 |
| **사실이 아닌 주석** | `/** Delegates to AEWEHUD::SetSlot */` — 존재하지 않는 클래스 | 코드를 고치면 근처 주석도 함께 고친다. 틀린 주석은 없는 주석보다 나쁘다 |

같은 함수 안의 나쁜 예와 좋은 예 (`EWEAttributeBase.cpp`, `GetClampRange`):

```cpp
// ❌ Narration — restates the two lines below
// MaxHealth: [1, max float].
OutMin = 1.f;
OutMax = TNumericLimits<float>::Max();

// ✅ Reason — something the code cannot say on its own
// The upper bound is the Max's Current value, not its Base, so a MaxHealth
// equip buff raises the recovery cap immediately.
OutMin = 0.f;
OutMax = GetMaxHealth();
```

> 헤더의 **선언 그룹 구분선**(`// --- Vital Attributes ---`)은 허용한다.
> 함수 본문의 라벨과 달리, 긴 헤더를 훑어볼 때 실제로 길잡이가 된다.

---

## 4. 툴팁 주석 — 장려

UHT 는 리플렉션 선언(`UPROPERTY` / `UFUNCTION` / `UCLASS` / `USTRUCT` / `UENUM`) 바로 위의 주석을 `ToolTip` 메타데이터로 만든다.
**주석 형식은 상관없다.** `//` 주석도 툴팁이 된다 (`EWEInventoryComponent.gen.cpp` 에서 `// Added from Savedata or Editor` 가 `ToolTip` 으로 들어간 것을 확인).
에디터 Details 패널과 BP 노드 위에 뜨는 이 글은 **코드를 보지 않는 사람**(디자이너, BP 작업자)이 읽는 문서다.
그래서 §1 기준을 적용하지 않는다. 이름을 풀어 쓴 한 줄이라도 없는 것보다 낫다.

### 대상

에디터 · BP 에 노출되는 리플렉션 선언.

- `UPROPERTY` — `EditAnywhere` / `EditDefaultsOnly` / `VisibleAnywhere` / `BlueprintReadOnly` / `BlueprintReadWrite`
- `UFUNCTION` — `BlueprintCallable` / `BlueprintPure` / `BlueprintImplementableEvent` / `BlueprintNativeEvent`
- `UCLASS` (BP 로 상속하거나 에디터에서 고르는 클래스), `USTRUCT(BlueprintType)`, `UENUM` 과 그 값
- **DataAsset 필드** — EWE 에서 특히 중요하다. 인챈트 · 무기 데이터는 에디터에서 채운다

노출되지 않는 선언(private 헬퍼, `UFUNCTION()` 만 붙은 RepNotify 등)은 §1 기준을 따른다.

### 형식

`/** */` 로 쓴다. Epic 코드의 관례이고, 툴팁으로 쓰려는 주석임이 드러난다.
`//` 로 바꾼다고 툴팁에서 빠지지 않는다는 점에 주의한다 (아래 "툴팁에 쓰지 않는 것").

### 좋은 툴팁에 들어갈 것

- 이 값이 **무엇을 정하는가** (구현이 아니라 쓰는 사람 관점)
- **단위** — 초, cm, %, 배율
- **특수값의 의미** — `0` = 무제한, 비어 있으면 어떻게 되는가
- **다른 필드와의 관계** — 무엇과 함께 써야 하는가

범위는 툴팁에 적기보다 `meta = (ClampMin, ClampMax)` 로 강제하고, 툴팁에는 그 범위의 **의미**를 쓴다.

```cpp
// Illustrative example — not current EWE code
/** Seconds before the spawned actor is destroyed. 0 = no time limit. */
UPROPERTY(EditAnywhere, Category = "Lifetime", meta = (ClampMin = "0", Units = "s"))
float Lifetime = 0.f;
```

### 툴팁에 쓰지 않는 것

리플렉션 선언 바로 위의 주석은 형식과 관계없이 그대로 에디터에 노출된다.
내부 호출 순서, 위임 대상, GC · 복제 같은 **구현자용 설명은 선언 위에 두지 않고** `.cpp` 정의부 주석으로 옮긴다.

쓰는 사람에게 알려 줄 것이 없는 노출 선언(예: 서브시스템으로 넘기기만 하는 포워더)은 **툴팁을 달지 않아도 된다.**
이때도 구현자용 메모는 지우지 않고 정의부로 옮긴다.

```cpp
// EWEPlayerController.h
UFUNCTION(BlueprintCallable, Category = Inventory)
void ToggleInventory();

// EWEPlayerController.cpp
// Delegates to UEWELocalUIManageSubsystem::ToggleInventory.
void AEWEPlayerController::ToggleInventory()
```

노출되지 않는 선언의 같은 종류 메모(`/** Delegates to ... */`)는 헤더에 그대로 둔다. 툴팁이 되지 않으므로 옮길 이유가 없다.

빈 툴팁 블록(클래스 마법사가 만든 `/** \n * \n */`)은 채우거나 지운다.

---

## 5. 한 번만 쓴다

- **선언부(`.h`)에 쓴 설명을 정의부(`.cpp`)에서 반복하지 않는다.**
  정의부에는 구현 안에서만 의미가 있는 이유만 쓴다.
  ```cpp
  // ❌ EWEAttributeBase.h and EWEAttributeBase.cpp carry the same sentence
  // Clamp the aggregated current value only; base value and modifiers are preserved (non-destructive).
  ```
- **같은 이유가 여러 곳에서 반복되면 관문에 한 번만 쓴다.**
  관문 = 가드, 공통 헬퍼, 단일 진입점의 선언부. 호출부에서는 생략한다.
  - "슬롯 쓰기는 서버 관문을 거쳐야 한다" → `AssignToQuickSlot_ServerOnly()` 선언부에 한 번. 호출부마다 `// Route through the server gate ...` 을 달지 않는다
  - 같은 가드 이유(`// Unsized is the expected state ...`)를 함수마다 반복하지 않는다
- **`CLAUDE.md` 의 규칙을 코드로 옮겨 적지 않는다.** 규칙은 문서에 있고, 코드에는 그 규칙의 관문에만 짧게 남긴다.

---

## 6. Epic 템플릿 주석

ThirdPerson 등 템플릿에서 넘어온 주석도 §1 기준을 따른다. 대부분은 뜻풀이이므로 지운다.
다만 **엔진 쪽 이유나 작업 방식을 설명하는 주석은 그대로 둔다.**

| 처리 | 예 (`EWECharacter.cpp`) |
|------|-------------------------|
| 제거 | `// Set size for collision capsule`, `// Create a follow camera`, `// Jumping`, `// get forward vector` |
| 보존 | `// Note: The skeletal mesh and anim blueprint references ... (to avoid direct content references in C++)` |
| 보존 | `// Don't rotate when the controller rotates. Let that just affect the camera.` |

파일 첫 줄의 저작권 고지(`// Copyright ...`, `// Fill out your copyright notice ...`)는 정리 대상이 아니다.

---

## 7. TODO

일반 `// TODO:` 는 허용한다. **무엇을, 왜** 해야 하는지, 가능하면 **끝나는 조건**까지 쓴다.
`EWECharacter.cpp` 의 `GetActorLabel()` TODO 가 좋은 예다 — 무엇이 문제인지, 언제 무엇으로 바꾸는지가 다 있다.

`@@@` 마커와는 다르다. `@@@` 는 사용자 개인 마커다 (§8).

---

## 8. `@@@` 영역

`@@@` 는 사용자가 **커밋 전에 직접 해야 할 작업**을 적어 두는 개인 마커다. 정의와 커밋 정책은 `CLAUDE.md` 를 따른다.
주석 정리 관점에서는 다음만 지킨다.

- 마커 줄과 그 아래 이어지는 주석 블록은 **정리 대상이 아니다.** 뜻풀이처럼 보여도 건드리지 않는다
- 새 `@@@` 마커는 사용자가 **TODO 추가를 요청했을 때만** 쓴다. 임의로 만들지 않는다
- 마커 제거는 `CLAUDE.md` 대로, 마커가 가리키는 작업을 완료했을 때만

---

## 9. 체크리스트

코드를 쓰거나 고친 뒤:

- [ ] 각 주석을 지웠을 때 사라지는 정보가 있는가? (툴팁 제외)
- [ ] 같은 내용이 선언부나 다른 호출부에 이미 있지 않은가?
- [ ] 에디터 · BP 에 노출된 선언에 `/** */` 툴팁이 있는가?
- [ ] 툴팁에 구현자용 설명이 섞이지 않았는가?
- [ ] 주석 처리된 코드를 남기지 않았는가?
- [ ] 코드를 바꿨다면 근처 주석이 여전히 사실인가?
- [ ] `@@@` 영역을 건드리지 않았는가?
