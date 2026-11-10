# UI / UX

> HUD, 인벤토리, 인챈터 UI의 동작 규칙과 구현 가이드를 다룬다.

---

## HUD

### 퀵슬롯 구조

```
슬롯 수: UEWEInventoryComponent::QuickSlotCount (기본 5, 개발자 설정, 복제하지 않음)
         위젯은 VM 의 슬롯 수(GetSlotCount)로 슬롯 위젯을 만든다
전 슬롯: 일반 무기

전환 방식:
  마우스 휠 스크롤
  숫자키 1 ~ 0
  선택은 서버 권한 — 입력 → RPC → OnRep_SelectedQuickSlotIndex
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

> 계층 · 의존 방향 · 핵심 원칙은 `core/architecture.md` 의 "UI 계층 (MVVM)" 절에 있다.
> 여기서는 ViewModel 생성 · 주입 절차와 **HUD 를 만들거나 고칠 때의 실무 규칙**을 다룬다.

### ViewModel 생성 · 주입

```cpp
// UEWELocalUIManageSubsystem::CreateQuickSlotWidget
CreateQuickSlotViewModel();   // NewObject, 이미 있으면 반환
```

- **생성은 위젯 생성 함수에서 1회.** `Create<X>Widget()` 이 VM 을 만들고, 현재 소스를
  pull 해 바인딩한 뒤 위젯에 주입한다. `Bind*()` 안에 두지 않는다 — `Bind*()` 는 여러
  경로에서 호출되고 조기 반환 분기가 많아, 생성이 아예 일어나지 않거나 두 번 일어나는
  창이 생긴다.
- VM 은 **위젯보다 오래 산다.** 위젯을 재생성해도 VM 의 Model 바인딩이 유지된다.
- 소스 변경 경로(Model 변경 · 빙의 · 타겟 갱신)는 VM 이 아직 없으면 스킵한다. VM 이 생기면 위젯 생성 함수의 pull 로 현재 소스에 바인딩된다.
- VM 생성에 실패하면(`ensure` + Error 로그) 위젯 생성 함수는 pull · 바인딩 · 주입을 모두 건너뛴다.

**주입:** 서브시스템은 VM 객체만 `SetViewModel(VM)` 으로 넘긴다. 각 위젯은 자기
`UMVVMView` 에 자기 이름(`FName`) 으로 바인딩한다. 상위 위젯(`UEWEQuickSlot`) 은 슬롯
위젯(`UEWESlotItem`) 을 만들고 아이템 VM 을 넘기며, 슬롯 위젯이 스스로 바인딩한다.

**슬롯 위젯 생성:** `UEWEQuickSlot` 은 VM 의 `GetSlotCount` 변경을 구독해 슬롯 위젯을 만든다. `UEWEInventory` 도 같은 구독을 갖지만 인벤토리 그리드의 슬롯 위젯 생성은 미구현이다.

**TargetInfo:** 서브시스템은 VM 만 갱신한다 (`UpdateTargetName` · `UpdateTargetInfo` ·
`ClearTargetInfo`). 보임 여부는 위젯이 VM 의 `HasTarget` 바인딩으로 정한다.

**Model 바인딩:** 세 경로로 들어온다.

- `PlayerControllerChanged` → `BindInventoryModel` — PC 의 `OnInventoryModelChanged` 를 구독하고, 이미 빙의된 PC 면 현재 Model 을 pull 한다
- PC `OnInventoryModelChanged(Old, New)` → `HandleInventoryModelChanged` → `UEWEQuickSlotViewModel::BindModel` · `UEWEInventoryViewModel::BindModel` (빙의 시)
- `CreateQuickSlotWidget` · `CreateInventoryWidget` — 현재 Model 을 pull 해 **자기 VM 만** `BindModel` 한다. 다른 VM 을 다시 바인딩하면 인벤토리 VM 의 선택이 풀리기 때문이다

퀵슬롯 VM 은 `OnQuickSlotsChanged` · `OnSelectedQuickSlotChanged` 를, 인벤토리 VM 은 `OnItemSlotChanged` 를 구독하고, 각자 `UpdateFromModel` 로 초기 상태를 채운다. 인벤토리 VM 은 새 Model 에 바인딩할 때 선택을 해제한다.

### create-then-bind 패턴

네 위젯(Status · TargetInfo · QuickSlot · Inventory)이 같은 형태를 따른다. 어느 쪽이 먼저 준비되든 결과가 같다.

```
Create<X>Widget()        →  VM 생성 → 현재 소스 pull · 바인딩 → VM 주입   ← 위젯이 준비된 경로
AcknowledgePossession()  →  Bind<X>()                ← 폰이 준비된 경로
                         →  PC OnInventoryModelChanged → HandleInventoryModelChanged → BindModel
```

`Bind*()` 는 **몇 번 불려도 안전**해야 한다. 이미 채워졌으면 덮어쓰지 않는다.

### 위젯을 만들 때

HUD 위젯(Status / TargetInfo / QuickSlot)과 인벤토리 위젯은 전부 같은 형태다. 새 HUD 를 추가할 때도 따른다.

```
1. UEWE<이름>ViewModel  : UMVVMViewModelBase   생성
     표시할 값을 FieldNotify 로 노출
     소스(ASC · 인벤토리 등)를 구독해 값 갱신

2. UEWE<이름>  : UUserWidget                   생성
     데이터 멤버를 두지 않는다. 바인딩만 한다

3. UEWELocalUIManageSubsystem 에 Create / Bind 추가
     Create<X>Widget()  — 위젯 생성 → VM 생성 → 현재 소스 pull · 바인딩 → VM 주입
     Bind<X>()          — 데이터 소스 연결. 여러 번 불려도 안전할 것
     퀵슬롯 · 인벤토리는 Bind<X> 대신 인벤토리 Model 을 바인딩한다 (위 "Model 바인딩")
     주입 · 표시 상태 규칙은 위 "ViewModel 생성 · 주입" 을 따른다

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
✅ 부모는 슬롯 위젯에 아이템 ViewModel 을 넘기고, 슬롯 위젯이 그 VM 을 관찰한다
```

퀵슬롯이 그 예다. `UEWEQuickSlot` 이 `BP_ItemSlot` (`UEWESlotItem`) 마다
`SetViewModel(UEWESlotItemViewModel)` 을 호출하고, 이후로는 자식에게 아무것도 하지 않는다.
슬롯 위젯은 `SetViewModel` 을 받으면 자기 `UMVVMView` 에 스스로 바인딩한다.
`WeaponSlots` 배열은 생성·레이아웃용일 뿐 상태 전달 통로가 아니다.

| 표시할 것 | 바인딩 소스 |
|---|---|
| 선택 강조 | `IsSelected` |
| 무기 이름 | `GetWeaponName()` |
| 빈 슬롯 처리 | `IsEmpty()` |
| 슬롯 번호 | `GetSlotIndex()` |

> ⚠️ **`IsSelected` 는 슬롯 변경 1회당 두 번 발화한다** — 이전 슬롯에 `false`,
> 새 슬롯에 `true`. 하이라이트는 이게 맞는 동작이지만, 전환 사운드를 넣으면
> "딸깍딸깍" 두 번 난다. `bSelected == true` 일 때만 재생하거나,
> 슬롯당이 아니라 **변경당 1회**인 피드백은 부모 쪽에서 처리한다.

### 초기화 순서를 가정하지 말 것

인벤토리 로드 · 폰 빙의 · 위젯 부착은 순서가 보장되지 않는다 (원칙: `core/architecture.md` "초기화 순서 무관성").
각 축이 알리는 지점과 받는 쪽이 판정하는 지점은 다음과 같다.

```
각 축은 준비되면 알린다        인벤토리 Model → OnQuickSlotsChanged · OnSelectedQuickSlotChanged
                              PC → OnInventoryModelChanged
받는 쪽이 조건을 판정한다      UEWEQuickSlotViewModel::BindModel → UpdateFromModel 로 현재 상태 당겨 옴
                              위젯 → VM GetSlotCount 변경을 구독해 슬롯 생성
```

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

### 구조

```
인벤토리는 고정 크기 슬롯 배열이다.
- 칸 수: InventorySlotCount (개발자 설정, 기본 32)
- 빈칸은 null. 무기를 제거하면 해당 칸만 비워지고, 뒤의 아이템이 앞으로 오지 않는다 (재정렬 없음).
```

### 선택 수명

```
선택은 인벤토리 VM 의 로컬 UI 상태다 (복제 안 함, Model 의 퀵슬롯 선택과 무관).
- 아이템 칸만 선택 가능 (빈칸 · 범위 밖은 무시)
- 해제: 선택 칸의 내용 변경 시 · 새 Model 바인딩 시 · 인벤토리 닫기 시
- 유지: 퀵슬롯 배정 후에도 선택은 유지된다
```

### 아이템 퀵슬롯 지정

```
1. 인벤토리에서 아이템 클릭 → 선택 상태 (인벤토리 VM)
2. 숫자키 입력 → 해당 퀵슬롯에 지정
   배정은 인벤토리 VM → Server_AssignToQuickSlot 로 Model 관문에 모인다
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
수정 대상은 UEWEWeaponInstance (런타임 상태). UEWEWeaponData 는 런타임 불변.
클라이언트에서 직접 수정하면 안 됨.
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
