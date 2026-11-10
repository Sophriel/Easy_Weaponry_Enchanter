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
#include "EWEInventoryViewModel.h"
#include "EWE/Character/EWECharacter.h"
#include "EWE/Character/EWEInventoryComponent.h"
#include "EWE/Character/EWEPlayerController.h"
#include "EasyWeaponryEnchanter/Interface/EWECharacterInterface.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

void UEWELocalUIManageSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    const UEWESettings *Settings = GetDefault<UEWESettings>();
    if (Settings && Settings->UIConfigAsset.IsValid())
    {
        UIConfigAsset = Settings->UIConfigAsset.Get();
    }
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

    // Unbind the view models from their Model before tearing them down.
    if (QuickSlotViewModel)
    {
        QuickSlotViewModel->BindModel(nullptr, nullptr);
        QuickSlotViewModel = nullptr;
    }

    if (InventoryViewModel)
    {
        InventoryViewModel->BindModel(nullptr, nullptr);
        InventoryViewModel = nullptr;
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

UEWEInventoryComponent *UEWELocalUIManageSubsystem::GetCurrentInventoryModel() const
{
    AEWEPlayerController *EWEPC = Cast<AEWEPlayerController>(CurrentPlayerController.Get());
    return EWEPC ? EWEPC->GetInventoryModel() : nullptr;
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

    CreateInventoryViewModel();
    if (!InventoryViewModel)
        return;

    // Pull the current Model so a widget created after possession still binds. Bind only
    // this view model — rebinding the other one would clear its state (inventory selection).
    UEWEInventoryComponent *Model = GetCurrentInventoryModel();
    if (Model)
    {
        InventoryViewModel->BindModel(nullptr, Model);
    }

    InventoryWidget->SetViewModel(InventoryViewModel);
}

void UEWELocalUIManageSubsystem::CreateInventoryViewModel()
{
    if (InventoryViewModel)
        return;

    // Only allocates the view model. The Model binding happens in the widget-creation
    // pull and OnInventoryModelChanged, not here. The slot array stays empty until a
    // Model is bound.
    InventoryViewModel = NewObject<UEWEInventoryViewModel>(this);
    if (!ensure(InventoryViewModel))
    {
        EWE_LOG(LogEWE, Error, TEXT("Failed to create inventory view model"));
        return;
    }
}

#pragma endregion

#pragma region QuickSlot

void UEWELocalUIManageSubsystem::CreateQuickSlotViewModel()
{
    if (QuickSlotViewModel)
        return;

    // Only allocates the view model. The Model binding happens in the widget-creation
    // pull and OnInventoryModelChanged, not here. The slot array stays empty until a
    // Model is bound.
    QuickSlotViewModel = NewObject<UEWEQuickSlotViewModel>(this);
    if (!ensure(QuickSlotViewModel))
    {
        EWE_LOG(LogEWE, Error, TEXT("Failed to create quick slot view model"));
        return;
    }
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

    CreateQuickSlotViewModel();
    if (!QuickSlotViewModel)
        return;

    // Pull the current Model so a widget created after possession still binds. Bind only
    // this view model — rebinding the other one would clear its state (inventory selection).
    UEWEInventoryComponent *Model = GetCurrentInventoryModel();
    if (Model)
    {
        QuickSlotViewModel->BindModel(nullptr, Model);
    }

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
    }

    if (InventoryViewModel)
    {
        InventoryViewModel->BindModel(Old, New);
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

    CreateStatusViewModel();
    if (!StatusViewModel)
        return;

    BindStatus();
    StatusWidget->SetViewModel(StatusViewModel);
}

void UEWELocalUIManageSubsystem::CreateStatusViewModel()
{
    if (StatusViewModel)
        return;

    StatusViewModel = NewObject<UEWEStatusViewModel>(this);
    if (!ensure(StatusViewModel))
    {
        EWE_LOG(LogEWE, Error, TEXT("Failed to create status view model"));
        return;
    }
}

void UEWELocalUIManageSubsystem::BindStatus()
{
    if (!IsValid(CurrentPlayerController) || !StatusViewModel)
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
        EWE_LOG(LogEWE, Warning, TEXT("BindStatus: ASC not found on pawn"));
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

    CreateTargetInfoViewModel();
    if (!TargetInfoViewModel)
        return;

    BindTargetInfo();
    TargetInfoWidget->SetViewModel(TargetInfoViewModel);
}

void UEWELocalUIManageSubsystem::CreateTargetInfoViewModel()
{
    if (TargetInfoViewModel)
        return;

    TargetInfoViewModel = NewObject<UEWETargetInfoViewModel>(this);
    if (!ensure(TargetInfoViewModel))
    {
        EWE_LOG(LogEWE, Error, TEXT("Failed to create target info view model"));
        return;
    }
}

void UEWELocalUIManageSubsystem::UpdateTargetName(const FText &TargetName)
{
    if (!TargetInfoViewModel)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("UpdateTargetName: TargetInfoViewModel not created"));
        return;
    }

    TargetInfoViewModel->SetTarget(TargetName);
}

void UEWELocalUIManageSubsystem::UpdateTargetInfo(const FText &TargetName, UAbilitySystemComponent *TargetASC)
{
    if (!TargetInfoViewModel)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("UpdateTargetInfo: TargetInfoViewModel not created"));
        return;
    }

    TargetInfoViewModel->SetTarget(TargetName, TargetASC);
}

void UEWELocalUIManageSubsystem::ClearTargetInfo()
{
    if (!TargetInfoViewModel)
        return;

    TargetInfoViewModel->ClearTarget();
}

#pragma endregion