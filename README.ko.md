# Easy Weaponry Enchanter

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5-313131?style=flat&logo=unrealengine&logoColor=white)](https://www.unrealengine.com)
[![Claude Code](https://img.shields.io/badge/Claude%20Code-Integration-D97757?style=flat&logo=anthropic&logoColor=white)](https://www.anthropic.com/claude-code)
[![License](https://img.shields.io/badge/License-Apache--2.0-green?style=flat)](LICENSE)

[English](README.md) | **한국어**

Unreal Engine 5.5용 C++ 플러그인

**Trigger → Condition → Function** 조합을 무기에 할당하는 GAS 기반 인챈트 시스템입니다.
스킬 제작 도구가 아니라 **이벤트 조합기(event combinator)** 입니다.

## 요구 사항

- `GameplayAbilities` 플러그인 — GAS 핵심 프레임워크
- `GameplayTags` 플러그인 — 태그 기반 Condition 및 필터링
- `EnhancedInput` 플러그인 — 입력 바인딩
- `ModelViewViewModel` 플러그인 — UI

## 기능

- **GAS 추상화 계층**: GAS 위에 Trigger → Condition → Function 파이프라인을 추상화합니다. 인챈트 하나는 GA 하나에, Function 하나는 AbilityTask 하나에 대응합니다.
- **DataAsset 기반 설계**: 인챈트 정의를 코드가 아닌 DataAsset만으로 구성합니다. 블루프린트 친화적입니다.
- **비동기 에셋 로딩**: 무기, UI 등 게임 에셋을 필요할 때 비동기로 로드합니다. 각 사용처가 강한 참조(strong reference)를 보유하며, 수명 관리는 GC에 위임합니다.
- **이벤트 기반 인벤토리**: 무기 획득·제거·동기화를 델리게이트로 처리합니다. 컴포넌트는 직접 의존 없이 이벤트로 통신합니다.
- **플러그인 모듈 분리**: `WITH_EDITOR`를 통해 EWERuntime과 EWEEditor를 엄격하게 분리합니다.
- **MVVM UI 아키텍처**: UI 뷰가 `ModelViewViewModel` 플러그인을 통해 ViewModel에 바인딩되어, UMG 위젯과 게임플레이 로직을 분리합니다.

## 개발 중

- **서버 권한 아키텍처**: 모든 인챈트와 GA 활성화는 서버 확인 이후에만 실행됩니다. 클라이언트 예측은 없습니다.
- **확장 가능한 Function 인터페이스**: `IEnchantFunction`을 구현하고 `ExecuteFunction()`을 오버라이드하여 커스텀 Function을 추가할 수 있습니다. C++ 확장은 블루프린트에 자동으로 노출됩니다.
- **8가지 Trigger 유형**: OnSpawn, OnAttack, OnHit, OnKill, OnDeath, OnInput, OnCombo, OnCollision.
- **조합 가능한 Condition**: 여러 Condition을 AND 논리로 결합합니다 — HP 비율, 횟수 제한, GameplayTag 필터, 선행 인챈트 종료 등.
- **4가지 이벤트 유형**: Single, Continuous, Sustained, Charged.
- **내장 Function**: Creation, Ejection, Rotation, Attach, TimeStop, Freeze, Teleport, Possess, Weather, StatusEffect.
- **레코딩 북(Recording Book) & 두루마리(Scroll)**: GA 활성화를 스크롤 아이템으로 기록하여 무기에 반복 할당할 수 있습니다.
- **JSON 내보내기/가져오기**: 인챈트 설정을 JSON 파일로 저장하고 공유할 수 있습니다.

## AI 협업

- `Claude Code` MD 파일 포함
