#pragma once

#include "CoreMinimal.h"
#include "GESDataTypes.h"
#include "PVDMaterialEffectControllerComp.generated.h"

class UMaterialEffectConfigDataAsset;
class UCurveFloat;
class UCurveLinearColor;

UENUM()
enum class EMatFXGlobalEvent : uint8
{
	MatFX_BeginPlay,
	MatFX_EnemyTakeDamage,
	MatFX_WeaponAttached,
	MatFX_WeaponDetached,
	MatFX_PerkDivineHaste,
	MatFX_PerkVolvansFeather,
	MatFx_MemoryPuzzle1,
	MatFx_MemoryPuzzle2,
	MatFx_MemoryPuzzle3,
	MatFx_MemoryPuzzle4,
	MatFx_BossBerserk,
	MatFx_BossSecondPhase,
	MatFx_EnemyTargeted,
	MatFx_EnemyUnTargeted,
	MatFx_PlayerHealed,
	MatFx_PlayerLevelUpXPGained,
	MatFx_PlayerLevelUp,
	MatFx_TurretDeath,
	MatFX_WeaponAuxAttached,
	MatFX_WeaponAuxDetached,
	MatFX_WeaponCharmSpawned,
	MatFX_EnemyBlockDamage,
	MatFX_EnemyBlockDamageOut,
	MatFX_ElementalLightHit,
	MatFX_ElementalSaltHit,
	MatFX_ElementalJusticeHit,
	MatFX_DivineStealthActivated,
	MatFX_DivineStealthDeactivated,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConfigRunnedWithGES, EMatFXGlobalEvent, MatFx);

UENUM()
enum class EMaterialEffectType
{
	OverrideMaterial,
	ChangeParameters
};

UENUM()
enum class EMaterialParameterChangeType
{
	Additive,
	Multiply,
	Override
};

UENUM()
enum class EMaterialParamType
{
	Float,
	Color UMETA(DisplayName="Color (Vector)"),
	Texture UMETA(DisplayName="Texture (Can't Animate)")
};

USTRUCT()
struct FMaterialParameterChangeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName ParameterName;
	
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "ParameterType != EMaterialParamType::Texture"))
	EMaterialParameterChangeType ParameterChangeType;

	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "ParameterType != EMaterialParamType::Texture"))
	bool IsAnimation;
	
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "IsAnimation && ParameterType != EMaterialParamType::Texture"))
	float AnimationTime;
	
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "IsAnimation && ParameterType != EMaterialParamType::Texture"))
	bool bLoopAnimation;
	
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "ParameterType == EMaterialParamType::Float"))
	bool ReturnToDefaultValue;
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "ReturnToDefaultValue && ParameterType == EMaterialParamType::Float"))

	float DefaultValue;
	
	UPROPERTY(EditAnywhere,
		meta = (EditConditionHides, EditCondition =
			"IsAnimation && ParameterType == EMaterialParamType::Float"
		))
	TObjectPtr<UCurveFloat> FloatCurve;
	
	UPROPERTY(EditAnywhere,
		meta = (EditConditionHides, EditCondition =
			"IsAnimation && ParameterType == EMaterialParamType::Color"
		))
	TObjectPtr<UCurveLinearColor> ColorCurve;
	
	UPROPERTY(EditAnywhere)
	bool HasDelay;

	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "hasDelay"))
	float Delay;

	UPROPERTY(EditAnywhere)
	bool HasLifetime;

	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "hasLifetime"))
	float Lifetime;

	UPROPERTY(EditAnywhere)
	EMaterialParamType ParameterType;

	UPROPERTY(EditAnywhere,
		meta = (EditConditionHides, EditCondition = "ParameterType == EMaterialParamType::Float"))
	float FloatParameterValue;

	UPROPERTY(EditAnywhere,
		meta = (EditConditionHides, EditCondition = "ParameterType == EMaterialParamType::Color"))
	FLinearColor LinearColorParameterValue;

	UPROPERTY(EditAnywhere,
		meta = (EditConditionHides, EditCondition =
			"!IsAnimation && ParameterType == EMaterialParamType::Texture"
		))
	TObjectPtr<UTexture2D> TextureParameterValue;
};

USTRUCT()
struct FMaterialEffectConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EMatFXGlobalEvent GlobalEventType;
	UPROPERTY(EditAnywhere)
	bool bHasFinisherEvent;
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "bHasFinisherEvent"))
	EMatFXGlobalEvent FinisherEventType;
	UPROPERTY(EditAnywhere)
	bool IsCameraPostProcessMaterial;
	UPROPERTY(EditAnywhere)
	EMaterialEffectType MaterialEffectType;
	UPROPERTY(EditAnywhere,meta = (EditConditionHides, EditCondition = "MaterialEffectType == EMaterialEffectType::ChangeParameters"))
	int Priority;
	UPROPERTY(EditAnywhere,meta = (EditConditionHides, EditCondition = "MaterialEffectType != EMaterialEffectType::ChangeParameters"))
	UMaterialInterface* EffectMaterial;
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "!IsCameraPostProcessMaterial"))
	bool EffectAllMeshes;
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "!EffectAllMeshes && !IsCameraPostProcessMaterial"))
	TArray<FString> EffectedMeshNameList;
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "EffectAllMeshes && !IsCameraPostProcessMaterial"))
	TArray<FString> ExcludedMeshNameList;
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "!IsCameraPostProcessMaterial"))
	bool IsOverlaySlot;
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "!IsOverlaySlot && !IsCameraPostProcessMaterial"))
	bool EffectAllSlots;
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "!EffectAllSlots && !IsOverlaySlot"))
	TArray<int> EffectedSlotIds;
	UPROPERTY(EditAnywhere,
		meta = (EditConditionHides, EditCondition = "MaterialEffectType == EMaterialEffectType::ChangeParameters"))
	TArray<FMaterialParameterChangeConfig> ParameterConfigs;
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "MaterialEffectType != EMaterialEffectType::ChangeParameters"))
	bool HasDelay;
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "hasDelay && MaterialEffectType != EMaterialEffectType::ChangeParameters"))
	float Delay;
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "MaterialEffectType != EMaterialEffectType::ChangeParameters"))
	bool HasLifetime;
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "hasLifetime && MaterialEffectType != EMaterialEffectType::ChangeParameters"))
	float Lifetime;
};

USTRUCT()
struct FMaterialEffectConfigContainer
{
	GENERATED_BODY()

	TArray<FMaterialEffectConfig> Configs;
};

UCLASS()
class UMaterialChangeHandler : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TObjectPtr<UMeshComponent> EffectedMesh;
	bool IsOverlaySlot = false;
	size_t SlotId;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> OldMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> NewMaterial;
	bool IsApplied = false;
	bool bKillFlag = false;
	float Delay = 0;
	float DelayCounter = 0;
	bool HasDelay;
	float Lifetime = 0;
	float LifetimeCounter = 0;
	bool HasLifetime;
	FString LambdaName;
	FGESEventContext EventContext;
};

UCLASS()
class UParameterChangeHandler : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TObjectPtr<UMeshComponent> EffectedMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;
	bool IsOverlaySlot = false;
	bool IsCameraPostProcessMaterial = false;
	size_t SlotId;
	float OldFloatValue;
	FLinearColor OldLinearColorValue;
	UPROPERTY()
	TObjectPtr<UTexture> OldTextureValue;
	FMaterialParameterChangeConfig Config;
	FMaterialEffectConfig* ParentConfig;
	bool IsApplied = false;
	bool bKillFlag = false;
	float DelayCounter = 0;
	float LifetimeCounter = 0;
	float AnimationCounter = 0;
	FString LambdaName;
	FGESEventContext EventContext;
	int Priority;

	void ApplyOldValues()
	{
		switch (Config.ParameterType)
		{
		case EMaterialParamType::Float:
			if(MaterialInstance != nullptr)
			{
				MaterialInstance->SetScalarParameterValue(
					Config.ParameterName,
					OldFloatValue);
			}
			break;
		case EMaterialParamType::Color:
			if(MaterialInstance != nullptr)
			{
				MaterialInstance->SetVectorParameterValue(
					Config.ParameterName,
					OldLinearColorValue);
			}
			break;
		case EMaterialParamType::Texture:
			if(MaterialInstance != nullptr)
			{
				MaterialInstance->SetTextureParameterValue(
					Config.ParameterName,
					OldTextureValue);
			}
			break;
		}
	}
};

template <>
struct TLess<UParameterChangeHandler>
{
	FORCEINLINE bool operator()(const UParameterChangeHandler& A,const UParameterChangeHandler& B) const
	{
		return A.Priority > B.Priority;
	}
};

UCLASS()
class UParameterChangeHandlerArray : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<UParameterChangeHandler*> Array;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PVD_API UPVDMaterialEffectControllerComp : public UActorComponent
{
	GENERATED_BODY()

public:
	UPVDMaterialEffectControllerComp();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TMap<EMatFXGlobalEvent, FMaterialEffectConfigContainer> EventConfigMap;
	
	UPROPERTY()
	TArray<UMaterialChangeHandler*> MaterialChangeHandlers;

	UPROPERTY()
	TMap<FString, UParameterChangeHandlerArray*> ParameterChangeHandlersMap;

	UPROPERTY()
	TMap<FString, UParameterChangeHandler*> PriorParameterChangeHandlerMap;

public:
	UPROPERTY(EditAnywhere)
	TArray<FMaterialEffectConfig> Configs;

	UPROPERTY(EditAnywhere)
	TArray<UMaterialEffectConfigDataAsset*> DataAssetConfigs;

	UPROPERTY(BlueprintAssignable)
	FConfigRunnedWithGES OnConfigRunnedWithGES;

private:
	UFUNCTION()
	void CategorizeConfigsWithEvents();

	UFUNCTION(BlueprintCallable)
	void RunConfigWithParameter(EMatFXGlobalEvent Type);

	UFUNCTION()
	void Run(TArray<FMaterialEffectConfig> ConfigArray);
	
	void Run(FMaterialEffectConfig& Config);

	//Parameter Change Functions
	
	UFUNCTION()
	void ProcessParameterChanges(float DeltaTime);
	
	UFUNCTION()
	const bool CreateParameterChangeHandler(FMaterialEffectConfig& Config, UMeshComponent* MeshComponent);
	
	UFUNCTION()
	void InitialSetupParameterChangeHandler(UParameterChangeHandler* ParameterChangeHandler);

	UParameterChangeHandler* EvaluatePriorParameterChangeHandler(TTuple<FString, UParameterChangeHandlerArray*> ParameterChangeHandlers);
	
	UFUNCTION()
	float CalculateAnimatedParameterConfigCurveTime(UParameterChangeHandler* ParameterChangeHandler);
	
	UFUNCTION()
	void ApplyParameterChange(UParameterChangeHandler* ParameterChangeHandler);
	
	UFUNCTION()
	const bool SetParametersOfPostProcessMaterials(FMaterialEffectConfig& Config);

	void GarbageCollectionCheckForParameterChanges(TTuple<FString, UParameterChangeHandlerArray*> ParameterChangeHandlers, UParameterChangeHandler* PriorParameterChangeHandler);
	
	//End of Parameter Change Functions

	//Material Change Functions
	
	UFUNCTION()
	void ProcessMaterialsChanges(float DeltaTime);
	
	UFUNCTION()
	const bool CreateMaterialChangeHandler(FMaterialEffectConfig& Config, UMaterialInterface* Material,
								  UMeshComponent* MeshComponent);
	
	UFUNCTION()
	void GarbageCollectionCheckForMaterialChanges();

	//End of Material Change Functions

	//Common Utility Functions
	
	UFUNCTION()
	UMaterialInstanceDynamic* CreateDynamicMaterialInstance(UParameterChangeHandler* ParameterChangeHandler);
	
	UFUNCTION()
	const TArray<UMeshComponent*> GetMeshes(const FMaterialEffectConfig& Config);

	//End of Common Utility Functions
};
