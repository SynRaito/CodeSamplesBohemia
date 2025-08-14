#include "PerkTreeClasses.h"
#include "Kismet/GameplayStatics.h"
#include "PVD/PVDBlueprintFunctions.h"
#include "PVD/PVDGameInstance.h"
#include "PVD/PVDSaveGame.h"
#include "PVD/Characters/PVDCharacter.h"
#include "PVD/Components/PerkManagementComponent.h"

void UPerkTreeNodeDataAsset::Deserialize()
{
	FString name = PerkName.ToString();
	if (UPVDGameInstance::GetSave()->PerkSaveData.PerkLevels.Contains(name))
	{
		SetCurrentLevel(UPVDGameInstance::GetSave()->PerkSaveData.PerkLevels[name], false);
	}
}

int UPerkTreeNodeDataAsset::GetCurrentLevel() const
{
	return CurrentLevel;
}

void UPerkTreeNodeDataAsset::SetCurrentLevel(int Value, bool serializeInSaveData)
{
	// gain perk if only progressing, else save will handle the clean-up
	if (Value > CurrentLevel)
	{
		GainPerk(Value);
	}

	CurrentLevel = Value;
	if (serializeInSaveData)
	{
		UPVDGameInstance::GetSave()->PerkSaveData.HandleCurrentLevelChanged(PerkName.GetPlainNameString(), CurrentLevel);
	}
}

void UPerkTreeNodeDataAsset::GainPerk() const
{
	if (const auto TheWorld = UPVDBlueprintFunctions::GetWorldContextFromViewport())
	{
		if (const APVDCharacter* Character = Cast<APVDCharacter>(UGameplayStatics::GetPlayerCharacter(TheWorld, 0)))
		{
			if (const auto PlayerPerkActorComp = Character->GetPerkActorComponent())
			{
				PlayerPerkActorComp->GainPerk(PerkDataAsset.Get(), EPerkPoolType::TreePerk);
			}
		}
	}
	else
	{
		PVD_LOG(Error, TEXT("Could not find the world!"));
	}
}

void UPerkTreeNodeDataAsset::GainPerk(int count) const
{
	if (const auto TheWorld = UPVDBlueprintFunctions::GetWorldContextFromViewport())
	{
		if (const APVDCharacter* Character = Cast<APVDCharacter>(UGameplayStatics::GetPlayerCharacter(TheWorld, 0)))
		{
			if (const auto PlayerPerkActorComp = Character->GetPerkActorComponent())
			{
				PlayerPerkActorComp->GainPerk(PerkDataAsset.Get(), EPerkPoolType::TreePerk, count);
			}
		}
	}
	else
	{
		PVD_LOG(Error, TEXT("Could not find the world!"));
	}
}

void UPerkTreeDataAsset::Deserialize()
{
	for (UPerkTreeRowDataAsset* PerkTreeRow : PerkTreeRows)
	{
		for (UPerkTreeNodeDataAsset* PerkTreeNode : PerkTreeRow->PerkTreeNodes)
		{
			PerkTreeNode->Deserialize();
		}
	}
}

void UPerkTreeDataAsset::ClearDataProgress(bool ShouldSave)
{
	for (auto PerkTreeRow : PerkTreeRows)
	{
		for (auto PerkTreeNode : PerkTreeRow->PerkTreeNodes)
		{
			PerkTreeNode->SetCurrentLevel(0, ShouldSave);
			PerkTreeNode->ChildNodes.Empty();
		}
	}
}
