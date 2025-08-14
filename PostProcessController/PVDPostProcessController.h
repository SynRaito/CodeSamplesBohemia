// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <functional>

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PVDPostProcessController.generated.h"

#define POST_PROCESS_float_BASEVALUESET(ATTRIBUTENAME) Config.floatBaseValue = PostProcessSettings.ATTRIBUTENAME;\

#define POST_PROCESS_FVector4_BASEVALUESET(ATTRIBUTENAME) Config.FVector4BaseValue = PostProcessSettings.ATTRIBUTENAME;\

#define POST_PROCESS_FLinearColor_BASEVALUESET(ATTRIBUTENAME) Config.FLinearColorBaseValue = PostProcessSettings.ATTRIBUTENAME;\

#define POST_PROCESS_ATTRIBUTE_CASE(TYPE, ATTRIBUTENAME) \
	case EPostProcessAttributes::##ATTRIBUTENAME:\
	Process = [&](FPostProcessSettings& PostProcessSettings, FPostProcessControllerConfig& Config, float DeltaTime){\
		TYPE Value = TYPE(); \
		if(!Config.bIsBaseValueInitialized){\
			POST_PROCESS_##TYPE##_BASEVALUESET(ATTRIBUTENAME)\
			Config.bIsBaseValueInitialized = true;\
		}\
		if(!Config.bIsTargetValueInitialized){\
			switch (Config.OperationType) {\
				case EOperationType::Additive :\
					Config.TYPE##TargetValue = Config.TYPE##BaseValue + Value; \
					break;\
				case EOperationType::Multiply :\
					Config.TYPE##TargetValue = Config.TYPE##BaseValue * Value; \
					break;\
				case EOperationType::Override :\
					Config.TYPE##TargetValue = Value; \
					break;\
			}\
			Config.bIsTargetValueInitialized = true;\
		}\
		switch (Config.CalculationType){\
			case ECalculationType::Instant : \
				Value = Config.TYPE##TargetValue;\
				Config.bIsCompleted = true;\
				break;\
			case ECalculationType::Lerp :\
				Value = Config.TYPE##BaseValue + (Config.TYPE##TargetValue - Config.TYPE##BaseValue) * Config.EffectTimer / Config.EffectLength;\
				break;\
			case ECalculationType::Curve :\
				Value = Config.TYPE##BaseValue + (Config.TYPE##TargetValue - Config.TYPE##BaseValue) * Config.FloatCurve->GetFloatValue(Config.EffectTimer / Config.EffectLength);\
				break;\
		}\
		PostProcessSettings.ATTRIBUTENAME = Value;\
	}; \
	End =  [&](FPostProcessSettings& PostProcessSettings, FPostProcessControllerConfig& Config){\
		if(Config.bUseBaseValueAsReturnValue){\
			PostProcessSettings.ATTRIBUTENAME = Config.TYPE##BaseValue;\
		}\
		else{\
			PostProcessSettings.ATTRIBUTENAME = Config.TYPE##ReturnValue;\
		}\
	};\
	break;\

UENUM()
enum class EPPFXGlobalEvent : uint8
{
	PPFX_BeginPlay
};

UENUM(BlueprintType)
enum class EPostProcessAttributes : uint8
{
	WhiteTemp UMETA(DisplayName = "Temperature (float)"),
	SceneColorTint UMETA(DisplayName = "Color Tint (FLinearColor)"),
	ColorSaturation UMETA(DisplayName = "Color Saturation (FVector4)")
};

UENUM(BlueprintType)
enum class EPostProcessAttributeType : uint8
{
	Float,
	FVector4,
	FLinearColor
};

UENUM(BlueprintType)
enum class EOperationType : uint8
{
	Override,
	Additive,
	Multiply
};

UENUM(BlueprintType)
enum class ECalculationType : uint8
{
	Instant,
	Lerp,
	Curve
};

USTRUCT(BlueprintType)
struct FPostProcessControllerConfig
{
	GENERATED_BODY()

	int Priority = 0;

	UPROPERTY(EditAnywhere)
	EPostProcessAttributes Attribute;

	UPROPERTY(EditAnywhere)
	EPostProcessAttributeType AttributeType;

	UPROPERTY(EditAnywhere)
	EOperationType OperationType;
	
	UPROPERTY(EditAnywhere)
	ECalculationType CalculationType;

	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "CalculationType == ECalculationType::Curve"))
	TObjectPtr<UCurveFloat> FloatCurve;

	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "AttributeType == EPostProcessAttributeType::FVector4"))
	FVector4 FVector4Value;

	UPROPERTY()
	FVector4 FVector4BaseValue;
	
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "AttributeType == EPostProcessAttributeType::FVector4 && !bUseBaseValueAsReturnValue"))
	FVector4 FVector4ReturnValue;

	UPROPERTY()
	FVector4 FVector4TargetValue;
	
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "AttributeType == EPostProcessAttributeType::FLinearColor"))
	FLinearColor FLinearColorValue;

	UPROPERTY()
	FLinearColor FLinearColorBaseValue;
	
	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "AttributeType == EPostProcessAttributeType::FLinearColor && !bUseBaseValueAsReturnValue"))
	FLinearColor FLinearColorReturnValue;

	UPROPERTY()
	FLinearColor FLinearColorTargetValue;

	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "AttributeType == EPostProcessAttributeType::FLinearColor"))
	float floatValue = 0;

	UPROPERTY()
	float floatBaseValue = 0;

	UPROPERTY(EditAnywhere, meta = (EditConditionHides, EditCondition = "AttributeType == EPostProcessAttributeType::Float && !bUseBaseValueAsReturnValue"))
	float floatReturnValue = 0;
	
	UPROPERTY()
	float floatTargetValue = 0;

	bool bIsBaseValueInitialized = false;
	bool bIsTargetValueInitialized = false;

	UPROPERTY(EditAnywhere)
	bool bUseBaseValueAsReturnValue = false; 
	
	UPROPERTY(EditAnywhere)
	float EffectDelay = 0;
	
	UPROPERTY()
	float DelayTimer = 0;
	
	UPROPERTY(EditAnywhere)
	float EffectLength = 0;
	
	UPROPERTY()
	float EffectTimer = 0;

	bool bIsCompleted = false;
	
	TFunction<void(FPostProcessSettings& PostProcessSettings, FPostProcessControllerConfig& Config, float DeltaTime)> Process;
	
	TFunction<void(FPostProcessSettings& PostProcessSettings, FPostProcessControllerConfig& Config)> End;
	
	void PickProcessFunction()
	{
		switch (Attribute) {
			POST_PROCESS_ATTRIBUTE_CASE(float, WhiteTemp)
			POST_PROCESS_ATTRIBUTE_CASE(FLinearColor, SceneColorTint)
			POST_PROCESS_ATTRIBUTE_CASE(FVector4, ColorSaturation)
		}
	}
};

USTRUCT(BlueprintType)
struct FPostProcessControllerConfigContainer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPPFXGlobalEvent GlobalEventType;
	
	UPROPERTY(EditAnywhere)
	int Priority = 0;

	UPROPERTY(EditAnywhere)
	bool bTerminateOtherRunningConfigsOnActivate = false;

	UPROPERTY(EditAnywhere)
	TArray<FPostProcessControllerConfig> Configs;
};

USTRUCT()
struct FPostProcessControllerConfigContainers
{
	GENERATED_BODY()
	
	TArray<FPostProcessControllerConfigContainer> ConfigContainers;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PVD_API UPVDPostProcessController : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPVDPostProcessController();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	
	UFUNCTION()
	void CategorizeConfigsWithEvents();
	
	UFUNCTION()
	void Run(FPostProcessControllerConfigContainers Containers);

	void Run(FPostProcessControllerConfig& Config);
	
	UFUNCTION()
	void ProcessConfigs(float DeltaTime);

private:
	FPostProcessSettings* PostProcessSettings;
	
	UPROPERTY(EditAnywhere)
	TArray<FPostProcessControllerConfigContainer> ConfigContainers;
	
	FPostProcessControllerConfig* ActiveRunningPostProcessControllerConfig = nullptr;
	
	UPROPERTY()
	TMap<EPPFXGlobalEvent, FPostProcessControllerConfigContainers> EventConfigContainerMap;
	
	UPROPERTY()
	int CurrentPriorityLevel;

	TArray<FPostProcessControllerConfig> RunningPostProcessControllerConfigs;
};