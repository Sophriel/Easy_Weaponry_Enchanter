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
#include "EWE/Character/EWEPlayerController.h"
#include "EasyWeaponryEnchanter/Interface/EWECharacterInterface.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "View/MVVMView.h"

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

void UEWELocalUIManageSubsystem::Deinitialize()
{
    // UEWEUIAsset outlives this subsystem, so never leave a subscription behind.
    ClearUILoadedSubscription();

    // Drop the player controller's Model relay subscription; it must not outlive this
    // subsystem.
    AEWEPlayerController *PC = Cast<AEWEPlayerController>(CurrentPlayerController.Get());
    if (PC)
    {
        PC->OnInventoryModelChanged.Remove(InventoryModelChangedHandle);
    }
    InventoryModelChangedHandle.Reset();

    // Unbind the view model from its Model before tearing it down.
    if (QuickSlotViewModel)
    {
        QuickSlotViewModel->BindModel(nullptr, nullptr);
        QuickSlotViewModel = nullptr;
    }

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
    // Relay the inventory Model to the quick slot view model. This runs before the
    // widget-creation early returns below, so a late controller still reaches the view
    // model even when the essential HUD is already on screen.
    BindInventoryModel(NewPlayerController);

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

    UEWEUIAsset *UIConfig = UIConfigAsset.Get();
    if (UIConfig)
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

    // Always visible.
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

    // The view model is an observer: it binds to the inventory Model through
    // BindInventoryModel, not here. The slot array stays empty until a Model is bound.
    QuickSlotViewModel = NewObject<UEWEQuickSlotViewModel>(this);
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

    // The view model may already be bound to a Model (the possession path can run before
    // the widget exists). SetViewModel is idempotent and re-drives slot creation.
    QuickSlotWidget->SetViewModel(QuickSlotViewModel);
}

void UEWELocalUIManageSubsystem::BindInventoryModel(APlayerController *NewPlayerController)
{
    // Drop the previous controller's subscription first.
    AEWEPlayerController *PreviousPC = Cast<AEWEPlayerController>(CurrentPlayerController.Get());
    if (PreviousPC)
    {
        PreviousPC->OnInventoryModelChanged.Remove(InventoryModelChangedHandle);
    }
    InventoryModelChangedHandle.Reset();

    if (!NewPlayerController)
    {
        return;
    }

    // The Model relay lives on the EWE controller, not the engine base.
    AEWEPlayerController *EWEPC = Cast<AEWEPlayerController>(NewPlayerController);
    if (!EWEPC)
    {
        return;
    }

    InventoryModelChangedHandle = EWEPC->OnInventoryModelChanged.AddUObject(
        this, &UEWELocalUIManageSubsystem::HandleInventoryModelChanged);

    // Pull the current Model for a late subscriber. The PC fires OnInventoryModelChanged
    // only on possession, so a controller that already possesses a pawn must be pulled.
    UEWEInventoryComponent *Model = EWEPC->GetInventoryModel();
    if (Model)
    {
        HandleInventoryModelChanged(nullptr, Model);
    }
}

void UEWELocalUIManageSubsystem::HandleInventoryModelChanged(UEWEInventoryComponent *Old, UEWEInventoryComponent *New)
{
    if (QuickSlotViewModel)
    {
        QuickSlotViewModel->BindModel(Old, New);

        // The slot count is known only once the view model is bound. Re-drive the widget
        // so it creates its slot widgets now (a no-op if it already has them).
        if (QuickSlotWidget)
        {
            QuickSlotWidget->SetViewModel(QuickSlotViewModel);
        }
    }
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

    StatusViewModel = NewObject<UEWEStatusViewModel>(this);
    UMVVMView *MVVMView = StatusWidget->GetExtension<UMVVMView>();
    if (!MVVMView)
    {
        EWE_LOG(LogEWE, Warning, TEXT("WBP_Status has no MVVMView extension. Enable ViewModel in WBP Class Settings."));
        return;
    }

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