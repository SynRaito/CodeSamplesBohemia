#include "PVD/PVD.h"
#include "../Components/PVDMaterialEffectControllerComp.h"
#include "GESDataTypes.h"
#include "GESHandler.h"
#include "Camera/CameraComponent.h"
#include "Curves/CurveLinearColor.h"
#include "PVD/Characters/PVDCharacter.h"
#include "Components/ActorComponent.h"
#include "PVD/Data/MaterialEffectConfigDataAsset.h"

UPVDMaterialEffectControllerComp::UPVDMaterialEffectControllerComp()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPVDMaterialEffectControllerComp::BeginPlay()
{
	Super::BeginPlay();
	
	/* Bind configs with GES events */
	CategorizeConfigsWithEvents();

	/** Handle any Begin Play triggers, Map will check if any trigger is set as BeginPlay*/
	GES_MATERIAL_EFFECT_EMIT(EMatFXGlobalEvent::MatFX_BeginPlay, GetOwner());
	
}

void UPVDMaterialEffectControllerComp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	/** Unbind from GES events */ 
	FGESHandler::DefaultHandler()->RemoveAllListenersForReceiver(this);
}

void UPVDMaterialEffectControllerComp::TickComponent(float DeltaTime, ELevelTick TickType,
                                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ProcessMaterialsChanges(DeltaTime);
	ProcessParameterChanges(DeltaTime);
}

/** Trigger setup and run */
void UPVDMaterialEffectControllerComp::CategorizeConfigsWithEvents()
{
	for (auto DataAssetConfigList : DataAssetConfigs)
	{
		for (auto Config : DataAssetConfigList->ConfigList)
		{
			Configs.Add(Config);
		}
	}
	
	for (auto Config : Configs)
	{
		if(!EventConfigMap.Contains(Config.GlobalEventType))
		{
			EventConfigMap.Add(Config.GlobalEventType);
		}
		
		EventConfigMap[Config.GlobalEventType].Configs.Add(Config);

		GES_MATERIAL_EFFECT_EVENT_CONTEXT(Config.GlobalEventType);
		FGESHandler::DefaultHandler()->AddLambdaListener(GESEventContext, [this,Config] (UObject* InTarget)
		{
			if(InTarget == GetOwner())
			{
				Run(EventConfigMap[Config.GlobalEventType].Configs);
				OnConfigRunnedWithGES.Broadcast(Config.GlobalEventType);
			}
		});
	}
}

void UPVDMaterialEffectControllerComp::RunConfigWithParameter(EMatFXGlobalEvent Type)
{
	for (auto Config : Configs)
	{
		if (Config.GlobalEventType == Type)
		{
			Run(EventConfigMap[Config.GlobalEventType].Configs);
		}
	}
}

void UPVDMaterialEffectControllerComp::Run(TArray<FMaterialEffectConfig> ConfigArray)
{
	for (FMaterialEffectConfig Config : ConfigArray)
	{
		Run(Config);
	}
}

void UPVDMaterialEffectControllerComp::Run(FMaterialEffectConfig& Config)
{
	TArray<UMeshComponent*> MeshComponents = GetMeshes(Config);
	switch (Config.MaterialEffectType)
	{
	case EMaterialEffectType::OverrideMaterial:
		for (UMeshComponent* MeshComponent : MeshComponents)
		{
			CreateMaterialChangeHandler(Config, Config.EffectMaterial, MeshComponent);
		}
		break;
	case EMaterialEffectType::ChangeParameters:
		if(Config.IsCameraPostProcessMaterial)
		{
			SetParametersOfPostProcessMaterials(Config);
		}
		for (UMeshComponent* MeshComponent : MeshComponents)
		{
			CreateParameterChangeHandler(Config, MeshComponent);
		}
		break;
	}
}

void UPVDMaterialEffectControllerComp::ProcessParameterChanges(float DeltaTime)
{
	for (auto ParameterChangeHandlers : ParameterChangeHandlersMap)
	{
		UParameterChangeHandler* PriorParameterChangeHandler = EvaluatePriorParameterChangeHandler(ParameterChangeHandlers);

		if (PriorParameterChangeHandler == nullptr)
		{
			continue;
		}
		
		GarbageCollectionCheckForParameterChanges(ParameterChangeHandlers, PriorParameterChangeHandler);

		for(auto ParameterChangeHandler : ParameterChangeHandlers.Value->Array)
		{
			if (ParameterChangeHandler->MaterialInstance == nullptr)
			{
				InitialSetupParameterChangeHandler(ParameterChangeHandler);
			}

			//Process Delay Timer
			if (ParameterChangeHandler->Config.Delay > 0 && ParameterChangeHandler->Config.HasDelay)
			{
				if (ParameterChangeHandler->DelayCounter < ParameterChangeHandler->Config.Delay)
				{
					ParameterChangeHandler->DelayCounter += DeltaTime;
					continue;
				}
			}

			//Check lifetime and killflag
			if (ParameterChangeHandler->Config.Lifetime > 0 && ParameterChangeHandler->Config.HasLifetime)
			{
				if (ParameterChangeHandler->LifetimeCounter < ParameterChangeHandler->Config.Lifetime)
				{
					ParameterChangeHandler->LifetimeCounter += DeltaTime;
				}
				else
				{
					ParameterChangeHandler->bKillFlag = true;
					continue;
				}
			}

			//Apply config if its prior one
			if(ParameterChangeHandler->ParentConfig == PriorParameterChangeHandler->ParentConfig)
			{
				ApplyParameterChange(ParameterChangeHandler);
			}

			//Increase animation timer anyway
			if(ParameterChangeHandler->Config.IsAnimation)
			{
				ParameterChangeHandler->AnimationCounter += DeltaTime;
			}
		}
	}
}

const bool UPVDMaterialEffectControllerComp::CreateParameterChangeHandler(FMaterialEffectConfig& Config,
																	  UMeshComponent* MeshComponent)
{
	TArray<FName> SlotNames = MeshComponent->GetMaterialSlotNames();

	for (size_t index = 0; index < SlotNames.Num(); ++index)
	{
		if (!Config.EffectedSlotIds.Contains(index) && !Config.EffectAllSlots && !Config.IsOverlaySlot)
			continue;
		for (FMaterialParameterChangeConfig ParameterConfig : Config.ParameterConfigs)
		{
			UParameterChangeHandler* ParameterChangeHandler = NewObject<UParameterChangeHandler>();
			ParameterChangeHandler->Config = ParameterConfig;
			ParameterChangeHandler->ParentConfig = &Config;
			ParameterChangeHandler->EffectedMesh = MeshComponent;
			ParameterChangeHandler->IsOverlaySlot = Config.IsOverlaySlot;
			ParameterChangeHandler->Priority = Config.Priority;
			ParameterChangeHandler->SlotId = index;

			if(Config.bHasFinisherEvent)
			{
				GES_MATERIAL_EFFECT_EVENT_CONTEXT(Config.FinisherEventType);

				TWeakObjectPtr<UParameterChangeHandler> WeakParameterChangeHandler = MakeWeakObjectPtr(ParameterChangeHandler);
				ParameterChangeHandler->EventContext = GESEventContext;
				ParameterChangeHandler->LambdaName = FGESHandler::DefaultHandler()->AddLambdaListener(GESEventContext, [WeakParameterChangeHandler]()
				{
					if(WeakParameterChangeHandler.IsValid())
					{
						WeakParameterChangeHandler->bKillFlag = true;
					}
				});
			}
			
			FString KeyName = FString(MeshComponent->GetName() + (Config.IsOverlaySlot ? "-1" : FString::FromInt(index)));
			
			if(!ParameterChangeHandlersMap.Contains(KeyName))
			{
				ParameterChangeHandlersMap.Add(KeyName, NewObject<UParameterChangeHandlerArray>());
			}

			ParameterChangeHandlersMap[KeyName]->Array.Add(ParameterChangeHandler);
			ParameterChangeHandlersMap[KeyName]->Array.Sort();
		}
	}
	return true;
}

void UPVDMaterialEffectControllerComp::InitialSetupParameterChangeHandler(UParameterChangeHandler* ParameterChangeHandler)
{
	ParameterChangeHandler->MaterialInstance = CreateDynamicMaterialInstance(ParameterChangeHandler);

	switch (ParameterChangeHandler->Config.ParameterType)
	{
	case EMaterialParamType::Float:
		ParameterChangeHandler->OldFloatValue = ParameterChangeHandler->MaterialInstance->K2_GetScalarParameterValue(
			ParameterChangeHandler->Config.ParameterName);
		if(ParameterChangeHandler->Config.ReturnToDefaultValue)
		{
			ParameterChangeHandler->OldFloatValue = ParameterChangeHandler->Config.DefaultValue;
		}
		switch (ParameterChangeHandler->Config.ParameterChangeType)
		{
		case EMaterialParameterChangeType::Additive:
			ParameterChangeHandler->Config.FloatParameterValue += ParameterChangeHandler->
				OldFloatValue;
			break;

		case EMaterialParameterChangeType::Multiply:
			ParameterChangeHandler->Config.FloatParameterValue *= ParameterChangeHandler->
				OldFloatValue;
			break;
		}
		break;

	case EMaterialParamType::Color:
		ParameterChangeHandler->OldLinearColorValue = ParameterChangeHandler->MaterialInstance->K2_GetVectorParameterValue(
			ParameterChangeHandler->Config.ParameterName);

		switch (ParameterChangeHandler->Config.ParameterChangeType)
		{
		case EMaterialParameterChangeType::Additive:
			ParameterChangeHandler->Config.LinearColorParameterValue += ParameterChangeHandler->
				OldLinearColorValue;
			break;
		case EMaterialParameterChangeType::Multiply:
			ParameterChangeHandler->Config.LinearColorParameterValue *= ParameterChangeHandler->
				OldLinearColorValue;
			break;
		}

		break;

	case EMaterialParamType::Texture:
		ParameterChangeHandler->OldTextureValue = ParameterChangeHandler->MaterialInstance->K2_GetTextureParameterValue(
			ParameterChangeHandler->Config.ParameterName);
		break;
	}
}

UParameterChangeHandler* UPVDMaterialEffectControllerComp::EvaluatePriorParameterChangeHandler(TTuple<FString, UParameterChangeHandlerArray*> ParameterChangeHandlers)
{
	if(!PriorParameterChangeHandlerMap.Contains(ParameterChangeHandlers.Key))
	{
		PriorParameterChangeHandlerMap.Add(ParameterChangeHandlers.Key, NewObject<UParameterChangeHandler>());
	}

	UParameterChangeHandler* PriorParameterChangeHandler = nullptr;
		
	if(!ParameterChangeHandlers.Value->Array.IsEmpty() &&
	   ParameterChangeHandlers.Value->Array[0] != PriorParameterChangeHandlerMap[ParameterChangeHandlers.Key])
	{
		if(PriorParameterChangeHandlerMap[ParameterChangeHandlers.Key] != nullptr)
		{
			PriorParameterChangeHandlerMap[ParameterChangeHandlers.Key]->ApplyOldValues();
		}
		PriorParameterChangeHandlerMap[ParameterChangeHandlers.Key] = ParameterChangeHandlers.Value->Array[0];
		PriorParameterChangeHandler = ParameterChangeHandlers.Value->Array[0];
	}
	else if(ParameterChangeHandlers.Value->Array.IsEmpty())
	{
		PriorParameterChangeHandlerMap.Remove(ParameterChangeHandlers.Key);
	}
	else
	{
		PriorParameterChangeHandler = ParameterChangeHandlers.Value->Array[0];
	}

	return PriorParameterChangeHandler;
}

float UPVDMaterialEffectControllerComp::CalculateAnimatedParameterConfigCurveTime(UParameterChangeHandler* ParameterChangeHandler)
{
	if (ParameterChangeHandler->Config.bLoopAnimation)
		return NumericMod(ParameterChangeHandler->AnimationCounter, ParameterChangeHandler->Config.AnimationTime) / ParameterChangeHandler->Config.AnimationTime;
	else
		return FMath::Clamp(ParameterChangeHandler->AnimationCounter / ParameterChangeHandler->Config.AnimationTime, 0.0f, 1.0f);
}

void UPVDMaterialEffectControllerComp::ApplyParameterChange(UParameterChangeHandler* ParameterChangeHandler)
{
	if (ParameterChangeHandler->Config.IsAnimation && ParameterChangeHandler->Config.ParameterType != EMaterialParamType::Texture)
	{
		float FloatCurveValue;
		FLinearColor ColorCurveValue;
		
		float CurveValueTime = CalculateAnimatedParameterConfigCurveTime(ParameterChangeHandler);
		
		switch (ParameterChangeHandler->Config.ParameterType)
		{
		case EMaterialParamType::Float:
			
			FloatCurveValue = ParameterChangeHandler->Config.FloatCurve->GetFloatValue(CurveValueTime);
			
			if(IsValid(ParameterChangeHandler->MaterialInstance))
			{
				ParameterChangeHandler->MaterialInstance->SetScalarParameterValue(
				ParameterChangeHandler->Config.ParameterName, FMath::Lerp(
					ParameterChangeHandler->OldFloatValue,
					ParameterChangeHandler->Config.FloatParameterValue,
					FloatCurveValue));
			}
			break;
			
		case EMaterialParamType::Color:
			
			ColorCurveValue = ParameterChangeHandler->Config.ColorCurve->GetLinearColorValue(CurveValueTime);
			
			if(IsValid(ParameterChangeHandler->MaterialInstance))
			{
				ParameterChangeHandler->MaterialInstance->SetVectorParameterValue(
					ParameterChangeHandler->Config.ParameterName, FMath::Lerp(
						ParameterChangeHandler->OldLinearColorValue,
						ParameterChangeHandler->Config.LinearColorParameterValue,
						ColorCurveValue));
			}
			break;
		}
	}
	else
	{
		switch (ParameterChangeHandler->Config.ParameterType)
		{
		case EMaterialParamType::Float:
			if(IsValid(ParameterChangeHandler->MaterialInstance))
			{
				ParameterChangeHandler->MaterialInstance->SetScalarParameterValue(
					ParameterChangeHandler->Config.ParameterName,
					ParameterChangeHandler->Config.FloatParameterValue);
			}
			break;
		case EMaterialParamType::Color:
			if(IsValid(ParameterChangeHandler->MaterialInstance))
			{
				ParameterChangeHandler->MaterialInstance->SetVectorParameterValue(
					ParameterChangeHandler->Config.ParameterName,
					ParameterChangeHandler->Config.LinearColorParameterValue);
			}
			break;
		case EMaterialParamType::Texture:
			if(IsValid(ParameterChangeHandler->MaterialInstance))
			{
				ParameterChangeHandler->MaterialInstance->SetTextureParameterValue(
					ParameterChangeHandler->Config.ParameterName,
					ParameterChangeHandler->Config.TextureParameterValue);
			}
			break;
		}
	}
}

const bool UPVDMaterialEffectControllerComp::SetParametersOfPostProcessMaterials(FMaterialEffectConfig& Config)
{
	if(Config.IsCameraPostProcessMaterial)
	{
		const APVDCharacter* Character = Cast<APVDCharacter>(GetOwner());
		if(Character != nullptr)
		{
			for (FMaterialParameterChangeConfig ParameterConfig : Config.ParameterConfigs)
			{
				for (size_t index = 0; index < Config.EffectedSlotIds.Num(); ++index)
				{
					UParameterChangeHandler* ParameterChangeHandler = NewObject<UParameterChangeHandler>();
					ParameterChangeHandler->Config = ParameterConfig;
					ParameterChangeHandler->Config = ParameterConfig;
					ParameterChangeHandler->IsCameraPostProcessMaterial = true;
					ParameterChangeHandler->SlotId = Config.EffectedSlotIds[index];
					ParameterChangeHandler->Priority = Config.Priority;

					if(!Config.HasLifetime)
					{
						GES_MATERIAL_EFFECT_EVENT_CONTEXT(Config.FinisherEventType);
						
						TWeakObjectPtr<UParameterChangeHandler> WeakParameterChangeHandler = MakeWeakObjectPtr(ParameterChangeHandler);
						ParameterChangeHandler->EventContext = GESEventContext;
						ParameterChangeHandler->LambdaName = FGESHandler::DefaultHandler()->AddLambdaListener(GESEventContext, [WeakParameterChangeHandler]()
						{
							if(WeakParameterChangeHandler.IsValid())	
							{
								WeakParameterChangeHandler->bKillFlag = true;
							}
						});
					}
					
					FString KeyName = FString("PostProcess" + (Config.IsOverlaySlot ? "-1" : FString::FromInt(index)));
			
					if(!ParameterChangeHandlersMap.Contains(KeyName))
					{
						ParameterChangeHandlersMap.Add(KeyName, NewObject<UParameterChangeHandlerArray>());
					}
		
					ParameterChangeHandlersMap[KeyName]->Array.Add(ParameterChangeHandler);
					
					ParameterChangeHandlersMap[KeyName]->Array.Sort();
				}
			}
		}
	}
	
	return true;
}

void UPVDMaterialEffectControllerComp::GarbageCollectionCheckForParameterChanges(TTuple<FString, UParameterChangeHandlerArray*> ParameterChangeHandlers, UParameterChangeHandler* PriorParameterChangeHandler)
{
	for(int i = ParameterChangeHandlers.Value->Array.Num() - 1; i >= 0; --i)
	{
		auto Obj = ParameterChangeHandlers.Value->Array[i];
		if(Obj->bKillFlag)
		{
			if(Obj->ParentConfig == PriorParameterChangeHandler->ParentConfig)
				Obj->ApplyOldValues();
			ParameterChangeHandlers.Value->Array.Remove(Obj);
			FGESHandler::DefaultHandler()->RemoveLambdaListener(Obj->EventContext, Obj->LambdaName);
			if(PriorParameterChangeHandlerMap[ParameterChangeHandlers.Key] == Obj)
			{
				PriorParameterChangeHandlerMap.Remove(ParameterChangeHandlers.Key);
			}
		}
	}
}

void UPVDMaterialEffectControllerComp::ProcessMaterialsChanges(float DeltaTime)
{
	for (int index = 0; index < MaterialChangeHandlers.Num(); ++index)
	{
		//Process delay timer
		if (MaterialChangeHandlers[index]->Delay > 0 && MaterialChangeHandlers[index]->HasDelay)
		{
			if (MaterialChangeHandlers[index]->DelayCounter < MaterialChangeHandlers[index]->Delay)
			{
				MaterialChangeHandlers[index]->DelayCounter += DeltaTime;
				continue;
			}
		}

		//Apply new material
		if (!MaterialChangeHandlers[index]->IsApplied)
		{
			if (MaterialChangeHandlers[index]->IsOverlaySlot)
			{
				MaterialChangeHandlers[index]->EffectedMesh->SetOverlayMaterial(MaterialChangeHandlers[index]->NewMaterial);
			}
			else
			{
				MaterialChangeHandlers[index]->EffectedMesh->SetMaterial(MaterialChangeHandlers[index]->SlotId,
																		MaterialChangeHandlers[index]->NewMaterial);
			}

			MaterialChangeHandlers[index]->IsApplied = true;
		}

		//Check lifetime
		if (MaterialChangeHandlers[index]->Lifetime > 0 && MaterialChangeHandlers[index]->HasLifetime)
		{
			if (MaterialChangeHandlers[index]->LifetimeCounter < MaterialChangeHandlers[index]->Lifetime)
			{
				MaterialChangeHandlers[index]->LifetimeCounter += DeltaTime;
			}
			else if(MaterialChangeHandlers[index]->HasLifetime)
			{
				MaterialChangeHandlers[index]->bKillFlag = true;
			}
		}
	}

	GarbageCollectionCheckForMaterialChanges();
}

const bool UPVDMaterialEffectControllerComp::CreateMaterialChangeHandler(FMaterialEffectConfig& Config,
                                                                UMaterialInterface* Material,
                                                                UMeshComponent* MeshComponent)
{
	TArray<FName> SlotNames = MeshComponent->GetMaterialSlotNames();

	for (size_t index = 0; index < SlotNames.Num(); ++index)
	{
		if (!Config.EffectedSlotIds.Contains(index) && !Config.EffectAllSlots && !Config.IsOverlaySlot)
			continue;

		UMaterialChangeHandler* MaterialChangeHandler = NewObject<UMaterialChangeHandler>();
		if (Config.HasDelay){
			MaterialChangeHandler->Delay = Config.Delay;
			MaterialChangeHandler->HasDelay = Config.HasDelay;
        }
		if (Config.HasLifetime){
			MaterialChangeHandler->Lifetime = Config.Lifetime;
			MaterialChangeHandler->HasLifetime = Config.HasLifetime;
        }
		if(Config.IsOverlaySlot)
			MaterialChangeHandler->OldMaterial = MeshComponent->GetOverlayMaterial();
		else
			MaterialChangeHandler->OldMaterial = MeshComponent->GetMaterial(index);
			
		MaterialChangeHandler->NewMaterial = Material;
		MaterialChangeHandler->EffectedMesh = MeshComponent;
		MaterialChangeHandler->IsOverlaySlot = Config.IsOverlaySlot;
		MaterialChangeHandler->SlotId = index;

		if(Config.bHasFinisherEvent)
		{
			GES_MATERIAL_EFFECT_EVENT_CONTEXT(Config.FinisherEventType);
			TWeakObjectPtr<UMaterialChangeHandler> WeakMaterialChangeHandler = MakeWeakObjectPtr(MaterialChangeHandler);
			
			MaterialChangeHandler->EventContext = GESEventContext;
			MaterialChangeHandler->LambdaName = FGESHandler::DefaultHandler()->AddLambdaListener(GESEventContext, [WeakMaterialChangeHandler]()
			{
				if(WeakMaterialChangeHandler.IsValid())
				{
					WeakMaterialChangeHandler->bKillFlag = true;
				}
			});
		}
		
		MaterialChangeHandlers.Add(MaterialChangeHandler);
	}
	return true;
}

void UPVDMaterialEffectControllerComp::GarbageCollectionCheckForMaterialChanges()
{
	for (int Index = MaterialChangeHandlers.Num() - 1; Index >= 0; --Index)
	{
		if(MaterialChangeHandlers[Index]->bKillFlag)
		{
			if (MaterialChangeHandlers[Index]->OldMaterial)
			{
				if (MaterialChangeHandlers[Index]->IsOverlaySlot)
				{
					MaterialChangeHandlers[Index]->EffectedMesh->SetOverlayMaterial(MaterialChangeHandlers[Index]->OldMaterial);
				}
				else
				{
					MaterialChangeHandlers[Index]->EffectedMesh->SetMaterial(MaterialChangeHandlers[Index]->SlotId,
																			MaterialChangeHandlers[Index]->OldMaterial);
				}
			}
			
			FGESHandler::DefaultHandler()->RemoveLambdaListener(MaterialChangeHandlers[Index]->EventContext, MaterialChangeHandlers[Index]->LambdaName);
			MaterialChangeHandlers.RemoveAt(Index);
		}
	}
}

UMaterialInstanceDynamic* UPVDMaterialEffectControllerComp::CreateDynamicMaterialInstance(
	UParameterChangeHandler* ParameterChangeHandler)
{
	UMaterialInstanceDynamic* MaterialInstance = nullptr;
			
	if (ParameterChangeHandler->IsOverlaySlot)
	{
		MaterialInstance = Cast<UMaterialInstanceDynamic>(
			ParameterChangeHandler->EffectedMesh->GetOverlayMaterial());
	}
	else if(ParameterChangeHandler->IsCameraPostProcessMaterial)
	{
		const APVDCharacter* Character = Cast<APVDCharacter>(GetOwner());
		if(Character != nullptr)
		{
			UCameraComponent* CameraComponent = Character->GetCameraComponent();
			auto WeightedBlendablesArray = CameraComponent->PostProcessSettings.WeightedBlendables.Array;
			MaterialInstance = Cast<UMaterialInstanceDynamic>(
				WeightedBlendablesArray[ParameterChangeHandler->SlotId].Object);
		}
	}
	else
	{
		MaterialInstance = Cast<UMaterialInstanceDynamic>(
			ParameterChangeHandler->EffectedMesh->GetMaterial(ParameterChangeHandler->SlotId));
	}
	if (MaterialInstance == nullptr)
	{
		if (ParameterChangeHandler->IsOverlaySlot)
		{
			MaterialInstance = UMaterialInstanceDynamic::Create(
				ParameterChangeHandler->EffectedMesh->GetOverlayMaterial(), this);
			ParameterChangeHandler->EffectedMesh->SetOverlayMaterial(MaterialInstance);
		}
		else if (ParameterChangeHandler->IsCameraPostProcessMaterial)
		{
			const APVDCharacter* Character = Cast<APVDCharacter>(GetOwner());
			if(Character != nullptr)
			{
				UCameraComponent* CameraComponent = Character->GetCameraComponent();
				auto WeightedBlendableArray = CameraComponent->PostProcessSettings.WeightedBlendables.Array;
				MaterialInstance = UMaterialInstanceDynamic::Create(
					Cast<UMaterialInterface>(WeightedBlendableArray[ParameterChangeHandler->SlotId].Object), this);
				WeightedBlendableArray[ParameterChangeHandler->SlotId].Object = MaterialInstance;
				CameraComponent->PostProcessSettings.WeightedBlendables.Array = WeightedBlendableArray;
			}
		}
		else
		{
			MaterialInstance = UMaterialInstanceDynamic::Create(
				ParameterChangeHandler->EffectedMesh->GetMaterial(ParameterChangeHandler->SlotId), this);
			ParameterChangeHandler->EffectedMesh->SetMaterial(ParameterChangeHandler->SlotId, MaterialInstance);
		}
	}

	return MaterialInstance;
}

const TArray<UMeshComponent*> UPVDMaterialEffectControllerComp::GetMeshes(const FMaterialEffectConfig& Config)
{
	TArray<UMeshComponent*> MeshComponents;
	GetOwner()->GetComponents<UMeshComponent>(MeshComponents);

	if (!Config.EffectAllMeshes)
	{
		TArray<UMeshComponent*> FilteredMeshComponents;
		for (UMeshComponent* MeshComponent : MeshComponents)
		{
			if (Config.EffectedMeshNameList.Contains(MeshComponent->GetName()) && !Config.ExcludedMeshNameList.Contains(MeshComponent->GetName()))
			{
				FilteredMeshComponents.Add(MeshComponent);
			}
		}
		return FilteredMeshComponents;
	}
	//else
	for (int i = MeshComponents.Num()-1; i >= 0; i--)
	{
		if (Config.ExcludedMeshNameList.Contains(MeshComponents[i]->GetName()))
		{
			MeshComponents.RemoveAt(i);
		}
	}
	return MeshComponents;
}