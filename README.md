# Easy Weaponry Enchanter

C++ Plugin for Unreal Engine 5.5

A GAS-based enchant system that assigns **Trigger → Condition → Function** combinations to weapons.
Not a skill authoring tool — an **event combinator**.

## Requirements

- `GameplayAbilities` plugin
- `GameplayTags` plugin
- `EnhancedInput` plugin

## Features

- **GAS Abstraction Layer**: Abstracts a Trigger → Condition → Function pipeline on top of GAS. One enchant maps to one GA, one Function maps to one AbilityTask.
- **DataAsset-Driven Design**: Enchant definitions are composed entirely through DataAssets, not code. Blueprint-friendly.
- **Async Asset Loading**: Weapons, UI, and other game assets are loaded asynchronously when needed. Each consumer holds a strong reference, delegating lifetime management to GC.
- **Event-Driven Inventory**: Weapon acquisition, removal, and synchronization through delegates. Components communicate via events with no direct dependencies.
- **Plugin Module Split**: Strict separation between EWERuntime and EWEEditor via `WITH_EDITOR`.

## In Progress

- **Server-Authoritative Architecture**: All enchant and GA activations execute only after server confirmation. No client prediction.
- **Extensible Function Interface**: Add custom Functions by implementing `IEnchantFunction` and overriding `ExecuteFunction()`. C++ extensions are automatically exposed to Blueprint.
- **8 Trigger Types**: OnSpawn, OnAttack, OnHit, OnKill, OnDeath, OnInput, OnCombo, OnCollision.
- **Composable Conditions**: Multiple conditions combined with AND logic — HP ratio, count limits, GameplayTag filters, preceding enchant end, and more.
- **4 Event Types**: Single, Continuous, Sustained, Charged.
- **Built-in Functions**: Creation, Ejection, Rotation, Attach, TimeStop, Freeze, Teleport, Possess, Weather, StatusEffect.
- **Recording Book & Scroll**: Records GA activations into scroll items that can be repeatedly assigned to weapons.
- **JSON Export/Import**: Save and share enchant configurations as JSON files.

## AI Cooperation

- `Continue` extension for VSCode
- `Claude Code` MD files included