// Fill out your copyright notice in the Description page of Project Settings.

#include "EWELocalUIManageSubsystem.h"
#include "EWEInventory.h"
#include "EWEQuickSlot.h"
#include "EWEStatus.h"
#include "EWETargetInfo.h"
#include "EWEUIAsset.h"
#include "EWE/EWESettings.h"
#include "EWE/EWELog.h"
#include "EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h"
#include "EWEStatusViewModel.h"
#include "EWETargetInfoViewModel.h"
#include "EWEQuickSlotViewModel.h"
#include "EWE/Character/EWECharacter.h"
#include "EWE/Character/EWEInventoryComponent.h"
#include "EasyWeaponryEnchanter/Interface/EWECharacterInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "View/MVVMView.h"

// Initialize subsystem
void UEWELocalUIManageSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    const UEWESettings *Settings = GetDefault<UEWESettings>();
    if (Settings && Settings->UIConfigAsset.IsValid())
    {
        UIConfigAsset = Settings->UIConfigAsset.Get();
    }

    CreateQuickSlotViewModel();
}

// Cleanup on shutdown
void UEWELocalUIManageSubsystem::Deinitialize()
{
    // UEWEUIAsset outlives this subsystem, so never leave a subscription behind.
    ClearUILoadedSubscription();

    // Tear the view model down before the widget, so the widget's injected
    // reference is still valid while we unsubscribe.
    if (QuickSlotViewModel)
    {
        QuickSlotViewModel->OnSlotSelected.RemoveAll(this);
        QuickSlotViewModel = nullptr;
    }

    // Drop the inventory subscription; it must not outlive this subsystem.
    if (UEWEInventoryComponent *Inventory = BoundInventory.Get())
    {
        Inventory->OnWeaponsSynced.RemoveDynamic(this, &UEWELocalUIManageSubsystem::HandleWeaponsSynced);
    }
    BoundInventory = nullptr;

    // Destroy widgets
    if (InventoryWidget)
    {
        CloseInventory();
        InventoryWidget->RemoveFromParent();
        InventoryWidget = nullptr;
    }

    if (QuickSlotWidget)
    {
        QuickSlotWidget->RemoveFromParent();
        QuickSlotWidget = nullptr;
    }

    if (StatusViewModel)
    {
        StatusViewModel->SetSource(nullptr);
        StatusViewModel = nullptr;
    }

    if (StatusWidget)
    {
        StatusWidget->RemoveFromParent();
        StatusWidget = nullptr;
    }

    if (TargetInfoViewModel)
    {
        TargetInfoViewModel->ClearTarget();
        TargetInfoViewModel = nullptr;
    }

    if (TargetInfoWidget)
    {
        TargetInfoWidget->RemoveFromParent();
        TargetInfoWidget = nullptr;
    }

    Super::Deinitialize();
}

void UEWELocalUIManageSubsystem::PlayerControllerChanged(APlayerController *NewPlayerController)
{
    CurrentPlayerController = NewPlayerController;

    // This fires more than once per session (possession, respawn, non-seamless travel).
    // The essential HUD is owned by this subsystem and outlives the controller, so once
    // it is on screen there is nothing to load and nothing to create.
    if (AreEssentialWidgetsInViewport())
    {
        return;
    }

    UEWEUIAsset *UIConfig = UIConfigAsset.Get();
    if (!UIConfig)
    {
        EWE_LOG(LogEWE, Error, TEXT("PlayerControllerChanged: UIConfigAsset is not loaded"));
        return;
    }

    if (UIConfig->IsWidgetsLoaded())
    {
        AddUIWidgets();
        return;
    }

    // Widget classes are still streaming. Subscribe once; re-entry before the load
    // completes must not stack duplicate callbacks.
    if (!UILoadedHandle.IsValid())
    {
        UILoadedHandle = UIConfig->OnUILoaded.AddUObject(this, &UEWELocalUIManageSubsystem::AddUIWidgets);
    }
}

bool UEWELocalUIManageSubsystem::AreEssentialWidgetsInViewport() const
{
    return QuickSlotWidget && QuickSlotWidget->IsInViewport() && StatusWidget && StatusWidget->IsInViewport() &&
           TargetInfoWidget && TargetInfoWidget->IsInViewport();
}

void UEWELocalUIManageSubsystem::ClearUILoadedSubscription()
{
    if (!UILoadedHandle.IsValid())
    {
        return;
    }

    if (UEWEUIAsset *UIConfig = UIConfigAsset.Get())
    {
        UIConfig->OnUILoaded.Remove(UILoadedHandle);
    }

    UILoadedHandle.Reset();
}

const UEWEUIAsset *UEWELocalUIManageSubsystem::GetUIConfigAsset() const
{
    const UEWEUIAsset *UIConfig = UIConfigAsset.Get();
    if (!UIConfig)
    {
        EWE_LOG(LogEWE, Error, TEXT("UIConfigAsset not set in UEWESettings"));
        return nullptr;
    }

    return UIConfig;
}

void UEWELocalUIManageSubsystem::AddUIWidgets()
{
    // OnUILoaded is a one-shot for this subsystem. Drop the subscription whether we
    // arrived here from the callback or directly from PlayerControllerChanged.
    ClearUILoadedSubscription();

    // Create widgets when ASyncLoad is ready (always visible)
    CreateQuickSlotWidget();
    CreateStatusWidget();

    // TargetInfo starts hidden until a target is acquired
    CreateTargetInfoWidget();
}

#pragma region Inventory

void UEWELocalUIManageSubsystem::ToggleInventory()
{
    if (!InventoryWidget)
    {
        CreateInventoryWidget();
    }

    if (!InventoryWidget)
    {
        EWE_LOG(LogEWE, Warning, TEXT("Failed to create inventory widget"));
        return;
    }

    if (InventoryWidget->IsInventoryOpen())
    {
        CloseInventory();
    }
    else
    {
        OpenInventory();
    }
}

void UEWELocalUIManageSubsystem::OpenInventory()
{
    if (!InventoryWidget)
    {
        EWE_LOG(LogEWE, Warning, TEXT("InventoryWidget not created"));
        return;
    }

    InventoryWidget->OpenInventory();
}

void UEWELocalUIManageSubsystem::CloseInventory()
{
    if (!InventoryWidget)
    {
        return;
    }

    InventoryWidget->CloseInventory();
}

bool UEWELocalUIManageSubsystem::IsInventoryOpen() const
{
    return InventoryWidget && InventoryWidget->IsInventoryOpen();
}

void UEWELocalUIManageSubsystem::SyncInventoryWeapons(const TArray<class UEWEWeaponData *> &Weapons)
{
    if (!InventoryWidget)
    {
        EWE_LOG(LogEWE, Warning, TEXT("InventoryWidget not created, cannot sync weapons"));
        return;
    }

    InventoryWidget->SetWeapons(Weapons);
}

void UEWELocalUIManageSubsystem::CreateInventoryWidget()
{
    if (InventoryWidget)
        return;

    const UEWEUIAsset *UIConfig = GetUIConfigAsset();
    if (!UIConfig)
        return;

    TSoftClassPtr<UUserWidget> WidgetClass = UIConfig->InventoryWidgetClass;
    if (!WidgetClass)
    {
        EWE_LOG(LogEWE, Error, TEXT("InventoryWidgetClass not set in UEWEUIAsset"));
        return;
    }

    InventoryWidget = CreateWidget<UEWEInventory>(GetWorld(), WidgetClass.Get());
    if (!InventoryWidget)
    {
        EWE_LOG(LogEWE, Error, TEXT("Failed to create Inventory widget"));
        return;
    }

    InventoryWidget->AddToViewport();
}

#pragma endregion

#pragma region QuickSlot

void UEWELocalUIManageSubsystem::CreateQuickSlotViewModel()
{
    if (QuickSlotViewModel)
        return;

    QuickSlotViewModel = NewObject<UEWEQuickSlotViewModel>(this);

    // Subscribe exactly once, at creation. Never inside BindQuickSlot(), which runs
    // more than once and returns early on several paths.
    QuickSlotViewModel->OnSlotSelected.AddUObject(
        this, &UEWELocalUIManageSubsystem::HandleSlotSelected);

    // The slot array stays empty until the widget attaches and sizes it.
}

void UEWELocalUIManageSubsystem::HandleSlotSelected(int32 SlotIndex, UEWEWeaponData *Weapon)
{
    if (!IsValid(CurrentPlayerController))
        return;

    APawn *CurrentPawn = CurrentPlayerController->GetPawn();
    if (!CurrentPawn)
    {
        EWE_LOG(LogEWE, Warning, TEXT("HandleSlotSelected: no pawn possessed"));
        return;
    }

    // EquipWeapon is a pure virtual on IEWECharacterInterface. Do not cast to the
    // concrete AEWECharacter here.
    IEWECharacterInterface *CharacterInterface = Cast<IEWECharacterInterface>(CurrentPawn);
    if (!CharacterInterface)
    {
        EWE_LOG(LogEWE, Warning, TEXT("HandleSlotSelected: pawn does not implement IEWECharacterInterface"));
        return;
    }

    // Weapon may be nullptr — selecting an empty slot unequips. See EquipWeapon().
    CharacterInterface->EquipWeapon(Weapon);
}

void UEWELocalUIManageSubsystem::CreateQuickSlotWidget()
{
    if (QuickSlotWidget)
        return;

    const UEWEUIAsset *UIConfig = GetUIConfigAsset();
    if (!UIConfig)
        return;

    TSoftClassPtr<UUserWidget> WidgetClass = UIConfig->QuickSlotWidgetClass;
    if (!WidgetClass)
    {
        EWE_LOG(LogEWE, Error, TEXT("QuickSlotWidgetClass not set in UEWEUIAsset"));
        return;
    }

    QuickSlotWidget = CreateWidget<UEWEQuickSlot>(GetWorld(), WidgetClass.Get());
    if (!QuickSlotWidget)
    {
        EWE_LOG(LogEWE, Error, TEXT("Failed to create QuickSlot widget"));
        return;
    }

    QuickSlotWidget->AddToViewport();

    QuickSlotWidget->SetViewModel(QuickSlotViewModel);

    BindQuickSlot();
}

void UEWELocalUIManageSubsystem::BindQuickSlot()
{
    // No QuickSlotWidget check on purpose. The view model outlives the widget and
    // must be able to hold data before any widget exists.
    if (!IsValid(CurrentPlayerController) || !QuickSlotViewModel)
        return;

    // Subscribe first. The guards below return early on several paths, and a
    // subscription placed after them would silently never happen.
    SubscribeToInventory();

    // The widget owns the slot count, so there is nothing to fill until it attaches.
    // This is expected on the possession path and must not log as a warning.
    if (QuickSlotViewModel->GetSlotCount() == 0)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("BindQuickSlot: slots not sized yet, deferring"));
        return;
    }

    SeedQuickSlotFromInventory();
}

void UEWELocalUIManageSubsystem::SubscribeToInventory()
{
    APawn *CurrentPawn = CurrentPlayerController->GetPawn();
    if (!CurrentPawn)
        return;

    UEWEInventoryComponent *Inventory = CurrentPawn->FindComponentByClass<UEWEInventoryComponent>();
    if (!Inventory)
    {
        EWE_LOG(LogEWE, Warning, TEXT("SubscribeToInventory: inventory component not found on pawn"));
        return;
    }

    if (BoundInventory.Get() == Inventory)
        return;

    // The pawn may have been replaced. Drop the previous subscription first.
    if (UEWEInventoryComponent *Previous = BoundInventory.Get())
    {
        Previous->OnWeaponsSynced.RemoveDynamic(this, &UEWELocalUIManageSubsystem::HandleWeaponsSynced);
    }

    Inventory->OnWeaponsSynced.AddDynamic(this, &UEWELocalUIManageSubsystem::HandleWeaponsSynced);
    BoundInventory = Inventory;
}

void UEWELocalUIManageSubsystem::SeedQuickSlotFromInventory()
{
    if (!QuickSlotViewModel || QuickSlotViewModel->GetSlotCount() == 0)
        return;

    UEWEInventoryComponent *Inventory = BoundInventory.Get();
    if (!Inventory)
        return;

    // Seed only when nothing has been assigned yet. A rebuild must not overwrite
    // an arrangement the player already changed.
    if (QuickSlotViewModel->IsEmpty())
    {
        QuickSlotViewModel->SeedFromInventory(Inventory->GetAllWeapons());
    }

    // Ensure a slot is selected — whether the data arrived through the seed
    // above or through the AddWeapon() relay. Selecting an empty slot
    // broadcasts nullptr (unarmed), which is correct while the inventory is
    // still empty.
    if (QuickSlotViewModel->GetSelectedIndex() == INDEX_NONE)
    {
        QuickSlotViewModel->SelectSlot(0);
    }
}

void UEWELocalUIManageSubsystem::HandleWeaponsSynced(const TArray<UEWEWeaponData *> &InWeapons, int32 NewCount)
{
    // The widget may still be missing. CreateQuickSlotWidget() calls BindQuickSlot()
    // afterwards, so that path picks the data up instead.
    SeedQuickSlotFromInventory();
}

void UEWELocalUIManageSubsystem::AddWeaponToQuickSlot(UEWEWeaponData *Weapon)
{
    if (!Weapon)
    {
        EWE_LOG(LogEWE, Warning, TEXT("AddWeaponToQuickSlot: Weapon is null"));
        return;
    }
    if (!QuickSlotViewModel)
    {
        EWE_LOG(LogEWE, Warning, TEXT("AddWeaponToQuickSlot: view model not created"));
        return;
    }
    QuickSlotViewModel->AddWeapon(Weapon);
}

void UEWELocalUIManageSubsystem::SetQuickSlot(int32 SlotIndex, UEWEWeaponData *Weapon)
{
    if (!QuickSlotViewModel)
    {
        EWE_LOG(LogEWE, Warning, TEXT("SetQuickSlot: view model not created"));
        return;
    }
    QuickSlotViewModel->SetSlot(SlotIndex, Weapon);
}

void UEWELocalUIManageSubsystem::SelectQuickSlot(int32 SlotIndex)
{
    if (!QuickSlotViewModel)
    {
        EWE_LOG(LogEWE, Warning, TEXT("SelectQuickSlot: view model not created"));
        return;
    }
    QuickSlotViewModel->SelectSlot(SlotIndex);
}

void UEWELocalUIManageSubsystem::ScrollQuickSlot(float ScrollDirection)
{
    if (!QuickSlotViewModel)
    {
        EWE_LOG(LogEWE, Warning, TEXT("ScrollQuickSlot: view model not created"));
        return;
    }
    QuickSlotViewModel->ScrollSlot(ScrollDirection);
}

#pragma endregion

#pragma region Status

void UEWELocalUIManageSubsystem::CreateStatusWidget()
{
    if (StatusWidget)
        return;

    const UEWEUIAsset *UIConfig = GetUIConfigAsset();
    if (!UIConfig)
        return;

    TSoftClassPtr<UUserWidget> WidgetClass = UIConfig->StatusWidgetClass;
    if (!WidgetClass)
    {
        EWE_LOG(LogEWE, Error, TEXT("StatusWidgetClass not set in UEWEUIAsset"));
        return;
    }

    StatusWidget = CreateWidget<UEWEStatus>(GetWorld(), WidgetClass.Get());
    if (!StatusWidget)
    {
        EWE_LOG(LogEWE, Error, TEXT("Failed to create Status widget"));
        return;
    }

    StatusWidget->AddToViewport();

    // Create the ViewModel instance.
    StatusViewModel = NewObject<UEWEStatusViewModel>(this);
    UMVVMView *MVVMView = StatusWidget->GetExtension<UMVVMView>();
    if (!MVVMView)
    {
        EWE_LOG(LogEWE, Warning, TEXT("WBP_Status has no MVVMView extension. Enable ViewModel in WBP Class Settings."));
        return;
    }

    // Inject the ViewModel into the Blueprint widget.
    MVVMView->SetViewModel(FName(TEXT("EWEStatusViewModel")), StatusViewModel);

    BindStatus();
}

void UEWELocalUIManageSubsystem::BindStatus()
{
    if (!CurrentPlayerController || !StatusWidget || !StatusViewModel)
        return;

    APawn *CurrentPawn = CurrentPlayerController->GetPawn();
    if (!CurrentPawn)
        return;

    IAbilitySystemInterface *CurrentPawnASI = Cast<IAbilitySystemInterface>(CurrentPawn);
    if (!CurrentPawnASI)
        return;

    UAbilitySystemComponent *CurrentPawnASC = CurrentPawnASI->GetAbilitySystemComponent();
    if (!CurrentPawnASC)
    {
        EWE_LOG(LogEWE, Warning, TEXT("CreateStatusWidget: ASC not found on pawn"));
        return;
    }

    StatusViewModel->SetSource(CurrentPawnASC);
}

#pragma endregion

#pragma region TargetInfo

void UEWELocalUIManageSubsystem::BindTargetInfo()
{
    if (!IsValid(CurrentPlayerController) || !TargetInfoViewModel)
        return;

    AEWECharacter *Char = Cast<AEWECharacter>(CurrentPlayerController->GetPawn());
    if (!Char)
        return;

    // Re-query the focused target so a late widget does not stay blank.
    Char->RefreshTargetInfo();
}

void UEWELocalUIManageSubsystem::CreateTargetInfoWidget()
{
    if (TargetInfoWidget)
        return;

    const UEWEUIAsset *UIConfig = GetUIConfigAsset();
    if (!UIConfig)
        return;

    TSoftClassPtr<UUserWidget> WidgetClass = UIConfig->TargetInfoWidgetClass;
    if (!WidgetClass)
    {
        EWE_LOG(LogEWE, Warning, TEXT("TargetInfoWidgetClass not set in UEWEUIAsset — skipping"));
        return;
    }

    TargetInfoWidget = CreateWidget<UEWETargetInfo>(GetWorld(), WidgetClass.Get());
    if (!TargetInfoWidget)
    {
        EWE_LOG(LogEWE, Error, TEXT("Failed to create TargetInfo widget"));
        return;
    }

    TargetInfoWidget->AddToViewport();
    TargetInfoWidget->SetVisibility(ESlateVisibility::Collapsed);

    // Create the ViewModel and inject it into the Blueprint widget.
    TargetInfoViewModel = NewObject<UEWETargetInfoViewModel>(this);
    UMVVMView *MVVMView = TargetInfoWidget->GetExtension<UMVVMView>();
    if (!MVVMView)
    {
        EWE_LOG(LogEWE, Warning,
                TEXT("TargetInfo widget has no MVVMView extension. Enable ViewModel in WBP Class Settings."));
        return;
    }

    MVVMView->SetViewModel(FName(TEXT("EWETargetInfoViewModel")), TargetInfoViewModel);

    BindTargetInfo();
}

void UEWELocalUIManageSubsystem::ShowTargetInfo()
{
    if (!TargetInfoWidget)
        return;

    TargetInfoWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UEWELocalUIManageSubsystem::HideTargetInfo()
{
    if (!TargetInfoWidget)
        return;

    TargetInfoWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void UEWELocalUIManageSubsystem::UpdateTargetName(const FText &TargetName)
{
    if (!TargetInfoViewModel)
    {
        EWE_LOG(LogEWE, Warning, TEXT("UpdateTargetName: TargetInfoViewModel not created"));
        return;
    }

    TargetInfoViewModel->SetTarget(TargetName);
    ShowTargetInfo();
}

void UEWELocalUIManageSubsystem::UpdateTargetInfo(const FText &TargetName, UAbilitySystemComponent *TargetASC)
{
    if (!TargetInfoViewModel)
    {
        EWE_LOG(LogEWE, Warning, TEXT("UpdateTargetInfo: TargetInfoViewModel not created"));
        return;
    }

    TargetInfoViewModel->SetTarget(TargetName, TargetASC);
    ShowTargetInfo();
}

void UEWELocalUIManageSubsystem::ClearTargetInfo()
{
    if (!TargetInfoViewModel)
        return;

    TargetInfoViewModel->ClearTarget();
    HideTargetInfo();
}

#pragma endregion