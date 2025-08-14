#pragma once

#include "CoreMinimal.h"
#include "PVDCommonButton.h"
#include "PerkTreeNodeWidget.generated.h"

class UDivineOfferingWidget;
class UPerkTreeNodeDataAsset;
class UTextBlock;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAvailabilityToggled, const UPerkTreeNodeWidget*, PerkTreeNodeWidget, bool, IsAvailable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLevelRequieredDelegate, const UPerkTreeNodeWidget*, PerkTreeNodeWidget, int, CurrentLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRefundRequestDelegate, const UPerkTreeNodeWidget*, PerkTreeNodeWidget);

UCLASS()
class PVD_API UPerkTreeNodeWidget : public UPVDCommonButton
{
	GENERATED_BODY()

private:
	TWeakObjectPtr<class UPVDPlayerProgressComponent> PlayerProgressComponentPtr;

	UPROPERTY()
	UDivineOfferingWidget* DivineOfferingWidgetRef;
	
	bool IsAvailable;
	
public:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = PVD)
	UImage* Image;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = PVD)
	UImage* Background;
	
	/** Level Indicator Images */
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = PVD)
	UImage* Level1;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = PVD)
	UImage* Level2;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = PVD)
	UImage* Level3;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = PVD)
	UImage* Level4;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category = PVD)
	UImage* Level5;
	
	UPROPERTY(EditAnywhere)
	FLinearColor NodeColor;

	UPROPERTY(EditAnywhere)
	FLinearColor HighlightColor = FColor::White;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UTexture2D> LevelFilledTexture;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UTexture2D> LevelUnfilledTexture;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UFMODEvent>> LevelSFX;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UFMODEvent> LevelMaxSFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor UnavailableColor{0.15f, 0.15f, 0.15f, 1.f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor AvailableColor{1.f, 1.f, 1.f, 1.f};
	
	UPROPERTY(BlueprintReadOnly)
	UPerkTreeNodeDataAsset* PerkTreeNodeData;

	UPROPERTY(BlueprintAssignable, Category=PVD)
	FLevelRequieredDelegate OnLevelRequired;

	UPROPERTY(BlueprintAssignable, Category=PVD)
	FRefundRequestDelegate OnRefundRequest;

	UPROPERTY(BlueprintAssignable, Category=PVD)
	FAvailabilityToggled OnAvailabilityToggled;
	
	UPROPERTY(EditAnywhere)
	UImage* ConnectionImage;
	
	UPROPERTY(EditAnywhere)
	FDataTableRowHandle RefundInputAction;
	
	FUIActionBindingHandle RefundInputHandle;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

	void Init();
	void UpdateState();
	uint8 GetPurchaseCost() const;
	uint8 GetTotalInvested() const;
	bool CanPurchase() const;
	bool CanRefund() const;
	void ToggleHighlight(bool bHighlight) const;
	void SetAvailable(bool Value);
	bool TryIncreaseLevel() const;
	bool TryRefund();
	void CheckChildNode() const;
	
	UFUNCTION(BlueprintCallable)
	void BtnSelectClick();
};
