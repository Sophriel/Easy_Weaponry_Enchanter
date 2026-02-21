# Project Structure (Indexing Reference)

Unreal Engine 5.5 C++ project.

Primary systems:
- Gameplay Ability System (GAS)
- EasyWeaponryEnchanter (EWE) plugin

---

# Root Directories

- /Source/
- /Plugins/EasyWeaponryEnchanter/

---

# Plugin Root

Plugins/EasyWeaponryEnchanter/Source/EasyWeaponryEnchanter/

Primary subdirectories:

- Ability/        → GameplayAbility implementations
- AbilityTask/    → Custom AbilityTask implementations
- Weapon/         → Weapon classes

Only reference directories that physically exist in the project.

---

# Class Prefix Rules

GameplayAbility classes:
- UEWEGA_

AbilityTask classes:
- UEWEAT_

These prefixes uniquely identify plugin gameplay logic.

---

# File–Class Mapping Rule

Class names include Unreal prefix (U).
File names do NOT include the leading "U".

Example:

Class:
UEWEGA_FireProjectile

Files:
EWEGA_FireProjectile.h
EWEGA_FireProjectile.cpp

Mapping rule:
Remove leading "U" from class name to get file name.

This rule applies consistently across the plugin.

---

# GAS Location Anchors

- AbilitySystemComponent is implemented on Character (inside /Source/)
- Weapons are located in /Weapon/
- Abilities are located in /Ability/
- AbilityTasks are located in /AbilityTask/

---

# Retrieval Keywords

- UEWEGA_
- UEWEAT_
- EasyWeaponryEnchanter
- GameplayAbility
- AbilityTask
- AbilitySystemComponent
- GiveAbility
- SpawnActor
- ActivateAbility
- GameplayEffect
- AttributeSet