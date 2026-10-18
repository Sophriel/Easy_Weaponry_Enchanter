# UI / UX

> HUD, 인벤토리, 인챈터 UI의 동작 규칙과 구현 가이드를 다룬다.

---

## HUD

### 퀵슬롯 구조

```
슬롯 수: 가변 — WBP_QuickSlot::MaxSlotSize 가 결정 (C++ 기본값 5, BP 에서 조정)
전 슬롯: 일반 무기

전환 방식:
  마우스 휠 스크롤
  숫자키 1 ~ 0
```

> "슬롯 0: 레코딩 북 고정" 은 아래 레코딩 북 UI 섹션의 **미구현 설계 정책**이다.
> 현재 모든 슬롯이 일반 무기용이다.

### 슬롯 교체 제한

```
현재 장착 무기의 GA 중 하나라도 End/Cancel 미처리 상태이면:
  → 퀵슬롯 전환 입력 무시
  → 피드백 없음 (조용히 차단)

판별:
  ASC에 등록된 현재 무기 GA 핸들 목록을 순회
  IsActive() && !IsEnded() && !IsCancelled() 인 항목이 있으면 차단
```

### 인챈트 목록 툴팁

```
마우스 호버 시 해당 슬롯 무기의 인챈트 DataAsset 이름 목록 표시.
읽기 전용 (호버 중 편집 불가).
```

### HUD 구현 시 주의사항

```
인챈트 발동/종료 상태는 GA 활성 여부로 판별.
GAS의 GameplayAbilitySpec.IsActive() 를 폴링하거나
OnGameplayAbilityActivated / OnGameplayAbilityEnded 델리게이트 구독.

UMG 위젯에서 ASC를 직접 참조하지 말 것.
→ ViewModel 을 통해 간접 참조. UEWEStatusViewModel 이 ASC 를 구독하고
   위젯은 그 ViewModel 에만 바인딩한다.
```

---

## HUD MVVM 구조

> 구조·의존 방향·설계 결정은 `core/architecture.md` 의 "UI 계층 (MVVM)" 절에 있다.
> 여기서는 **HUD 를 만들거나 고칠 때의 실무 규칙**만 다룬다.

### 위젯을 만들 때

세 HUD 위젯(Status / TargetInfo / QuickSlot)은 전부 같은 형태다. 새 HUD 를 추가할 때도 따른다.

```
1. UEWE<이름>ViewModel  : UMVVMViewModelBase   생성
     표시할 값을 FieldNotify 로 노출
     소스(ASC · 인벤토리 등)를 구독해 값 갱신

2. UEWE<이름>  : UUserWidget                   생성
     데이터 멤버를 두지 않는다. 바인딩만 한다

3. UEWELocalUIManageSubsystem 에 Create / Bind 추가
     Create<X>Widget()  — 위젯 생성 → UMVVMView 에 VM 주입 → Bind<X>()
     Bind<X>()          — 데이터 소스 연결. 여러 번 불려도 안전할 것

4. WBP Class Settings 에서 ViewModel 등록
     Creation Type = Manual, 이름은 C++ SetViewModel() 의 FName 과 일치
```

### BP 바인딩 규칙

```
프로퍼티 바인딩 금지 — Config/DefaultEditor.ini 의 PropertyBindingRule=Prevent
→ 반드시 ViewModel 바인딩을 쓴다

바인딩 대상은 프로퍼티보다 BP 함수를 권장
→ SetHighlighted(bool) 같은 함수에 꽂아두면
   나중에 사운드·애니메이션을 그 안에 추가할 수 있다

"Execute at Initialization" 을 켜둘 것
→ 꺼져 있으면 첫 입력 전까지 초기 상태가 화면에 반영되지 않는다
```

### 자식 위젯은 스스로 관찰한다

```
❌ 부모 위젯이 자식의 함수를 호출해 상태를 밀어넣는다
✅ 부모는 자식에게 Entry ViewModel 을 1회 주입하고, 자식이 그 VM 을 관찰한다
```

퀵슬롯이 그 예다. `UEWEQuickSlot` 이 `BP_ItemSlot` 마다
`UEWEQuickSlotItemViewModel` 을 주입하고, 이후로는 자식에게 아무것도 하지 않는다.
`WeaponSlots` 배열은 생성·레이아웃용일 뿐 상태 전달 통로가 아니다.

| 표시할 것 | 바인딩 소스 |
|---|---|
| 선택 강조 | `IsSelected` |
| 무기 아이콘 | `GetWeapon()` |
| 무기 이름 | `GetWeaponName()` |
| 빈 슬롯 처리 | `IsEmpty()` |

> ⚠️ **`IsSelected` 는 슬롯 변경 1회당 두 번 발화한다** — 이전 슬롯에 `false`,
> 새 슬롯에 `true`. 하이라이트는 이게 맞는 동작이지만, 전환 사운드를 넣으면
> "딸깍딸깍" 두 번 난다. `bSelected == true` 일 때만 재생하거나,
> 슬롯당이 아니라 **변경당 1회**인 피드백은 부모 쪽에서 처리한다.

### 초기화 순서를 가정하지 말 것

인벤토리 로드 · 폰 빙의 · 위젯 부착은 순서가 보장되지 않는다.

```
❌ "BeginPlay 시점엔 인벤토리가 차 있다"
❌ "위젯이 만들어졌으니 폰도 있다"
✅ 준비되면 알린다 / 받는 쪽이 조건을 판정한다
```

HUD 코드에서 데이터가 비어 있는 상태는 **에러가 아니라 기대 상태**다.
경고 대신 `Verbose` 로 남기고 조용히 넘긴다.

---

## 인벤토리

### 기본 동작

```
Tab 키로 표시/숨기기 토글.
HUD 퀵슬롯에 장착 가능한 아이템(무기, 두루마리 등)을 보관.
```

### 아이템 퀵슬롯 지정

```
1. 인벤토리에서 아이템 클릭 → 선택 상태
2. 숫자키 입력 → 해당 퀵슬롯에 지정
```

### 인벤토리 입력 차단 조건

```
Time Stop 또는 Freeze 상태:
  → 인벤토리 UI 조작 자체는 허용
  → 단, UI를 통해 캐릭터에 변화를 주는 행위는 차단
    (예: 아이템 사용, 퀵슬롯 교체 등)

Possess Object 빙의 중:
  → 인벤토리 UI 조작 전체 차단
```

---

## 인챈터 UI

### 역할

```
무기에 인챈트(DataAsset)를 부여하고 슬롯을 관리하는 전용 UI.
두루마리 아이템을 무기에 적용.
미리보기 기능 없음.
```

### 슬롯 편집 제한

```
해당 무기가 현재 장착(퀵슬롯에 활성화)된 상태이면:
  → 인챈트 슬롯 편집 차단
  → 무기를 인벤토리로 이동하거나 다른 무기로 교체 후 편집 가능

이 제한은 서버에서도 동일하게 검증.
클라이언트에서만 차단하면 안 됨.
```

### 두루마리 부여 흐름

```
1. 인챈터 UI에서 두루마리 아이템 선택
2. 대상 무기 슬롯에 적용 버튼 클릭
3. 서버에서 슬롯 수 초과 여부 확인
   → 슬롯 초과 시: 앞에서부터 가능한 만큼만 부여 (Warning 없음)
4. 원본 두루마리 소모 없음 (재사용 가능)
```

### 인챈터 UI 구현 시 주의사항

```
DataAsset 변경은 반드시 서버를 통해 처리.
클라이언트에서 UEWEWeaponData을 직접 수정하면 안 됨.
→ Server RPC로 변경 요청 → 서버에서 처리 → 복제로 클라이언트 반영.
```

---

## Possess Object 중 UI

### 붉은 테두리 효과

```
원본 캐릭터 피격 시:
  → 클라이언트 화면 전체에 붉은 테두리 PostProcess 효과 활성화
  → 빙의 해제 전까지 유지 (중복 피격 시 중복 적용 없음)
  → 빙의 해제 시 즉시 제거

구현:
  PlayerController.Client_NotifyHitWhilePossessing() 수신 시
  → HUD 위젯의 HitBorderWidget 활성화
  → bIsShowingHitBorder 플래그로 중복 활성화 방지
```

### 빙의 중 UI 차단

```
빙의 중 모든 기존 HUD/인벤토리/인챈터 UI 입력 차단.
붉은 테두리 위젯 및 빙의 해제 버튼만 표시/동작.

구현:
  PlayerController에서 InputMode를 GameOnly로 설정.
  빙의 해제 버튼은 별도 독립 위젯으로 항상 최상단에 표시.
```

---

## 레코딩 북 UI

### 레코딩 상태 표시

```
Recording GE가 활성 상태이면:
  → 슬롯 0 아이콘에 녹화 중 인디케이터 표시 (예: 빨간 점 애니메이션)
  → 현재 기록된 GA 수 / 최대 슬롯 수 표시 (예: 3 / 8)
```

### 레코딩 종료 피드백

```
슬롯 초과 자동 종료 시:
  → 화면에 간략한 토스트 메시지: "Recording complete (slot limit reached)"
  → 두루마리 생성 알림

정상 종료 (LMB 재입력) 시:
  → 두루마리 아이템 생성 알림
  → 인벤토리에 두루마리 추가됨을 시각적으로 표시
```
