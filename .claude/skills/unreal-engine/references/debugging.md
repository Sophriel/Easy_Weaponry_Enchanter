# Debugging

## Overview

This reference covers the core debugging tools available in Unreal Engine C++ development. It is organized into five areas:

- **UE_LOG**: Structured runtime logging with custom categories and verbosity control
- **Screen Messages**: Lightweight in-editor feedback via `GEngine`
- **Visual Logger**: Spatial and timeline-based debugging, primarily for AI and physics
- **Console Commands**: In-game profiling and state inspection commands
- **Crash & Assert Patterns**: Defensive coding tools (`check`, `ensure`, `checkf`) for catching failures early

Load this reference when setting up logging infrastructure, diagnosing runtime behavior, or profiling performance.

---

## UE_LOG — Structured Logging

Define a custom log category per module to enable contextual filtering:

```cpp
// MyGame.h (or top of .cpp for static categories)
DECLARE_LOG_CATEGORY_EXTERN(LogMyGame, Log, All);

// MyGame.cpp
DEFINE_LOG_CATEGORY(LogMyGame);

// Usage
UE_LOG(LogMyGame, Log,     TEXT("Initialized: %s"), *GetName());
UE_LOG(LogMyGame, Warning, TEXT("Health is low: %f"), CurrentHealth);
UE_LOG(LogMyGame, Error,   TEXT("HealthComp is null on %s"), *GetName());
```

**Log Verbosity Levels** (in order of severity):

| Level | Use Case |
|-------|----------|
| `VeryVerbose` | Trace-level spam (disable in production) |
| `Verbose` | Detailed flow tracing |
| `Log` | General info |
| `Warning` | Recoverable unexpected state |
| `Error` | Non-fatal failure |
| `Fatal` | Crash immediately |

---

## Screen Messages

Fast feedback during play-in-editor. Disappears on its own — not persisted to log:

```cpp
// Key = -1 means new unique message each time
// Key = any int means overwrite the same line (useful for per-frame values)
if (GEngine)
{
    GEngine->AddOnScreenDebugMessage(-1,  5.f, FColor::Red,    TEXT("Died!"));
    GEngine->AddOnScreenDebugMessage( 1, 0.f, FColor::Yellow,
        FString::Printf(TEXT("Health: %f"), CurrentHealth));
}
```

---

## Visual Logger

Extremely useful for AI and spatial debugging. Records events with position, so you can scrub through them in the editor like a timeline.

```cpp
#include "VisualLogger/VisualLogger.h"

// Log a sphere at an actor's location
UE_VLOG_SPHERE(this, LogMyGame, Verbose,
    GetActorLocation(), 50.f, FColor::Green, TEXT("Detection Radius"));

// Log a line
UE_VLOG_SEGMENT(this, LogMyGame, Verbose,
    StartPos, EndPos, FColor::Red, TEXT("Patrol Path"));
```

Enable in editor: **Window → Visual Logger**, then Play.

Implement `IVisualLoggerDebugSnapshotInterface` for richer per-frame state snapshots.

---

## Useful Console Commands

```
# Show GC stats
stat gc

# Show memory
stat memory

# Show ability system state (requires GAS)
showdebug abilitysystem

# Dump all loaded objects of a class
obj list class=MyClass

# Force GC
gc.collect
```

---

## Crash & Assert Patterns

```cpp
// Hard assert — crashes in dev, skips in shipping
check(HealthComp != nullptr);

// Soft assert — logs and returns
if (!ensure(HealthComp)) return;

// Always-on check (even in shipping)
checkAlways(bIsInitialized);

// Custom message
checkf(Health > 0.f, TEXT("Health must be positive, got %f"), Health);
```
