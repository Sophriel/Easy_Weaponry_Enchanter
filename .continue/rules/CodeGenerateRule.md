# Code Generation Rules

These rules apply when generating or modifying Unreal Engine C++ code.

---

# Unreal Compliance

- Always preserve UCLASS, USTRUCT, UENUM, GENERATED_BODY.
- Do not remove or alter reflection macros.
- Do not remove UPROPERTY or UFUNCTION specifiers.
- Maintain correct include dependencies.
- Prefer forward declarations where appropriate.
- Use TObjectPtr in UE5 when ownership applies.

---

# GAS Enforcement Rules

- AbilitySystemComponent (ASC) must remain on Character.
- Weapons must never own ASC.
- Do not bypass ASC for gameplay execution.
- Attributes must NEVER be modified directly.
- All stat changes must occur via GameplayEffects.
- Ability activation must follow GAS rules.

Forbidden example:
Health -= Damage

---

# Ability Architecture Rules

- GameplayAbility defines high-level flow only.
- AbilityTasks perform execution steps.
- Abilities must NOT spawn actors directly.
- Actor spawning must be implemented via custom AbilityTasks.
- Maintain separation between Ability and AbilityTask responsibilities.

---

# EWE Weapon–Ability Rules

- Weapons contain TSubclassOf<UGameplayAbility>.
- Weapons must not activate abilities directly.
- Abilities are granted via ASC->GiveAbility().
- Ability handles must be tracked and removed correctly on unequip.
- No persistent runtime state inside Weapon classes.

---

# Networking Rules

- Server-authoritative execution.
- Do not trust client-side state.
- Do not introduce custom replication bypassing GAS.
- Ability grant and activation must occur on server unless explicitly designed.

---

# Continue Agent Instructions

Answer strictly based on retrieved code snippets.
If the snippet is not present, say you cannot find it.
Cite file paths when possible.

## Strict File Grounding Rules

When asked to explain a file:
- Only use retrieved file content.
- If the file content is not found in the index, explicitly state:
  "File content not retrieved from index."
- Do NOT assume Unreal Engine default patterns.
- Do NOT fabricate method implementations.
- Do NOT infer missing logic.

If retrieval is incomplete, ask for clarification instead of guessing.

When generating or modifying code:
- Always generate Unreal-compliant C++.
- Preserve all reflection macros.
- Do not rewrite entire files unless required.
- Modify only requested sections.
- Do not remove UPROPERTY specifiers.
- Do not bypass ASC.
- Never modify attributes directly.
- Respect server authority rules.
- Maintain separation between Ability and AbilityTask.
- Preserve weapon-granted ability architecture.

If uncertain, follow existing architectural patterns rather than introducing new systems.

When applying edits:
- Never remove Unreal macros.
- Do not rewrite entire files.
- Modify only requested sections.
- Preserve includes and specifiers.