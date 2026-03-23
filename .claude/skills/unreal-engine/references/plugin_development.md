# Plugin Development Guide

## Overview

UE 플러그인을 직접 개발할 때의 모듈 구조, Build.cs 설정, 에디터/런타임 분리 패턴을 다룬다.

> **기존 플러그인을 프로젝트에 도입하여 사용하는 경우** → `references/plugin_guidance.md`를 참조.

---

## Module Structure

### Runtime + Editor 분리 패턴

대부분의 게임플레이 플러그인은 최소 2개 모듈로 구성한다.

```
MyPlugin/
├── MyPlugin.uplugin
├── Source/
│   ├── MyPluginRuntime/          ← 게임에 포함되는 핵심 로직
│   │   ├── Public/
│   │   ├── Private/
│   │   └── MyPluginRuntime.Build.cs
│   └── MyPluginEditor/           ← 에디터 전용 (커스텀 디테일 패널, 툴 등)
│       ├── Public/
│       ├── Private/
│       └── MyPluginEditor.Build.cs
```

### .uplugin Modules 배열

```json
{
  "FileVersion": 3,
  "Version": 1,
  "VersionName": "1.0",
  "FriendlyName": "My Plugin",
  "Description": "",
  "Category": "Gameplay",
  "CreatedBy": "",
  "CanContainContent": true,
  "Modules": [
    {
      "Name": "MyPluginRuntime",
      "Type": "Runtime",
      "LoadingPhase": "Default"
    },
    {
      "Name": "MyPluginEditor",
      "Type": "Editor",
      "LoadingPhase": "Default"
    }
  ],
  "Plugins": [
    {
      "Name": "GameplayAbilities",
      "Enabled": true
    }
  ]
}
```

- `Type: "Runtime"` → 패키지 빌드에 포함됨
- `Type: "Editor"` → 에디터에서만 로드, 패키지 빌드에서 자동 제외
- `Plugins` 배열 → 이 플러그인이 의존하는 다른 플러그인 목록

### Module Type 정리

| Type | 에디터 로드 | 패키지 빌드 포함 | 사용 시점 |
|------|------------|----------------|----------|
| `Runtime` | O | O | 게임 핵심 로직 |
| `RuntimeNoCommandlet` | O (커맨들릿 제외) | O | 커맨들릿 환경 제외 필요 시 |
| `Editor` | O | X | 에디터 전용 툴, 커스텀 디테일 패널 |
| `EditorNoCommandlet` | O (커맨들릿 제외) | X | 에디터 전용 + 커맨들릿 제외 |
| `UncookedOnly` | O | X | 쿠킹 전 데이터만 필요한 경우 |

---

## Build.cs 설정

### Runtime 모듈

```csharp
// MyPluginRuntime.Build.cs
using UnrealBuildTool;

public class MyPluginRuntime : ModuleRules
{
    public MyPluginRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "EnhancedInput"
        });
    }
}
```

### Editor 모듈 (Runtime에 의존)

```csharp
// MyPluginEditor.Build.cs
using UnrealBuildTool;

public class MyPluginEditor : ModuleRules
{
    public MyPluginEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "UnrealEd",             // 에디터 프레임워크
            "PropertyEditor",       // 디테일 패널 커스터마이징
            "MyPluginRuntime"       // 자신의 Runtime 모듈
        });
    }
}
```

### Public vs Private Dependency

| 종류 | 의미 | 사용 시점 |
|------|------|----------|
| `PublicDependencyModuleNames` | 이 모듈의 Public 헤더에 해당 모듈 타입이 노출됨 | 외부 모듈이 이 모듈을 사용할 때도 해당 의존이 필요한 경우 |
| `PrivateDependencyModuleNames` | 이 모듈의 Private 구현에서만 사용 | 내부 구현에만 필요한 경우 (빌드 시간 절약) |

판단 기준: 해당 모듈의 타입이 **Public 헤더의 함수 시그니처나 멤버 변수에 등장**하면 Public, **cpp 파일에서만 사용**하면 Private.

```csharp
// UEnhancedInputComponent가 Public 헤더에 노출됨 → Public
PublicDependencyModuleNames.Add("EnhancedInput");

// JSON 파싱이 cpp에서만 사용됨 → Private
PrivateDependencyModuleNames.Add("Json");
PrivateDependencyModuleNames.Add("JsonUtilities");
```

---

## WITH_EDITOR 매크로

Runtime 모듈 내에서도 에디터 전용 코드가 필요한 경우:

```cpp
// 에디터에서 프로퍼티 변경 시 유효성 검사
void UMyDataAsset::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);

#if WITH_EDITOR
    // 에디터에서만 실행되는 유효성 검증
    if (Event.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMyDataAsset, SomeField))
    {
        ValidateData();
    }
#endif
}
```

`WITH_EDITOR`로 감싼 코드는 패키지 빌드에서 컴파일 자체가 제외된다.

### 주의: WITH_EDITOR vs WITH_EDITORONLY_DATA

| 매크로 | 용도 |
|--------|------|
| `WITH_EDITOR` | 에디터 빌드에서만 컴파일되는 코드 블록 |
| `WITH_EDITORONLY_DATA` | 에디터 전용 데이터 멤버 (쿠킹 시 제거됨) |

```cpp
#if WITH_EDITORONLY_DATA
    UPROPERTY(EditDefaultsOnly, Category = "Debug")
    FString DebugDescription;  // 패키지 빌드에서 이 멤버 자체가 사라짐
#endif
```

---

## IModuleInterface

각 모듈은 `IModuleInterface`를 구현하여 로드/언로드 시점에 로직을 추가할 수 있다.

```cpp
// MyPluginRuntime.h
#pragma once
#include "Modules/ModuleManager.h"

class FMyPluginRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
```

```cpp
// MyPluginRuntime.cpp
#include "MyPluginRuntime.h"

void FMyPluginRuntimeModule::StartupModule()
{
    // 모듈 로드 시 초기화 (서브시스템 등록, 로그 카테고리 초기화 등)
}

void FMyPluginRuntimeModule::ShutdownModule()
{
    // 모듈 언로드 시 정리
}

IMPLEMENT_MODULE(FMyPluginRuntimeModule, MyPluginRuntime)
```

Editor 모듈에서 디테일 패널 커스터마이징을 등록하는 전형적 패턴:

```cpp
void FMyPluginEditorModule::StartupModule()
{
    FPropertyEditorModule& PropertyModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    PropertyModule.RegisterCustomClassLayout(
        "MyDataAsset",
        FOnGetDetailCustomizationInstance::CreateStatic(&FMyDataAssetDetails::MakeInstance));
}

void FMyPluginEditorModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyModule =
            FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

        PropertyModule.UnregisterCustomClassLayout("MyDataAsset");
    }
}
```

---

## 외부 프로젝트에서 플러그인 사용

다른 프로젝트가 이 플러그인을 사용하려면:

1. 플러그인 폴더를 프로젝트의 `Plugins/` 디렉토리에 배치
2. `.uproject` Plugins 배열에 추가:

```json
{
  "Plugins": [
    {"Name": "MyPlugin", "Enabled": true}
  ]
}
```

3. 프로젝트 Build.cs에 Runtime 모듈 의존성 추가:

```csharp
PublicDependencyModuleNames.Add("MyPluginRuntime");
```

---

## 플러그인 Content 디렉토리

`"CanContainContent": true`로 설정하면 플러그인 자체 Content 폴더를 가질 수 있다.

```
MyPlugin/
├── Content/           ← 플러그인 전용 에셋 (블루프린트, 데이터 테이블 등)
│   └── Data/
├── Source/
└── MyPlugin.uplugin
```

플러그인 Content 내 에셋 경로는 `/PluginName/` 프리픽스로 참조된다.
