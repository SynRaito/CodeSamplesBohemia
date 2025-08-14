#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PVD/Components/PVDMaterialEffectControllerComp.h"
#include "PVD/Enums/PerkAbilityImplementationType.h"
#include "PVD/Enums/PerkType.h"
#include "PVD/GAS/Abilities/PVDGameplayAbility.h"
#include "PVD/Localization/LocalizedTextFormatter.h"
#include "PerkDataAsset.generated.h"

enum class EElementalPerkType : uint8;

UENUM(BlueprintType)
enum class EPerkPresentationMode : uint8
{
	None,
	PerkInfo,
	CharacterInfo,
	WeaponInfo
};

UENUM(BlueprintType)
enum class EPerkRarity : uint8
{
	None = 0,
	Common,
	Rare,
	Epic,
	Legendary
};

USTRUCT(BlueprintType)
struct FPerkLevelData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Information)
	FULocalizedTextFormatter Name;
	UPROPERTY(EditAnywhere)
	UTexture2D* GrayscaleImage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Information) 
	FULocalizedTextFormatter Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Information)
	EPerkRarity PerkRarity = EPerkRarity::Common;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Information)
	UTexture2D* Image;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Information)
	uint8 PurchaseCost = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ability)
	TSubclassOf<UPVDGameplayAbility> Ability;
	UPROPERTY(EditAnywhere , BlueprintReadWrite, Category = Ability)
	bool UseAbilityOnAdd;
	UPROPERTY(EditAnywhere , BlueprintReadWrite, meta=(EditCondition = "UseAbilityOnAdd") , Category = Ability)
	bool ReactivateOnCooldownEnds;
};

UCLASS(BlueprintType)
class PVD_API UPerkDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPerkType PerkType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EElementalPerkType ElementalPerkType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPerkPresentationMode PresentationMode = EPerkPresentationMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPerkSFXType PerkSFXType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag TutorialTag;

	/** Some perks should not be offered as selection i.e in level up, or safe rooms.
	 * If false will not be added to available perks pool list when display is requested */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsSelectable = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Logic , meta=(EditCondition="PerkType == EPerkType::GameplayAbility" , EditConditionHides))
	EPerkAbilityImplementationType AbilityImplementationType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Logic , meta=(EditCondition="PerkType == EPerkType::GameplayEffect" , EditConditionHides))
	TSubclassOf<UGameplayEffect> GameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Logic , meta=(EditCondition="PerkType == EPerkType::GameplayEffect" , EditConditionHides))
	bool IsRemovePreviousActiveLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Logic , meta=(EditCondition="PerkType == EPerkType::GameplayEffect" , EditConditionHides))
	bool IsSetMagnitudeByCaller;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Logic , meta=(EditCondition="PerkType == EPerkType::GameplayEffect && IsSetMagnitudeByCaller == true" , EditConditionHides))
	FGameplayTagContainer MagnitudeDataTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Logic)
	bool HasMaterialControlFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Logic , meta=(EditCondition="HasMaterialControlFX == true" , EditConditionHides))
	EMatFXGlobalEvent MaterialFXEventName;


	/* Gameplay Effect to apply each time this perk or its levels gained.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Logic , meta=(EditCondition="PerkType == EPerkType::GameplayEffect" , EditConditionHides))
	TSubclassOf<UGameplayEffect> PostGainGameplayEffect;

	/** owner of the perk should set the magnitude */
	UPROPERTY()
	float Magnitude;

	UPROPERTY(EditAnywhere , BlueprintReadWrite , Category = Information)
	TArray<FPerkLevelData> PerkLevelDatas;

	UPROPERTY()
	FActiveGameplayEffectHandle ActivationGameplayEffectHandle; 

	UPROPERTY()
	FGameplayAbilitySpecHandle GivenAbilitySpecHandle;

	UPROPERTY()
	FGameplayTag CooldownTag;
	
#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif
};

