#include "../Components/PerkManagementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GESHandler.h"
#include "PVDMaterialEffectControllerComp.h"
#include "PVDPlayerProgressComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PVD/PVDPlayerController.h"
#include "PVD/PVDPlayerState.h"
#include "PVD/Characters/PVDCharacter.h"
#include "PVD/Characters/PVDCharacterBase.h"
#include "PVD/Subsystem/PVDIngameTutorialSubsystem.h"
#include "PVD/UI/GameplayStackWidget.h"
#include "PVD/UI/MainHUDWidget.h"
#include "PVD/UI/PerkPanelWidget.h"

UPerkManagementComponent::UPerkManagementComponent()
{
}

void UPerkManagementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const auto PlayerState = Cast<APVDPlayerState>(UGameplayStatics::GetPlayerState(GetWorld(), 0)))
	{
		PlayerProgressComponentPtr = MakeWeakObjectPtr(PlayerState->GetPlayerProgressComponent());
	}

	PlayerCharacterPtr = MakeWeakObjectPtr(Cast<APVDCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)));
	PlayerControllerPtr = MakeWeakObjectPtr(Cast<APVDPlayerController>(PlayerCharacterPtr->GetController()));
}

void UPerkManagementComponent::LevelUp()
{
	ShowRandomLevelPerks(3);
}

//Request Amount = 0 means all
TArray<UPerkDataAsset*> UPerkManagementComponent::GetPerkList(int8 RequestedAmount, EPerkPoolType PerkPoolType)
{
	TArray<UPerkDataAsset*> PerkPool;
	switch (PerkPoolType)
	{
	case EPerkPoolType::None:
		return PerkPool;
		break;
	case EPerkPoolType::LevelPerk:
		PerkPool = LevelUpPerks;
		break;
	case EPerkPoolType::ElementalPerk:
		PerkPool = ElementalPerks;
		break;
	case EPerkPoolType::CompanionPerk:
		PerkPool = CompanionPerks;
		break;
	case EPerkPoolType::TreePerk:
		return PerkPool;
		break;
	}

	/** Remove none selectable perks from the pool */
	PerkPool.RemoveAll<>([=](const UPerkDataAsset* PerkDataAsset)
	{
		return !PerkDataAsset->IsSelectable;
	});

	if (RequestedAmount > 0)
	{
		TArray<UPerkDataAsset*> PerkDataAssets;

		RequestedAmount = FMath::Clamp(RequestedAmount, 0, PerkPool.Num());
		if (RequestedAmount > 0)
		{
			for (int i = 0; i < RequestedAmount; i++)
			{
				const int32 RandomValue = FMath::RandRange(0, PerkPool.Num() - 1);
				PerkDataAssets.Add(PerkPool[RandomValue]);

				PerkPool.RemoveAt(RandomValue);
			}
		}

		return PerkDataAssets;
	}

	/** return all available Perks that are selectable/purchasable */
	return PerkPool;
}

bool UPerkManagementComponent::CanLevelUp() const
{
	bool bCanLevelUp = false;
	if (PlayerProgressComponentPtr.IsValid())
	{
		bCanLevelUp = PlayerProgressComponentPtr->CanLevelUp();

		PlayerControllerPtr->GetHUDWidget()->ToggleLevelUpIndication(bCanLevelUp);
	}

	return bCanLevelUp;
}

void UPerkManagementComponent::OnUIClosed()
{
	PerkPanelWidget->OnUICanceled.RemoveDynamic(this, &ThisClass::OnUIClosed);

	PerkPanelWidget->DeactivateWidget();

	if (const auto PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (const auto PlayerCharacter = Cast<APVDCharacter>(PlayerController->GetCharacter()))
		{
			GES_MATERIAL_EFFECT_EMIT(EMatFXGlobalEvent::MatFx_PlayerLevelUp, PlayerCharacter);
		}
	}
}

void UPerkManagementComponent::ShowRandomLevelPerks(int8 Amount)
{
	// Player may skip current level up via ESC, do not refresh previously displayed list
	if (RandomSelectedLevelUpPerks.IsEmpty())
	{
		RandomSelectedLevelUpPerks = GetPerkList(Amount, EPerkPoolType::LevelPerk);
	}

	if (PerkPanelWidgetClass)
	{
		PerkPanelWidget = Cast<UPerkPanelWidget>(PlayerControllerPtr->HUDStack->PushStack(PerkPanelWidgetClass));

		PerkPanelWidget->OnPerkSelect.AddUniqueDynamic(this, &ThisClass::OnLevelUpPerkSelected);
		PerkPanelWidget->OnUICanceled.AddUniqueDynamic(this, &ThisClass::OnUIClosed);

		PerkPanelWidget->DrawLevelUpPerks(RandomSelectedLevelUpPerks, OwnedPerks);
	}
}

int UPerkManagementComponent::GetCurrentLevelOfPerk(UPerkDataAsset* PerkDataAsset)
{
	if (OwnedPerks.Contains(PerkDataAsset))
	{
		return OwnedPerks[PerkDataAsset];
	}
	return 0;
}

UPerkDataAsset* UPerkManagementComponent::SetElementalPerk(EElementalPerkType PerkType)
{
	UPerkDataAsset* ChoosenElementalPerkDataAsset = nullptr;
	switch (PerkType)
	{
	case EElementalPerkType::Light:
		ChoosenElementalPerkDataAsset = LightElementalPerk;
		ElementalPerks.Append(LightElementalPerks);
		break;
	case EElementalPerkType::Salt:
		ChoosenElementalPerkDataAsset = SaltElementalPerk;
		ElementalPerks.Append(SaltElementalPerks);
		break;
	case EElementalPerkType::Justice:
		ChoosenElementalPerkDataAsset = JusticeElementalPerk;
		ElementalPerks.Append(JusticeElementalPerks);
		break;
	default: ;
	}
	if (ChoosenElementalPerkDataAsset != nullptr)
		PlayerCharacterPtr.Get()->GetPerkActorComponent()->GainPerk(ChoosenElementalPerkDataAsset, EPerkPoolType::None, 1);
	return ChoosenElementalPerkDataAsset;
}

bool UPerkManagementComponent::GainPerk(UPerkDataAsset* PerkDataAsset, EPerkPoolType PerkPoolType, int count)
{
	if (!IsValid(PerkDataAsset))
		return false;

	if (PerkDataAsset->TutorialTag.IsValid())
	{
		GetWorld()->GetGameInstance()->GetSubsystem<UPVDIngameTutorialSubsystem>()->ActivateTutorialWithTag(PerkDataAsset->TutorialTag);
	}

	const bool IsPerkOwned = OwnedPerks.Contains(PerkDataAsset);

	int PerkLevel;

	if (IsPerkOwned)
	{
		PerkLevel = GetCurrentPerkLevel(PerkDataAsset);
	}
	else
	{
		PerkLevel = count;
	}

	if (PerkDataAsset->PerkLevelDatas.Num() == PerkLevel - 1)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red,
		                                 FString::Printf(TEXT("Perk gaining failed, max level already reached :%s %d"), *PerkDataAsset->GetName(), PerkLevel + 1));
		return false;
	}

	if (!IsPerkOwned)
	{
		OwnedPerks.Add(PerkDataAsset, count);
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Blue,
		                                 FString::Printf(TEXT("Perk Gained:%s %d"), *PerkDataAsset->GetName(), OwnedPerks[PerkDataAsset]));;
	}
	else
	{
		TrySetCurrentPerkLevel(PerkDataAsset, PerkLevel + 1);
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Yellow,
		                                 FString::Printf(TEXT("Owned Perk Level Set:%s %d"), *PerkDataAsset->GetName(), OwnedPerks[PerkDataAsset]));
	}

	bool IsPerkLastLevel = PerkDataAsset->PerkLevelDatas.Num() <= PerkLevel + count;

	/** remove from displayable perk pool list accoridng to type */
	if (IsPerkLastLevel)
	{
		TArray<UPerkDataAsset*>* PerkPool = nullptr;
		switch (PerkPoolType)
		{
		case EPerkPoolType::None:
			break;
		case EPerkPoolType::LevelPerk:
			PerkPool = &LevelUpPerks;
			break;
		case EPerkPoolType::ElementalPerk:
			PerkPool = &ElementalPerks;
			break;
		case EPerkPoolType::CompanionPerk:
			PerkPool = &CompanionPerks;
			break;
		case EPerkPoolType::TreePerk:
			break;
		default: ;
		}

		if (PerkPool)
			PerkPool->Remove(PerkDataAsset);
	}

	switch (PerkDataAsset->PerkType)
	{
	case EPerkType::GameplayAbility:
		return GivePerkAbility(PerkDataAsset);
	case EPerkType::GameplayEffect:
		if (ApplyGameplayEffect(PerkDataAsset))
		{
			ApplyPostGainGameplayEffect(PerkDataAsset);
			OnPerkGained.Broadcast();
			return true;
		}
	default:
		PVD_LOG(Error, TEXT("Given Perk Data Asset's Perk Type is Invalid!"))
		return false;
	}
}

bool UPerkManagementComponent::LosePerk(UPerkDataAsset* PerkDataAsset, EPerkPoolType PerkPoolType)
{
	const bool IsPerkOwned = OwnedPerks.Contains(PerkDataAsset);

	if (IsPerkOwned)
	{
		if (PerkDataAsset->ActivationGameplayEffectHandle.IsValid())
		{
			PlayerCharacterPtr->GetAbilitySystemComponent()->RemoveActiveGameplayEffect(PerkDataAsset->ActivationGameplayEffectHandle);
			PerkDataAsset->ActivationGameplayEffectHandle.Invalidate();
		}
		
		ClearPerkAbilities(PerkDataAsset);
		OwnedPerks.Remove(PerkDataAsset);

		TArray<UPerkDataAsset*>* PerkPool = nullptr;
		switch (PerkPoolType)
		{
		case EPerkPoolType::None:
			break;
		case EPerkPoolType::LevelPerk:
			PerkPool = &LevelUpPerks;
			break;
		case EPerkPoolType::ElementalPerk:
			PerkPool = &ElementalPerks;
			break;
		case EPerkPoolType::CompanionPerk:
			PerkPool = &CompanionPerks;
			break;
		case EPerkPoolType::TreePerk:
			break;
		default: ;
		}

		if (PerkPool != nullptr)
		{
			PerkPool->Add(PerkDataAsset);
		}
	}

	return IsPerkOwned;
}

bool UPerkManagementComponent::GivePerkAbility(UPerkDataAsset* PerkDataAsset)
{
	const bool IsPerkOwned = OwnedPerks.Contains(PerkDataAsset);

	if (!IsPerkOwned)
	{
		return false;
	}

	APVDCharacter* Character = Cast<APVDCharacter>(GetOwner());

	if (!IsValid(Character))
	{
		return false;
	}

	UAbilitySystemComponent* AbilitySystemComponent = Character->GetAbilitySystemComponent();

	if (!IsValid(AbilitySystemComponent))
	{
		return false;
	}

	uint8 PerkLevelIndex = GetCurrentPerkLevel(PerkDataAsset) - 1;

	if (PerkDataAsset->PerkLevelDatas.IsValidIndex(PerkLevelIndex))
	{
		/** if this is not the first level of the perk, and type is Override clear the previous ability */
		if (PerkLevelIndex >= 1 && PerkDataAsset->AbilityImplementationType == EPerkAbilityImplementationType::Override)
		{
			TArray<FGameplayAbilitySpec> Abilities = AbilitySystemComponent->GetActivatableAbilities();

			FGameplayAbilitySpec* GameplayAbilitySpec = Abilities.FindByPredicate([&PerkDataAsset , PerkLevelIndex](const FGameplayAbilitySpec& AbilitySpec)
			{
				return AbilitySpec.Ability.Get() == PerkDataAsset->PerkLevelDatas[PerkLevelIndex - 1].Ability.GetDefaultObject(); // return previous level ability to clear
			});

			if (GameplayAbilitySpec != nullptr)
			{
				AbilitySystemComponent->ClearAbility(GameplayAbilitySpec->Handle);
			}
		}

		if (PerkDataAsset->PerkLevelDatas[PerkLevelIndex].Ability != nullptr)
		{
			const auto AbilityClass = PerkDataAsset->PerkLevelDatas[PerkLevelIndex].Ability;
			const auto AbilityInputID = static_cast<int32>(PerkDataAsset->PerkLevelDatas[PerkLevelIndex].Ability.GetDefaultObject()->AbilityInputID);
			const auto AbilityLevel = PerkLevelIndex + 1;
			const auto AbilitySpec = FGameplayAbilitySpec(AbilityClass, AbilityLevel, AbilityInputID, this);

			const auto GameplayAbilitySpecHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);

			/** Set spec handle of the perk ablity to reference if needed */
			PerkDataAsset->GivenAbilitySpecHandle = GameplayAbilitySpecHandle;

			if (PerkDataAsset->PerkLevelDatas[PerkLevelIndex].UseAbilityOnAdd)
			{
				AbilitySystemComponent->TryActivateAbilityByClass(PerkDataAsset->PerkLevelDatas[PerkLevelIndex].Ability);

				if (PerkDataAsset->HasMaterialControlFX)
				{
					GES_MATERIAL_EFFECT_EMIT(PerkDataAsset->MaterialFXEventName, Character);
				}
			}

			if (PerkDataAsset->PerkLevelDatas[PerkLevelIndex].ReactivateOnCooldownEnds)
			{
				FGameplayTag CooldownTag = PerkDataAsset->PerkLevelDatas[PerkLevelIndex].Ability.GetDefaultObject()->GetCooldownTagContainer().First();
				PerkDataAsset->CooldownTag = CooldownTag;
				AutoActivateAbilityCooldownTagMap.Add(CooldownTag, GameplayAbilitySpecHandle);
				AbilitySystemComponent->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnGameplayEffectTagChanged);
				const auto InBattleTag = FGameplayTag::RequestGameplayTag("State.InBattle");
				AutoActivateAbilityHandles.Add(GameplayAbilitySpecHandle);
				AbilitySystemComponent->RegisterGameplayTagEvent(InBattleTag, EGameplayTagEventType::AnyCountChange).AddUObject(this, &ThisClass::OnInBattleTagChanged);
			}
		}
	}

	return true;
}

int UPerkManagementComponent::GetCurrentPerkLevel(const UPerkDataAsset* PerkDataAsset)
{
	if (OwnedPerks.Contains(PerkDataAsset))
		return OwnedPerks[PerkDataAsset];
	
	return -1;
}

bool UPerkManagementComponent::TrySetCurrentPerkLevel(const UPerkDataAsset* PerkDataAsset, const int Level)
{
	if (!IsValid(PerkDataAsset))
	{
		return false;
	}

	if (!OwnedPerks.Contains(PerkDataAsset))
	{
		return false;
	}

	OwnedPerks[PerkDataAsset] = Level;

	return true;
}

void UPerkManagementComponent::ActivateAutoAbilitiesIgnoreCooldown()
{
	if (UAbilitySystemComponent* AbilitySystemComponent = PlayerCharacterPtr->GetAbilitySystemComponent())
	{
		for (TPair<FGameplayTag, FGameplayAbilitySpecHandle> TagMapTuple : AutoActivateAbilityCooldownTagMap)
		{
			if (auto AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromHandle(TagMapTuple.Value))
			{
				for (UGameplayAbility* Ability : AbilitySpec->GetAbilityInstances())
				{
					if (auto PVDAbility = Cast<UPVDGameplayAbility>(Ability))
					{
						PVDAbility->CancelCooldown();
					}
				}
			}
		}
	}
}

void UPerkManagementComponent::OnGameplayEffectTagChanged(const FGameplayTag InCallbackTag, int32 InTagCount)
{
	if (APVDCharacter* Character = Cast<APVDCharacter>(GetOwner()))
	{
		if (UAbilitySystemComponent* AbilitySystemComponent = Character->GetAbilitySystemComponent())
		{
			// if (InTagCount == 0)
			// {
			// 	
			// }
			if (Character->GetCurrentRoomType() == ERoomType::Battle || Character->GetCurrentRoomType() == ERoomType::Boss)
			{
				AbilitySystemComponent->TryActivateAbility(AutoActivateAbilityCooldownTagMap[InCallbackTag]);
			}
		}
	}
}

void UPerkManagementComponent::OnInBattleTagChanged(const FGameplayTag InCallbackTag, int32 InTagCount)
{
	if (APVDCharacter* Character = Cast<APVDCharacter>(GetOwner()))
	{
		if (UAbilitySystemComponent* AbilitySystemComponent = Character->GetAbilitySystemComponent())
		{
			if (Character->GetCurrentRoomType() == ERoomType::Battle || Character->GetCurrentRoomType() == ERoomType::Boss)
			{
				for (auto AbilityHandle : AutoActivateAbilityHandles)
				{
					AbilitySystemComponent->TryActivateAbility(AbilityHandle);
				}
			}
		}
	}
}

void UPerkManagementComponent::OnLevelUpPerkSelected(UPerkDataAsset* InGainPerk)
{
	// Clear displayed list for next level up
	RandomSelectedLevelUpPerks.Empty();

	GainPerk(InGainPerk, EPerkPoolType::LevelPerk);
	
	OnUIClosed();

	// we still have level up opportunity
	if (CanLevelUp())
	{
		ShowRandomLevelPerks(3);
	}
	
}

void UPerkManagementComponent::ClearPerkAbilities(const UPerkDataAsset* PerkDataAsset)
{
	switch (PerkDataAsset->AbilityImplementationType)
	{
	case EPerkAbilityImplementationType::Override:
		ClearOverridePerkAbility(PerkDataAsset);
		break;
	case EPerkAbilityImplementationType::Additive:
		ClearAdditivePerkAbility(PerkDataAsset);
		break;
	default:
		PVD_LOG(Error, TEXT("Perk Ability Implementation Type is Invalid!"));
	}

	AutoActivateAbilityCooldownTagMap.Remove(PerkDataAsset->CooldownTag);
}

void UPerkManagementComponent::ClearOverridePerkAbility(const UPerkDataAsset* PerkDataAsset)
{
	uint8 PerkLevel = GetCurrentPerkLevel(PerkDataAsset);

	APVDCharacter* Character = Cast<APVDCharacter>(GetOwner());

	if (!IsValid(Character))
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystemComponent = Character->GetAbilitySystemComponent();

	TArray<FGameplayAbilitySpec> Abilities = AbilitySystemComponent->GetActivatableAbilities();

	FGameplayAbilitySpec* GameplayAbilitySpec = Abilities.FindByPredicate(
		[&PerkDataAsset , PerkLevel](const FGameplayAbilitySpec& AbilitySpec)
		{
			return AbilitySpec.Ability.Get() == PerkDataAsset->PerkLevelDatas[PerkLevel - 1].Ability.
			                                                                                 GetDefaultObject();
		});

	if (GameplayAbilitySpec != nullptr)
	{
		AbilitySystemComponent->ClearAbility(GameplayAbilitySpec->Handle);
	}
}

void UPerkManagementComponent::ClearAdditivePerkAbility(const UPerkDataAsset* PerkDataAsset)
{
	APVDCharacter* Character = Cast<APVDCharacter>(GetOwner());

	if (!IsValid(Character))
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystemComponent = Character->GetAbilitySystemComponent();

	TArray<FGameplayAbilitySpec> Abilities = AbilitySystemComponent->GetActivatableAbilities();

	int PerkLevel = GetCurrentPerkLevel(PerkDataAsset);

	for (int i = 0; i < PerkLevel; i++)
	{
		FGameplayAbilitySpec* GameplayAbilitySpec = Abilities.FindByPredicate(
			[&PerkDataAsset , i](const FGameplayAbilitySpec& AbilitySpec)
			{
				return AbilitySpec.Ability.Get() == PerkDataAsset->PerkLevelDatas[i].Ability.GetDefaultObject();
			});

		if (GameplayAbilitySpec != nullptr)
		{
			AbilitySystemComponent->ClearAbility(GameplayAbilitySpec->Handle);
		}
	}
}

bool UPerkManagementComponent::ApplyGameplayEffect(UPerkDataAsset* PerkDataAsset)
{
	if (!IsValid(PerkDataAsset))
	{
		PVD_LOG(Error, TEXT("Given Perk Data Asset is Invalid!"));
		return false;
	}

	UGameplayEffect* GameplayEffect = PerkDataAsset->GameplayEffect.GetDefaultObject();

	if (!IsValid(GameplayEffect))
	{
		PVD_LOG(Error, TEXT("Given Perk Data Asset's Gameplay Effect is Invalid!"));
		return false;
	}

	APVDCharacter* Character = Cast<APVDCharacter>(GetOwner());

	if (!IsValid(Character))
	{
		return false;
	}

	//Can be Cached???
	UAbilitySystemComponent* AbilitySystemComponent = Character->GetAbilitySystemComponent();

	if (!IsValid(AbilitySystemComponent))
	{
		return false;
	}

	const auto PerkLevel = GetCurrentPerkLevel(PerkDataAsset);

	// remove previous active effect i.e. if it is infinite and marked as not "additively stacked"
	if (PerkLevel > 1 && PerkDataAsset->IsRemovePreviousActiveLevel)
	{
		if (PerkDataAsset->ActivationGameplayEffectHandle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(PerkDataAsset->ActivationGameplayEffectHandle);
		}
	}

	FGameplayEffectContextHandle GameplayEffectContextHandle = AbilitySystemComponent->MakeEffectContext();
	GameplayEffectContextHandle.AddSourceObject(GetOwner());

	const auto EffectSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(PerkDataAsset->GameplayEffect, PerkLevel,GameplayEffectContextHandle);
	
	if (PerkDataAsset->IsSetMagnitudeByCaller)
	{
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle,
		                                                              PerkDataAsset->MagnitudeDataTag.First(),
		                                                              PerkDataAsset->Magnitude);

		PerkDataAsset->ActivationGameplayEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	}
	else
	{
		PerkDataAsset->ActivationGameplayEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	}

	if (PerkDataAsset->HasMaterialControlFX)
	{
		GES_MATERIAL_EFFECT_EMIT(PerkDataAsset->MaterialFXEventName, Character);
	}

	return true;
}

bool UPerkManagementComponent::ApplyPostGainGameplayEffect(UPerkDataAsset* PerkDataAsset)
{
	if (!IsValid(PerkDataAsset->PostGainGameplayEffect))
	{
		PVD_LOG(Error, TEXT("Given Perk Data Asset's Post Gain Gameplay Effect is Invalid!"));
		return false;
	}

	UGameplayEffect* GameplayEffect = PerkDataAsset->PostGainGameplayEffect.GetDefaultObject();

	if (!IsValid(GameplayEffect))
	{
		PVD_LOG(Error, TEXT("Given Perk Data Asset's Gameplay Effect is Invalid!"));
		return false;
	}

	UAbilitySystemComponent* AbilitySystemComponent = PlayerCharacterPtr->GetAbilitySystemComponent();

	if (!IsValid(AbilitySystemComponent))
	{
		return false;
	}

	FGameplayEffectContextHandle GameplayEffectContextHandle;

	if (PerkDataAsset->IsSetMagnitudeByCaller)
	{
		GameplayEffectContextHandle = AbilitySystemComponent->MakeEffectContext();
		GameplayEffectContextHandle.AddSourceObject(GetOwner());

		const auto EffectSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(PerkDataAsset->GameplayEffect, 1,
		                                                                       GameplayEffectContextHandle);

		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle,
		                                                              PerkDataAsset->MagnitudeDataTag.First(),
		                                                              PerkDataAsset->Magnitude);

		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	}
	else
	{
		AbilitySystemComponent->ApplyGameplayEffectToSelf(GameplayEffect,
		                                                  1,
		                                                  GameplayEffectContextHandle);
	}

	return true;
}
