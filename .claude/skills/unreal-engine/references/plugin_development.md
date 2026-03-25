# Plugin Development Guide

## Overview

Covers module structure, Build.cs configuration, and editor/runtime separation patterns for developing UE plugins.

> **For using existing plugins in your project**, see `references/plugin_guidance.md`.

---

## Module Structure

### Runtime + Editor Split Pattern

Most gameplay plugins consist of at least two modules.

```
MyPlugin/
├── MyPlugin.uplugin
├── Source/
│   ├── MyPluginRuntime/          ← Core logic included in packaged builds
│   │   ├── Public/
│   │   ├── Private/
│   │   └── MyPluginRuntime.Build.cs
│   └── MyPluginEditor/           ← Editor-only (custom detail panels, tools, etc.)
│       ├── Public/
│       ├── Private/
│       └── MyPluginEditor.Build.cs
```

### .uplugin Modules Array

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

- `Type: "Runtime"` → Included in packaged builds
- `Type: "Editor"` → Loaded only in editor, automatically excluded from packaged builds
- `Plugins` array → Other plugins this plugin depends on

### Module Type Reference

| Type | Editor Load | Packaged Build | Use Case |
|------|------------|----------------|----------|
| `Runtime` | O | O | Core game logic |
| `RuntimeNoCommandlet` | O (excl. commandlet) | O | Exclude from commandlet environments |
| `Editor` | O | X | Editor-only tools, custom detail panels |
| `EditorNoCommandlet` | O (excl. commandlet) | X | Editor-only + exclude commandlet |
| `UncookedOnly` | O | X | Pre-cook data only |

---

## Build.cs Configuration

### Runtime Module

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

### Editor Module (depends on Runtime)

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
            "UnrealEd",             // Editor framework
            "PropertyEditor",       // Detail panel customization
            "MyPluginRuntime"       // Own Runtime module
        });
    }
}
```

### Public vs Private Dependency

| Type | Meaning | When to Use |
|------|------|----------|
| `PublicDependencyModuleNames` | Types from this module appear in Public headers | When external modules using this module also need the dependency |
| `PrivateDependencyModuleNames` | Used only in Private implementation | Internal implementation only (reduces build times) |

Rule of thumb: if the module's types appear in **Public header function signatures or member variables**, use Public. If used **only in cpp files**, use Private.

```csharp
// UEnhancedInputComponent exposed in Public header → Public
PublicDependencyModuleNames.Add("EnhancedInput");

// JSON parsing used only in cpp → Private
PrivateDependencyModuleNames.Add("Json");
PrivateDependencyModuleNames.Add("JsonUtilities");
```

---

## WITH_EDITOR Macro

When editor-only code is needed inside a Runtime module:

```cpp
// Validate on property change in editor
void UMyDataAsset::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);

#if WITH_EDITOR
    // Editor-only validation
    if (Event.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMyDataAsset, SomeField))
    {
        ValidateData();
    }
#endif
}
```

Code wrapped in `WITH_EDITOR` is excluded from compilation in packaged builds.

### Note: WITH_EDITOR vs WITH_EDITORONLY_DATA

| Macro | Purpose |
|--------|------|
| `WITH_EDITOR` | Code block compiled only in editor builds |
| `WITH_EDITORONLY_DATA` | Editor-only data members (stripped during cooking) |

```cpp
#if WITH_EDITORONLY_DATA
    UPROPERTY(EditDefaultsOnly, Category = "Debug")
    FString DebugDescription;  // This member is stripped entirely in packaged builds
#endif
```

---

## IModuleInterface

Each module can implement `IModuleInterface` to add logic at load/unload time.

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
    // Initialize on module load (register subsystems, log categories, etc.)
}

void FMyPluginRuntimeModule::ShutdownModule()
{
    // Cleanup on module unload
}

IMPLEMENT_MODULE(FMyPluginRuntimeModule, MyPluginRuntime)
```

Typical pattern for registering detail panel customization in an Editor module:

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

## Using the Plugin from External Projects

For another project to use this plugin:

1. Place the plugin folder in the project's `Plugins/` directory
2. Add to the `.uproject` Plugins array:

```json
{
  "Plugins": [
    {"Name": "MyPlugin", "Enabled": true}
  ]
}
```

3. Add Runtime module dependency in project Build.cs:

```csharp
PublicDependencyModuleNames.Add("MyPluginRuntime");
```

---

## Plugin Content Directory

Setting `"CanContainContent": true` allows the plugin to have its own Content folder.

```
MyPlugin/
├── Content/           ← Plugin-specific assets (Blueprints, DataTables, etc.)
│   └── Data/
├── Source/
└── MyPlugin.uplugin
```

Assets inside plugin Content are referenced with the `/PluginName/` prefix.
