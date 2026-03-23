# Plugin-Specific Guidance

> **이 문서는 "외부/서드파티 플러그인을 프로젝트에 도입하여 사용할 때"의 가이드입니다.**
> 플러그인을 직접 개발하는 경우 → `references/plugin_development.md`를 참조하세요.

## Checking Enabled Plugins

Always verify plugin availability in `.uproject` before writing plugin-dependent code:

```bash
# Check if a specific plugin is enabled
grep -i "PluginName" *.uproject

# List all enabled plugins
grep -A2 '"Enabled": true' *.uproject
```

---

## Unknown or Experimental Plugins

When encountering unfamiliar plugins (e.g., Mutable, MutableClothing, RelativeIKOp):

### Step 1 — Search Official Documentation

```
web_search: "Unreal Engine [PluginName] documentation API"
web_search: "Unreal Engine [PluginName] [EngineVersion] usage examples"
```

### Step 2 — Check Engine Source (Source Builds Only)

```bash
# Engine plugins location
find /path/to/UE5/Engine/Plugins -type d -name "PluginName"

# Read plugin descriptor
cat /path/to/UE5/Engine/Plugins/PluginName/PluginName.uplugin
```

### Step 3 — Be Transparent

If documentation is sparse, communicate clearly:

> "This plugin is experimental/underdocumented for this engine version. The following is based on available source and community findings — verify before use."

---

## Common Plugin Dependency Patterns

### Adding Plugin Dependency in Build.cs

```csharp
// Required when using plugin types in C++
PrivateDependencyModuleNames.AddRange(new string[] {
    "PluginModuleName"
});
```

### Conditional Compilation by Plugin

```cpp
#if WITH_EDITORONLY_DATA
    // Editor-only plugin code
#endif
```

---

## Plugin Stability Reference

| Status | Meaning |
|--------|---------|
| `Stable` | Production-safe, API unlikely to change |
| `Beta` | Mostly stable, minor API changes possible |
| `Experimental` | API may change significantly; use with caution |
| `EarlyAccess` | Preview only; not for production |

Check plugin status in the `.uplugin` file under `"MarketplaceURL"` or in the Epic documentation.
