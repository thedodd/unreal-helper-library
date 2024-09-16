﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/DebugSubsystem/UHLDebugSubsystem.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystem/UHLAbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Core/UHLGameplayTags.h"
#include "Development/UHLDebugSubsystemSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/DebugSubsystem/UHLDebugCategoryComponent.h"
#include "UI/UHLDebugWidget.h"


UUHLDebugSubsystem::UUHLDebugSubsystem()
    : Super()
{
    static ConstructorHelpers::FClassFinder<UUHLDebugWidget> DefaultMasterClass(TEXT("/UnrealHelperLibrary/UI/UI_UHL_DebugWidget"));
    UHLDebugWidgetClass = DefaultMasterClass.Class;
}

void UUHLDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const UUHLDebugSubsystemSettings* DeveloperSettings = GetDefault<UUHLDebugSubsystemSettings>();
    DebugCategories = DeveloperSettings->DebugCategories;

    // TODO on actors initiallized enabled debug categories
    // or give opportunity to make it in PlayerController
    if (DeveloperSettings->bEnableDebugCategoriesOnStart)
    {
        // TODO:
        // GetOuter()->GetWorld()->OnWorldBeginPlay.AddWeakLambda([=]()
        // {
        //     SetUp();
        // });
    }
}

void UUHLDebugSubsystem::SetUp()
{
    if (bSetupped) return;

    bSetupped = true;
    const UUHLDebugSubsystemSettings* DeveloperSettings = GetDefault<UUHLDebugSubsystemSettings>();
    DebugCategories = DeveloperSettings->DebugCategories;

    OnDebugCategoryChanged.AddUniqueDynamic(this, &UUHLDebugSubsystem::OnDebugCategoryChangedInternal);

    for (const TTuple<FGameplayTag, bool>& EnabledDebugCategory : DeveloperSettings->EnabledDebugCategories)
    {
        if (EnabledDebugCategory.Value == true)
        {
            EnableDebugCategory(EnabledDebugCategory.Key, EnabledDebugCategory.Value);
        }
    };
}

bool UUHLDebugSubsystem::IsCategoryEnabled(const FGameplayTag DebugCategoryTag) const
{
    const FUHLDebugCategory* UHLDebugCategory = DebugCategories.FindByPredicate([=](const FUHLDebugCategory& DebugCategory)
    {
        return DebugCategory.Tags.HasAny(FGameplayTagContainer(DebugCategoryTag));
    });
    if (UHLDebugCategory != nullptr)
    {
        return UHLDebugCategory->bEnabled;
    }
    return false;
}

void UUHLDebugSubsystem::EnableDebugCategory(const FGameplayTag DebugCategoryTag, bool bEnable)
{
    bool bActivated = bEnable;

    FUHLDebugCategory* UHLDebugCategory = DebugCategories.FindByPredicate([=](const FUHLDebugCategory& DebugCategory)
    {
        return DebugCategory.Tags.HasAny(FGameplayTagContainer(DebugCategoryTag));
    });
    if (UHLDebugCategory != nullptr)
    {
        if (bEnable)
        {
            bActivated = UHLDebugCategory->TryActivate(this);
        }
        else
        {
            // TODO deactivate
        }
    }

    for (FGameplayTag GameplayTag : UHLDebugCategory->Tags.GetGameplayTagArray())
    {
        OnDebugCategoryChanged.Broadcast(GameplayTag, bActivated);
    }
}

void UUHLDebugSubsystem::ToggleAbilityInputDebug()
{
    UUHLAbilitySystemComponent* UHLASC = GetPlayerAbilitySystemComponent();

    UUHLDebugWidget* DebugWidget = GetOrCreateUHLDebugWidget();
    if (!DebugWidget) return;
    DebugWidget->ToggleAbilityInputCache(UHLASC);
}

UUHLDebugWidget* UUHLDebugSubsystem::GetOrCreateUHLDebugWidget()
{
    if (DebugWidgetInstance) return DebugWidgetInstance;

    APlayerController* PlayerController = GetPlayerController();
    if (!PlayerController) return nullptr;

    DebugWidgetInstance = CreateWidget<UUHLDebugWidget>(PlayerController, UHLDebugWidgetClass);
    DebugWidgetInstance->AddToViewport(99999999);

    return DebugWidgetInstance;
}

void UUHLDebugSubsystem::OnDebugCategoryChangedInternal(FGameplayTag DebugCategoryTag, bool bEnabled)
{
    if (DebugCategoryTag == UHLGameplayTags::TAG_UHL_DebugCategory_InputSystem_AbilityInputCache)
    {
        ToggleAbilityInputDebug();
    }
}

APlayerController* UUHLDebugSubsystem::GetPlayerController() const
{
    return UGameplayStatics::GetPlayerController(GetWorld(), 0);
}

UUHLAbilitySystemComponent* UUHLDebugSubsystem::GetPlayerAbilitySystemComponent() const
{
    APlayerController* PlayerController = GetPlayerController();
    if (!PlayerController) return nullptr;

    APawn* Pawn = PlayerController->GetPawn();
    if (!Pawn) return nullptr;

    IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Pawn);
    if (!AbilitySystemInterface) return nullptr;

    UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent();
    if (!ASC) return nullptr;

    UUHLAbilitySystemComponent* UHLASC = Cast<UUHLAbilitySystemComponent>(ASC);
    if (!UHLASC) return nullptr;

    return UHLASC;
}
