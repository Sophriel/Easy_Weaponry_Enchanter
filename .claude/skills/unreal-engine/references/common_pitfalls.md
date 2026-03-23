# Common Pitfalls & Troubleshooting

## Overview

This reference covers common mistakes, debugging strategies, and solutions to frequent Unreal Engine development issues. Load when encountering errors, unexpected behavior, or project setup problems.

## Input System Issues

### Enhanced Input Not Working

**Symptoms**:
- Input actions don't fire
- Character doesn't respond to keyboard/mouse

**Checklist**:
1. **Plugin enabled**: Check `.uproject` has `"EnhancedInput"` plugin enabled
2. **Module dependency**: Verify `EnhancedInput` in `Build.cs`
3. **Mapping context added**: Must call `AddMappingContext` in `BeginPlay`
4. **Input actions assigned**: Check Blueprint Details panel has Input Actions set
5. **Correct component type**: Must cast to `UEnhancedInputComponent`, not base `UInputComponent`
6. **Project settings**: Editor → Project Settings → Input → Default Classes → Enhanced Input Component

**Common mistake**:
```cpp
// WRONG - base component doesn't support Enhanced Input
if (UInputComponent* Input = PlayerInputComponent)
{
    Input->BindAction(MoveAction, ...); // Won't compile
}

// CORRECT
if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
{
    EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
}
```

### Assuming Input Action Asset Names

**Symptom**: Null pointer or compile error when binding actions that don't exist

**Cause**: Hardcoding asset names without verifying they exist in the project

```cpp
// ❌ DON'T assume this asset exists
EnhancedInput->BindAction(IA_Jump, ETriggerEvent::Started, this, &AMyCharacter::Jump);
```

```bash
# ✅ Discover first — search broadly
find Content -name "*IA_*"
find Content -name "IA_*.uasset"
find Content -name "*InputAction*.uasset"
# Results might show: IA_Jump, IA_PlayerJump, InputAction_Jump, etc.
```

After discovery, verify the asset is also **assigned in the Blueprint Details panel** — the asset existing on disk is not enough if the UPROPERTY slot is empty.

### Using Generic Property Names Without Verification

**Cause**: Naming `UPROPERTY` variables before checking the project's actual asset naming convention

```cpp
// ❌ Too generic — what if the project uses IA_PlayerJump?
UPROPERTY(EditAnywhere, Category = "Input")
UInputAction* JumpAction;
```

```bash
# ✅ Discover the actual convention first
find Content/Input -name "*.uasset"
# Then name the UPROPERTY to match discovered asset names
```

### Wrong Trigger Event

**Symptom**: Callback fires too many times or not at all

**Cause**: Using wrong `ETriggerEvent` type

**Solution**:
- `Started` - Once on initial press (good for single-fire actions)
- `Triggered` - Every frame while conditions met (good for movement)
- `Ongoing` - While input active (rarely used)
- `Completed` - Once on release (good for charge-ups)
- `Canceled` - When interrupted

```cpp
// WRONG for continuous movement
EnhancedInput->BindAction(MoveAction, ETriggerEvent::Started, ...);

// CORRECT
EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, ...);
```

## Garbage Collection Issues

### ❌ Raw UObject* Without UPROPERTY

```cpp
// Will be silently collected by GC — no warning, no crash, just disappears
UHealthComponent* HealthComp;
```

### ✅ Always Wrap in UPROPERTY

```cpp
UPROPERTY()
UHealthComponent* HealthComp;
```

---

### ❌ Using nullptr Check for Pending-Kill Objects

```cpp
if (HealthComp != nullptr) { ... } // Object may be pending kill — unsafe
```

### ✅ Use IsValid()

```cpp
if (IsValid(HealthComp)) { ... } // Handles both null and pending kill state
```

---

## Gameplay Ability System Issues

### Ability Not Activating

**Symptoms**:
- `TryActivateAbility` returns false
- Ability logic never executes

**Debug checklist**:
1. **Authority**: Abilities only activate on server
```cpp
if (!HasAuthority())
{
    UE_LOG(LogTemp, Warning, TEXT("No authority - ability won't activate"));
}
```

2. **ASC initialized**: Must call `InitAbilityActorInfo`
```cpp
// In PossessedBy
AbilitySystemComponent->InitAbilityActorInfo(this, this);
```

3. **Ability granted**: Check ability was given via `GiveAbility`
```cpp
// Use showdebug abilitysystem to verify
```

4. **CommitAbility succeeds**: Check costs, cooldowns, tags
```cpp
if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
{
    UE_LOG(LogTemp, Warning, TEXT("CommitAbility failed - check costs/cooldowns/tags"));
    EndAbility(...);
}
```

5. **Tags allow activation**:
```cpp
// Check BlockAbilitiesWithTag and RequiredTags in ability
```

### Attributes Not Replicating

**Symptoms**:
- Client sees wrong attribute values
- UI doesn't update on clients

**Solution**:
```cpp
// In AttributeSet.cpp
void UMyAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    // CRITICAL: Must use DOREPLIFETIME_CONDITION_NOTIFY
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, Health, COND_None, REPNOTIFY_Always);
}

// Must implement OnRep
UFUNCTION()
void OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, Health, OldHealth);
}
```

### GAS Setup on PlayerState vs Character

**Issue**: Confusion about where to place ASC

**For single-player or listen server**:
- ASC on Character is simpler
- Owner = Avatar = Character

**For dedicated server**:
- ASC on PlayerState survives respawns
- Owner = PlayerState, Avatar = Character
- More complex setup but necessary for multiplayer

**Critical initialization**:
```cpp
// If ASC on PlayerState
void AMyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    
    if (AMyPlayerState* PS = GetPlayerState<AMyPlayerState>())
    {
        // Owner is PlayerState, Avatar is Character
        PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
    }
}

void AMyCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    
    if (AMyPlayerState* PS = GetPlayerState<AMyPlayerState>())
    {
        PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
    }
}
```

## Build & Compilation Issues

### Unresolved External Symbol

**Symptom**: Linker error like `unresolved external symbol "class UClass * __cdecl ..."`

**Common causes**:
1. **Missing module dependency** in `Build.cs`
2. **Missing MODULENAME_API** export macro
3. **Forward declaration where include needed**
4. **Implementation missing** for declared function

**Solutions**:
```csharp
// 1. Add missing modules to Build.cs
PublicDependencyModuleNames.AddRange(new string[] {
    "EnhancedInput",  // If using Enhanced Input
    "GameplayAbilities",  // If using GAS
    "UMG"  // If using UI widgets
});
```

```cpp
// 2. Add export macro for cross-module usage
class PROJECTNAME_API AMyCharacter : public ACharacter
{
    // ...
};
```

### Hot Reload Issues

**Symptom**: Changes not visible after compilation, crashes on hot reload

**Best practice**: **Don't use Hot Reload for significant changes**

**Solution**:
1. Close Unreal Editor
2. Delete `Binaries/`, `Intermediate/`, `Saved/` folders
3. Rebuild in IDE (Visual Studio, Rider)
4. Reopen editor

**When Hot Reload is safe**:
- Function body changes (not signature)
- Constant value changes
- Minor logic tweaks

**When to avoid**:
- Adding/removing UFUNCTIONs
- Adding/removing UPROPERTYs
- Changing class inheritance
- Adding new includes

### "Asset Failed to Load" Errors

**Symptom**: Missing assets, pink textures, errors in log

**Causes**:
1. Asset moved/renamed without fixing redirectors
2. Asset deleted while still referenced
3. Blueprint referencing missing C++ class

**Solutions**:
```bash
# Fix redirectors in Content Browser
Right-click Content folder → Fix Up Redirectors in Folder

# Find asset references
Right-click asset → Reference Viewer
```

## Blueprint vs C++ Integration Issues

### ❌ uint32 in Blueprint-Exposed Functions

**Error**: `Type 'uint32' is not supported by blueprint.`

**Cause**: Blueprint only supports signed integer types. `uint32` and other unsigned types are not recognized by the reflection system.

```cpp
// ❌ Causes compile error when exposed to Blueprint
UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
UWeaponData* GetWeapon(uint32 TargetWeapon);
```

```cpp
// ✅ Use int32 instead
UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
UWeaponData* GetWeapon(int32 TargetWeapon);
```

---

### ❌ TObjectPtr in UFUNCTION or Delegate Declarations

**Error**: `UFunctions cannot take a TObjectPtr as a function parameter or return value.`

**Cause**: `TObjectPtr<>` is a smart pointer wrapper for member variables only. The reflection system does not support it in `UFUNCTION` signatures or `DECLARE_DYNAMIC_MULTICAST_DELEGATE` parameter lists. Use raw pointers instead.

```cpp
// ❌ TObjectPtr in UFUNCTION return value
UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
TArray<TObjectPtr<UWeaponData>> GetAllWeapons() const;

// ❌ TObjectPtr in Delegate declaration
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponsSyncedEvent,
    const TArray<UWeaponData*>&, Weapons, int32, NewCount);
// (even if TObjectPtr appears only in related types, avoid it in delegate param lists)
```

```cpp
// ✅ Use raw pointer in UFUNCTION return value
UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
TArray<UWeaponData*> GetAllWeapons() const;

// ✅ Use raw pointer in Delegate declaration
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponsSyncedEvent,
    const TArray<UWeaponData*>&, Weapons, int32, NewCount);
```

> `TObjectPtr<>` is valid **only** as a class member variable (`UPROPERTY`). Never use it in function signatures or delegate declarations.

---

### ⚠️ Misconception: Delegate Macros Always Require Full #include

AI 도구(Claude Code 포함)가 종종 잘못 진단하는 케이스.

**잘못된 진단 예시**:
> "DECLARE_DYNAMIC_MULTICAST_DELEGATE_* 매크로는 리플렉션 정보를 필요로 하므로
> forward declaration만으로는 컴파일 에러가 발생한다. #include가 필요하다."

**실제 동작**:
- 매개변수 타입이 **`UObject` 기반 포인터(`UMyClass*`)** 라면 forward declaration만으로 충분하다
- UHT는 포인터 타입 + 이름 정보만으로 처리 가능하며, 전체 클래스 정의를 요구하지 않는다

```cpp
// ✅ 이것은 정상 동작한다 — #include 불필요
class UEWEWeaponData;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponAcquiredEvent, UEWEWeaponData*, Weapon);
```

`#include`가 실제로 필요한 경우:
- 포인터가 아닌 **값 타입** 매개변수 (e.g. `FMyStruct`)
- 같은 번역 단위에서 해당 타입의 **멤버에 접근**하는 경우

---

### ❌ BlueprintReadWrite on Internal State

```cpp
// Allows Blueprint to overwrite C++ state — fragile and hard to debug
UPROPERTY(EditAnywhere, BlueprintReadWrite)
float CurrentHealth;
```

### ✅ Use BlueprintReadOnly or Accessor Functions

```cpp
UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
float CurrentHealth;

UFUNCTION(BlueprintCallable)
void SetHealth(float NewHealth);
```

---

### Blueprint Can't See C++ Function

**Symptom**: Function not visible in Blueprint even though marked UFUNCTION

**Checklist**:
1. **UFUNCTION with BlueprintCallable or BlueprintPure**
```cpp
UFUNCTION(BlueprintCallable, Category = "MyCategory")
void MyFunction();
```

2. **Function is public** (Blueprint can't access private/protected)

3. **Class is blueprintable**
```cpp
UCLASS(Blueprintable)
class AMyCharacter : public ACharacter { };
```

4. **Hot reload completed** (see above)

5. **Regenerate project files**
```bash
# Close editor, then:
GenerateProjectFiles.bat  # On Windows
# Or use Unreal Build Tool
```

### Can't Find C++ Class in Blueprint

**Symptom**: Can't create Blueprint child of C++ class

**Solutions**:
1. **Mark class Blueprintable**
```cpp
UCLASS(Blueprintable, BlueprintType)
class PROJECTNAME_API AMyActor : public AActor { };
```

2. **Ensure class is compiled** and editor restarted

3. **Check Show Plugin Content / Show Engine Content** in Content Browser filters

## Asset Reference Issues

### Hard References Causing Long Load Times

**Problem**: Including asset headers causes entire asset to load

**Wrong**:
```cpp
// In header file - causes hard reference
#include "MyHugeAnimation.h"

UPROPERTY(EditAnywhere)
UAnimSequence* Animation = MyHugeAnimation;  // Loads entire asset at compile time
```

**Correct**:
```cpp
// In header - soft reference
UPROPERTY(EditAnywhere)
TSoftObjectPtr<UAnimSequence> Animation;

// Load when needed
if (!Animation.IsNull())
{
    UAnimSequence* LoadedAnim = Animation.LoadSynchronous();
}
```

### Finding Asset References in C++

**Problem**: Need to reference Blueprint or asset in C++

**Method 1: Expose as property**:
```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assets")
TSubclassOf<UGameplayAbility> AbilityClass;

// Set in Blueprint Details panel
```

**Method 2: Load by path**:
```cpp
// Get reference path: Right-click asset → Copy Reference
// Example: /Game/Blueprints/BP_MyCharacter.BP_MyCharacter_C

TSoftClassPtr<AActor> AssetClass = TSoftClassPtr<AActor>(
    FSoftObjectPath(TEXT("/Game/Blueprints/BP_MyCharacter.BP_MyCharacter_C"))
);

TSubclassOf<AActor> LoadedClass = AssetClass.LoadSynchronous();
```

**Method 3: ConstructorHelpers (for assets, not Blueprints)**:
```cpp
// In constructor only
static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(
    TEXT("/Game/Meshes/MyMesh")
);
if (MeshAsset.Succeeded())
{
    Mesh = MeshAsset.Object;
}
```

## Plugin Issues

### Plugin Not Loading

**Symptoms**:
- Classes from plugin not found
- Plugin marked as "Missing" in editor

**Solutions**:
1. **Check `.uproject`**: Ensure plugin listed as enabled
2. **Rebuild project**: Close editor, rebuild from IDE
3. **Check plugin compatibility**: Some plugins require specific engine versions
4. **Module dependencies**: Add plugin modules to `Build.cs`

### Experimental Plugin Documentation Missing

**Strategy**:
```
# Search Epic docs
web_search: "Unreal Engine [PluginName] documentation"

# Search community
web_search: "Unreal Engine [PluginName] tutorial example"

# Check source code
# Engine/Plugins/Experimental/PluginName/Source/
view Engine/Plugins/Experimental/PluginName/Source/Public

# Forums & AnswerHub
web_search: "site:forums.unrealengine.com [PluginName]"
```

## Multiplayer & Replication Issues

### Properties Not Replicating

**Checklist**:
```cpp
// 1. Mark property for replication
UPROPERTY(Replicated)
float Health;

// 2. Set replication in constructor
bReplicates = true;

// 3. Implement GetLifetimeReplicatedProps
void AMyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AMyActor, Health);
}

// 4. Include Net/UnrealNetwork.h
#include "Net/UnrealNetwork.h"
```

### RPC Not Executing

**Common issues**:
1. **Function not marked for RPC**
```cpp
UFUNCTION(Server, Reliable)
void ServerFunction();

// Must implement _Implementation
void AMyClass::ServerFunction_Implementation()
{
    // Logic here
}
```

2. **Actor not replicated**: `bReplicates = true`
3. **No authority**: Check `GetLocalRole()` and `GetRemoteRole()`

## Performance Issues

### Finding Performance Bottlenecks

**Common culprits**:
- Too many tick functions: Use timers or events instead
- Hard asset references: Use soft references
- Expensive Blueprint logic in Tick: Move to C++
- Unoptimized GAS usage: Batch gameplay cues, use minimal replication

> For profiling console commands (`stat fps`, `stat unit`, `profilegpu`, etc.), see `references/debugging.md`.

### ❌ Cast<T>() Inside Tick

```cpp
void AMyCharacter::Tick(float DeltaTime)
{
    // Called every frame — extremely expensive
    if (UHealthComponent* HC = Cast<UHealthComponent>(GetComponentByClass(...)))
    { ... }
}
```

### ✅ Cache in BeginPlay

```cpp
void AMyCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    HealthComp = FindComponentByClass<UHealthComponent>();
    check(HealthComp); // Fail hard in dev if missing
}
```

---

### ❌ Enabling Tick Unnecessarily

```cpp
AMyActor::AMyActor()
{
    PrimaryActorTick.bCanEverTick = true; // Default — don't leave this on blindly
}
```

### ✅ Disable by Default, Use Timers

```cpp
AMyActor::AMyActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AMyActor::BeginPlay()
{
    Super::BeginPlay();
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AMyActor::OnTimer, 1.f, true);
}
```

## Debugging Tools

> Console commands, logging setup, Visual Logger, and Visual Studio debugging tips are covered in `references/debugging.md`.

## Best Practices to Avoid Issues

1. **Always close editor before rebuilding** after major C++ changes
2. **Use source control** (Git, Perforce) for safety
3. **Test in PIE first**, then packaged build
4. **Check logs** in `Saved/Logs/` when things go wrong
5. **Use forward declarations** in headers when possible
6. **Profile early and often** to catch performance issues
7. **Document assumptions** about asset structure in code comments
8. **Version control asset references** with comments

## Emergency Recovery

### Project Won't Open

1. Delete `Intermediate/`, `Binaries/`, `Saved/` folders
2. Right-click `.uproject` → "Generate Visual Studio project files"
3. Build from IDE
4. Try opening again

### Corrupted Assets

1. Check source control for previous version
2. Use Content Browser → Show in Explorer → check `.uasset` file exists
3. Delete problematic asset (after backup)
4. Re-import or recreate

### Crashes on Launch

1. Check `Saved/Logs/ProjectName.log` for crash callstack
2. Try launching with `-log` parameter for detailed logging
3. Disable plugins one-by-one in `.uproject`
4. Restore from source control if recent changes suspected
