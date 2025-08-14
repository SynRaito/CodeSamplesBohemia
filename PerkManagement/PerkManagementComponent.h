// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PerkManagementComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPerkGained);

class ALevelUpAltar;
class UPVDPlayerProgressComponent;
class UPerkDataAsset;

UENUM(BlueprintType)
enum class EPerkPoolType : uint8
{
	None,
	LevelPerk,
	ElementalPerk,
	CompanionPerk,
	TreePerk
};

UENUM(BlueprintType)
enum class EElementalPerkType : uint8
{
	None,
	Light,
	Salt,
	Justice
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PVD_API UPerkManagementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPerkManagementComponent();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable)
	void LevelUp();
	
	UFUNCTION()
	bool GainPerk(UPerkDataAsset* PerkDataAsset, EPerkPoolType PerkPoolType, int count = 1);
	
	UFUNCTION()
	bool LosePerk(UPerkDataAsset* PerkDataAsset, EPerkPoolType PerkPoolType);

	/** Get Amount of available perks excluding not selectable ones. Return all available if Amount is not supplied */
	UFUNCTION()
	TArray<UPerkDataAsset*> GetPerkList(int8 Amount, EPerkPoolType PerkPoolType);

	UFUNCTION(BlueprintCallable, Category=PVD)
	int GetCurrentPerkLevel(const UPerkDataAsset* PerkDataAsset);

	UFUNCTION()
	const TMap<UPerkDataAsset* , int8>& GetOwnedPerks() const { return OwnedPerks; }

	UFUNCTION()
	bool CanLevelUp() const;
	
	UFUNCTION()
	int GetCurrentLevelOfPerk(UPerkDataAsset* PerkDataAsset);
	
	UFUNCTION()
	UPerkDataAsset* SetElementalPerk(EElementalPerkType PerkType);
	
	UFUNCTION()
	void ActivateAutoAbilitiesIgnoreCooldown();
	
protected:
	// Perk Panel Widget setup
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UPerkPanelWidget> PerkPanelWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UPerkPanelWidget> PerkPanelWidget;
	// end
	
private:
	UFUNCTION()
	void ShowRandomLevelPerks(int8 Amount);
	UFUNCTION()
	void ClearPerkAbilities(const UPerkDataAsset* PerkDataAsset);
	UFUNCTION()
	void ClearOverridePerkAbility(const UPerkDataAsset* PerkDataAsset);
	UFUNCTION()
	void ClearAdditivePerkAbility(const UPerkDataAsset* PerkDataAsset);
	UFUNCTION()
	bool ApplyGameplayEffect(UPerkDataAsset* PerkDataAsset);
	UFUNCTION()
	bool ApplyPostGainGameplayEffect(UPerkDataAsset* PerkDataAsset);
	UFUNCTION()
	bool GivePerkAbility(UPerkDataAsset* PerkDataAsset);
	UFUNCTION()
	bool TrySetCurrentPerkLevel(const UPerkDataAsset* PerkDataAsset,int Level);

	UFUNCTION()
	void OnGameplayEffectTagChanged(const FGameplayTag InCallbackTag, int32 InTagCount);
	UFUNCTION()
	void OnInBattleTagChanged(const FGameplayTag InCallbackTag, int32 InTagCount);
	
	UFUNCTION()
	void OnLevelUpPerkSelected(UPerkDataAsset* InGainPerk);

	UFUNCTION(BlueprintCallable, Category=PVD)
	void OnUIClosed();

	UPROPERTY()
	TMap<UPerkDataAsset* , int8> OwnedPerks;

	UPROPERTY()
	TArray<UPerkDataAsset *> RandomSelectedLevelUpPerks;

	TWeakObjectPtr<class APVDCharacter> PlayerCharacterPtr;
	TWeakObjectPtr<class APVDPlayerController> PlayerControllerPtr;
	TWeakObjectPtr<UPVDPlayerProgressComponent> PlayerProgressComponentPtr;
	
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AutoActivateAbilityHandles;
	
	UPROPERTY()
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> AutoActivateAbilityCooldownTagMap;
	
public:
	uint32 bElementalPerkChoosed:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UPerkDataAsset*> LevelUpPerks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UPerkDataAsset* LightElementalPerk;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UPerkDataAsset* SaltElementalPerk;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UPerkDataAsset* JusticeElementalPerk;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<UPerkDataAsset*> LightElementalPerks;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<UPerkDataAsset*> SaltElementalPerks;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<UPerkDataAsset*> JusticeElementalPerks;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<UPerkDataAsset*> ElementalPerks;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UPerkDataAsset*> CompanionPerks;

	UPROPERTY(BlueprintAssignable)
	FOnPerkGained OnPerkGained;
	
};
