#include "PVDPostProcessController.h"
#include "GESDataTypes.h"
#include "GESHandler.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PVD/PVD.h"
#include "PVD/Characters/PVDCharacter.h"


UPVDPostProcessController::UPVDPostProcessController()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPVDPostProcessController::BeginPlay()
{
	Super::BeginPlay();

	if(auto Character = Cast<APVDCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn()))
	{
		if(auto CameraComponent = Character->GetCameraComponent())
		{
			PostProcessSettings = &CameraComponent->PostProcessSettings;
		}
	}
	
	CategorizeConfigsWithEvents();
	
	/** Handle any Begin Play triggers, Map will check if any trigger is set as BeginPlay*/
	GES_POSTPROCESS_EFFECT_EMIT(EPPFXGlobalEvent::PPFX_BeginPlay, GetOwner());
}

void UPVDPostProcessController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	/** Unbind from GES events */ 
	FGESHandler::DefaultHandler()->RemoveAllListenersForReceiver(this);
}

void UPVDPostProcessController::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	ProcessConfigs(DeltaTime);
}

/** Trigger setup and run */
void UPVDPostProcessController::CategorizeConfigsWithEvents()
{
	for (auto ConfigContainer : ConfigContainers)
	{
		if(!EventConfigContainerMap.Contains(ConfigContainer.GlobalEventType))
		{
			EventConfigContainerMap.Add(ConfigContainer.GlobalEventType);
		}
	
		EventConfigContainerMap[ConfigContainer.GlobalEventType].ConfigContainers.Add(ConfigContainer);
		    
		GES_POSTPROCESS_EFFECT_EVENT_CONTEXT(ConfigContainer.GlobalEventType);
		FGESHandler::DefaultHandler()->AddLambdaListener(GESEventContext, [this,ConfigContainer] (UObject* InTarget)
		{
			if(InTarget == GetOwner())
			{
				Run(EventConfigContainerMap[ConfigContainer.GlobalEventType]);
			}
		});
	}
}

void UPVDPostProcessController::Run(FPostProcessControllerConfigContainers Containers)
{
	for (FPostProcessControllerConfigContainer ConfigContainer : Containers.ConfigContainers)
	{
		if(ConfigContainer.bTerminateOtherRunningConfigsOnActivate)
		{
			for (auto RunningPostProcessControllerConfig : RunningPostProcessControllerConfigs)
			{
				RunningPostProcessControllerConfig.End(*PostProcessSettings,RunningPostProcessControllerConfig);
			}
			RunningPostProcessControllerConfigs.Empty();
		}
		for (auto Config : ConfigContainer.Configs)
		{
			Config.PickProcessFunction();
			Config.Priority = ConfigContainer.Priority;
			Run(Config);
		}
	}
}

void UPVDPostProcessController::Run(FPostProcessControllerConfig& Config)
{
	RunningPostProcessControllerConfigs.Add(Config);
}

void UPVDPostProcessController::ProcessConfigs(float DeltaTime)
{
	for (int iterator = RunningPostProcessControllerConfigs.Num() - 1; iterator >= 0; --iterator)
	{
		FPostProcessControllerConfig &RunningPostProcessControllerConfig = RunningPostProcessControllerConfigs[iterator];

		//Process Delay Timer
		if(RunningPostProcessControllerConfig.DelayTimer < RunningPostProcessControllerConfig.EffectDelay)
		{
			RunningPostProcessControllerConfig.EffectDelay += DeltaTime;
			continue;
		}
		
		RunningPostProcessControllerConfig.EffectTimer += DeltaTime;
		
		if(RunningPostProcessControllerConfig.EffectLength >= RunningPostProcessControllerConfig.EffectTimer)
		{
			if(CurrentPriorityLevel <= RunningPostProcessControllerConfig.Priority)
			{
				CurrentPriorityLevel = RunningPostProcessControllerConfig.Priority;
				if(ActiveRunningPostProcessControllerConfig != nullptr)
				{
					switch (ActiveRunningPostProcessControllerConfig->AttributeType)
					{
					case EPostProcessAttributeType::FVector4:
						if(ActiveRunningPostProcessControllerConfig->bUseBaseValueAsReturnValue)
							RunningPostProcessControllerConfig.FVector4BaseValue = ActiveRunningPostProcessControllerConfig->FVector4BaseValue;
						else
							RunningPostProcessControllerConfig.FVector4BaseValue = ActiveRunningPostProcessControllerConfig->FVector4ReturnValue;
						break;
					case EPostProcessAttributeType::Float:
						if(ActiveRunningPostProcessControllerConfig->bUseBaseValueAsReturnValue)
							RunningPostProcessControllerConfig.floatBaseValue = ActiveRunningPostProcessControllerConfig->floatBaseValue;
						else
							RunningPostProcessControllerConfig.floatBaseValue = ActiveRunningPostProcessControllerConfig->floatReturnValue;
						break;
					case EPostProcessAttributeType::FLinearColor:
						if(ActiveRunningPostProcessControllerConfig->bUseBaseValueAsReturnValue)
							RunningPostProcessControllerConfig.FLinearColorBaseValue = ActiveRunningPostProcessControllerConfig->FLinearColorBaseValue;
						else
							RunningPostProcessControllerConfig.FLinearColorBaseValue = ActiveRunningPostProcessControllerConfig->FLinearColorReturnValue;
						break;
					}
					RunningPostProcessControllerConfig.bIsBaseValueInitialized = true;
				} 
				ActiveRunningPostProcessControllerConfig = &RunningPostProcessControllerConfig;
			}
		}
		else
		{
			RunningPostProcessControllerConfig.End(*PostProcessSettings, *ActiveRunningPostProcessControllerConfig);
			if(&RunningPostProcessControllerConfig == ActiveRunningPostProcessControllerConfig)
			{
				ActiveRunningPostProcessControllerConfig = nullptr;
			}
			RunningPostProcessControllerConfigs.RemoveAt(iterator);
		}
	}

	if(ActiveRunningPostProcessControllerConfig != nullptr && !ActiveRunningPostProcessControllerConfig->bIsCompleted)
	{
		ActiveRunningPostProcessControllerConfig->Process(*PostProcessSettings, *ActiveRunningPostProcessControllerConfig, DeltaTime);
	}
}