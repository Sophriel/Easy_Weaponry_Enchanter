# Enhanced Input System Reference

## Overview

Enhanced Input is UE5's data-driven input system that replaces the legacy input system. Load this reference when working with input binding, mapping contexts, or input actions.

## Core Concepts

### Input Actions (UInputAction)

Data assets that represent logical player actions like "Jump", "Move", or "Shoot".

**Value Types**:
- `Digital (bool)` - Simple button press (e.g., Jump)
- `Axis1D (float)` - Single axis (e.g., Throttle: -1.0 to 1.0)
- `Axis2D (FVector2D)` - Two axes (e.g., Movement: X=strafe, Y=forward)
- `Axis3D (FVector)` - Three axes (e.g., Flying: X, Y, Z)

### Input Mapping Contexts (UInputMappingContext)

Containers that map physical inputs (keys, buttons) to Input Actions. Multiple contexts can be active simultaneously with different priorities.

**Adding contexts at runtime**:
```cpp
#include "EnhancedInputSubsystems.h"

void AMyCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            // Add mapping context with priority (higher = more important)
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}
```

**Removing contexts**:
```cpp
Subsystem->RemoveMappingContext(MappingContextToRemove);
```

## Binding Input Actions in C++

### In Character/Pawn Class

**Template for Enhanced Input binding**:

```cpp
#include "EnhancedInputComponent.h"

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    
    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // VERIFY these asset references exist before binding
        EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
        EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyCharacter::Look);
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AMyCharacter::Jump);
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMyCharacter::StopJumping);
    }
}
```

**Header declarations**:

```cpp
// Always use BlueprintReadOnly — input actions should not be overwritten from BP
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
UInputAction* MoveAction;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
UInputAction* LookAction;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
class UInputMappingContext* DefaultMappingContext;
```

### Callback Function Signatures

**For different value types**:

```cpp
// Digital (bool) - receives FInputActionValue
void Jump(const FInputActionValue& Value);

// Axis1D (float)
void Accelerate(const FInputActionValue& Value);

// Axis2D (FVector2D)
void Move(const FInputActionValue& Value);

// Axis3D (FVector)
void Fly(const FInputActionValue& Value);
```

**Extracting values**:
```cpp
void AMyCharacter::Move(const FInputActionValue& Value)
{
    // For Axis2D
    const FVector2D MovementVector = Value.Get<FVector2D>();
    
    // For Digital/bool
    const bool bIsPressed = Value.Get<bool>();
    
    // For Axis1D
    const float AxisValue = Value.Get<float>();
}
```

## Trigger Events

When binding actions, specify when the callback fires:

- `ETriggerEvent::Started` - Input just started (initial press)
- `ETriggerEvent::Ongoing` - Input is actively held/moved
- `ETriggerEvent::Triggered` - Input meets trigger conditions (most common)
- `ETriggerEvent::Canceled` - Input was interrupted
- `ETriggerEvent::Completed` - Input finished (button released)

**Common usage patterns**:
```cpp
// Button press - fire once
EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AMyCharacter::Interact);

// Continuous input - fire every frame while held
EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);

// Button release
EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMyCharacter::StopJumping);
```

## Input Modifiers

Modify raw input values before they reach your code. Applied in the Input Mapping Context editor.

**Common modifiers**:
- `Negate` - Invert the input value
- `Dead Zone` - Create dead zones for analog sticks
- `Scalar` - Multiply input by a value
- `Smooth` - Smooth out input over time
- `Response Curve` - Apply custom response curves

## Input Triggers

Determine when an action fires. Applied in the Input Mapping Context editor.

**Common triggers**:
- `Pressed` - Fire once on initial press
- `Released` - Fire once on release
- `Hold` - Require holding for duration
- `Tap` - Quick press and release
- `Chord` - Require multiple inputs simultaneously

## Common Patterns

### Movement with Enhanced Input

```cpp
// Implementation
void AMyCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();
    
    if (Controller != nullptr)
    {
        // Forward/backward movement
        AddMovementInput(GetActorForwardVector(), MovementVector.Y);
        
        // Left/right movement
        AddMovementInput(GetActorRightVector(), MovementVector.X);
    }
}

void AMyCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
    
    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}
```

### Conditional Context Switching

```cpp
void AMyCharacter::EnterAimingMode()
{
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem())
    {
        // Remove default context
        Subsystem->RemoveMappingContext(DefaultMappingContext);
        
        // Add aiming context (different key bindings)
        Subsystem->AddMappingContext(AimingMappingContext, 1); // Higher priority
    }
}

void AMyCharacter::ExitAimingMode()
{
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem())
    {
        Subsystem->RemoveMappingContext(AimingMappingContext);
        Subsystem->AddMappingContext(DefaultMappingContext, 0);
    }
}
```

## Debugging Enhanced Input

### Console Commands

```
showdebug enhancedinput  // Show active contexts and triggered actions
```

### Common Issues

**Input not working**:
1. Check mapping context is added in BeginPlay
2. Verify Input Actions are assigned in Blueprint/Details panel
3. Ensure Enhanced Input plugin is enabled
4. Check that UEnhancedInputComponent is used (not base UInputComponent)

**Wrong trigger firing**:
- Review ETriggerEvent type (Started vs Triggered vs Completed)
- Check Input Triggers in the Mapping Context

## Asset Naming Conventions

**Common patterns** (verify with project):
- Input Actions: `IA_ActionName` (e.g., `IA_Jump`, `IA_Move`)
- Mapping Contexts: `IMC_ContextName` (e.g., `IMC_Default`, `IMC_Vehicle`)
But ALWAYS verify - projects use different naming conventions.

| Asset | Value Type | Common Names |
|-------|-----------|--------------|
| Move | Axis2D | `IA_Move`, `IA_Movement` |
| Look | Axis2D | `IA_Look`, `IA_Aim` |
| Jump | Boolean | `IA_Jump` |
| Interact | Boolean | `IA_Interact` |

**NEVER assume input action names**. Always discover them first:

```bash
# Find Input Actions in Content
find Content -type f \( -name "IA_*.uasset" -o -name "*InputAction*.uasset" \)

# Find Input Mapping Contexts
find Content -type f \( -name "IMC_*.uasset" -o -name "*MappingContext*.uasset" \)
```

## .uasset Files and Blueprint Reading

**.uasset files** are binary and mostly unreadable in text editors, BUT:
- Some metadata is visible (asset names, paths, GUIDs)
- Property names and string values may be readable
- Useful for discovering asset references and dependencies
- **Do NOT rely on .uasset contents for implementation details**

**Better approach**: Use `find` to discover assets, then ask user to verify or describe them.

---

## Required Module Dependencies

In `YourProject.Build.cs`:

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", 
    "CoreUObject", 
    "Engine", 
    "InputCore",
    "EnhancedInput"  // ← Required
});
```

## Migration from Legacy Input

**Legacy system**:
```cpp
// Old way (UE4)
PlayerInputComponent->BindAxis("MoveForward", this, &AMyCharacter::MoveForward);
```

**Enhanced Input**:
```cpp
// New way (UE5)
if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
{
    EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
}
```
