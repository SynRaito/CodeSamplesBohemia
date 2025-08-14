#pragma once

#include "CoreMinimal.h"
#include "PerkDataAsset.h"
#include "PerkTreeClasses.generated.h"

class UPerkManagementComponent;
class APVDCharacter;
class UPerkTreeNodeWidget;
class UPerkTreeRowDataAsset;
class UPerkTreeDataAsset;

UENUM(BlueprintType)
enum class ESkillTreeSection : uint8
{
	None,
	Offensive,
	Defensive,
	Utility
};

UCLASS()
class UPerkTreeNodeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	void Deserialize();
	int GetCurrentLevel() const;
	void SetCurrentLevel(int Value, bool serializeInSaveData = true);

private:
	void GainPerk() const;
	void GainPerk(int count) const;

public:
	UPROPERTY(EditAnywhere)
	FName PerkName;
	
	UPROPERTY(EditAnywhere)
	FGameplayTag TutorialActivateInfoTag;
	
	UPROPERTY(EditAnywhere)
	ESkillTreeSection SkillTreeSection;

	UPROPERTY(EditAnywhere)
	UTexture2D* PerkImage;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UPerkDataAsset> PerkDataAsset;

	UPROPERTY(EditAnywhere)
	FVector2D ParentNodePosition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=PVD)
	int MaxLevel;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPerkTreeNodeDataAsset> ParentNode;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UPerkTreeNodeDataAsset>> ChildNodes;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPerkTreeRowDataAsset> PerkTreeRow;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPerkTreeNodeWidget> PerkTreeNodeWidget;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=PVD, meta=(AllowPrivateAccess))
	int CurrentLevel = 0;
};

UCLASS()
class UPerkTreeRowDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	TArray<UPerkTreeNodeDataAsset*> PerkTreeNodes;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPerkTreeRowDataAsset> ParentRow;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UPerkTreeRowDataAsset> ChildRow;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPerkTreeDataAsset> PerkTree;
};

UCLASS()
class UPerkTreeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	FORCEINLINE UPerkTreeNodeDataAsset* GetNode(size_t Row, size_t Column) { return PerkTreeRows[Row]->PerkTreeNodes[Column]; }
	void Deserialize();

	UFUNCTION(BlueprintCallable)
	void ClearDataProgress(bool ShouldSave);
	
	UPROPERTY(EditAnywhere)
	TArray<UPerkTreeRowDataAsset*> PerkTreeRows;
};

UENUM()
enum class EPerkTreeNodeLevelState
{
	None,
	Some,
	Max
};
