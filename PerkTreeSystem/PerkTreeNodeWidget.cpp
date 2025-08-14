#include "PerkTreeNodeWidget.h"
#include "DivineOfferingWidget.h"
#include "GESHandler.h"
#include "PVDCommonButton.h"
#include "PVD/Data/PerkTreeClasses.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetInputLibrary.h"
#include "PVD/PVDPlayerState.h"
#include "Input/CommonUIInputTypes.h"
#include "PVD/Components/PVDPlayerProgressComponent.h"

void UPerkTreeNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Background->SetBrushTintColor(NodeColor);
}

void UPerkTreeNodeWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UPerkTreeNodeWidget::Init()
{
	if (const auto PlayerState = Cast<APVDPlayerState>(UGameplayStatics::GetPlayerState(GetWorld(), 0)))
	{
		if (const auto PlayerProgressComponent = PlayerState->GetPlayerProgressComponent())
		{
			PlayerProgressComponentPtr = MakeWeakObjectPtr(PlayerProgressComponent);
		}
	}

	switch (PerkTreeNodeData->MaxLevel)
	{
	case 0:
		Level1->SetVisibility(ESlateVisibility::Collapsed);
	case 1:
		Level2->SetVisibility(ESlateVisibility::Collapsed);
	case 2:
		Level3->SetVisibility(ESlateVisibility::Collapsed);
	case 3:
		Level4->SetVisibility(ESlateVisibility::Collapsed);
	case 4:
		Level5->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (PerkTreeNodeData->PerkImage != nullptr)
	{
		Image->SetBrushFromTexture(PerkTreeNodeData->PerkImage);
	}

	UpdateState();
}

void UPerkTreeNodeWidget::UpdateState()
{
	// initial root node
	if (PerkTreeNodeData->ParentNode == nullptr)
	{
		SetAvailable(true);
	}
	else
	{
		if (PerkTreeNodeData->ParentNode->GetCurrentLevel() == PerkTreeNodeData->ParentNode->MaxLevel)
		{
			SetAvailable(true);
		}
		else
		{
			SetAvailable(false);
		}
	}
}

void UPerkTreeNodeWidget::SetAvailable(bool Value)
{
	IsAvailable = Value;
	if (IsAvailable)
	{
		SetColorAndOpacity(AvailableColor);
		SetIsLocked(false);

		const auto StrLevelText = FString::Printf(TEXT("%d/%d"), PerkTreeNodeData->GetCurrentLevel(), PerkTreeNodeData->MaxLevel);
		FText LevelText = FText::FromString(StrLevelText);

		if (ConnectionImage)
		{
			ConnectionImage->SetColorAndOpacity(AvailableColor);
		}
	}
	else
	{
		SetColorAndOpacity(UnavailableColor);
		SetIsLocked(true);
		if (ConnectionImage)
		{
			ConnectionImage->SetColorAndOpacity(UnavailableColor);
		}
	}

}

bool UPerkTreeNodeWidget::TryIncreaseLevel() const
{
	if (PerkTreeNodeData != nullptr)
	{
		if (PerkTreeNodeData->GetCurrentLevel() < PerkTreeNodeData->MaxLevel)
		{
			if (PerkTreeNodeData->GetCurrentLevel() + 1 == PerkTreeNodeData->MaxLevel)
			{
				UFMODBlueprintStatics::PlayEvent2D(GetWorld(), LevelMaxSFX, true);
			}
			else
			{
				UFMODBlueprintStatics::PlayEvent2D(GetWorld(), LevelSFX[PerkTreeNodeData->GetCurrentLevel()], true);
			}
			
			/** Spend divine offering currency, must be prior to level increase to get the correct purchase cost via perk level */
			const auto PurchaseCost = GetPurchaseCost();
			PlayerProgressComponentPtr->TrySpendDivineOffering(PurchaseCost);

			PerkTreeNodeData->SetCurrentLevel(PerkTreeNodeData->GetCurrentLevel() + 1);

			const auto StrLevelText = FString::Printf(TEXT("%d/%d"), PerkTreeNodeData->GetCurrentLevel(), PerkTreeNodeData->MaxLevel);
			FText LevelText = FText::FromString(StrLevelText);
		}
		else
		{
			return false;
		}

		CheckChildNode();

		return true;
	}

	return false;
}

bool UPerkTreeNodeWidget::TryRefund()
{
	if (CanRefund())
	{
		PVD_LOG(Display, TEXT("Perk Tree Node Widget->Refund Request"));

		OnRefundRequest.Broadcast(this);
		OnAvailabilityToggled.Broadcast(this, false);

		GES_EMIT_CONTEXT(this, "pvd.gameplayevent", "event.perktree.refund");
		FGESHandler::DefaultHandler()->EmitEvent(GESEmitContext);

		if (IsValid(DivineOfferingWidgetRef))
		{
			DivineOfferingWidgetRef->SetHighlightedNode(this);
		}
		
		return true;
	}
	return false;
}

void UPerkTreeNodeWidget::BtnSelectClick()
{
	if (CanPurchase())
	{
		if (TryIncreaseLevel())
		{
			OnLevelRequired.Broadcast(this, PerkTreeNodeData->GetCurrentLevel());
		}

		if (IsValid(DivineOfferingWidgetRef))
		{
			DivineOfferingWidgetRef->SetHighlightedNode(this);
		}
	}
}

void UPerkTreeNodeWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (IsValid(DivineOfferingWidgetRef))
	{
		DivineOfferingWidgetRef->SetHighlightedNode(this);
	}
}

FReply UPerkTreeNodeWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (UKismetInputLibrary::PointerEvent_IsMouseButtonDown(InMouseEvent, FKey("RightMouseButton")))
	{
		TryRefund();

		return FReply::Handled(); 
	}

	return FReply::Unhandled();
}

FReply UPerkTreeNodeWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	FBindUIActionArgs ActionArgs = FBindUIActionArgs(RefundInputAction,FSimpleDelegate::CreateLambda([&]()
	{
		TryRefund();
	}));
	
	RefundInputHandle = RegisterUIActionBinding(ActionArgs);
	
	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

void UPerkTreeNodeWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	
	RefundInputHandle.Unregister();
}

void UPerkTreeNodeWidget::CheckChildNode() const
{
	if (PerkTreeNodeData->GetCurrentLevel() == PerkTreeNodeData->MaxLevel)
	{
		for (auto ChildNode : PerkTreeNodeData->ChildNodes)
		{

			if (ChildNode->PerkTreeNodeWidget == nullptr)
			{
				PVD_LOG(Error, TEXT("Child node (parent->%s) tree widget is not valid"), *ChildNode->ParentNodePosition.ToString());
				continue;
			}
			
			if (!ChildNode->PerkTreeNodeWidget->IsAvailable)
			{
				ChildNode->PerkTreeNodeWidget->SetAvailable(true);
				ChildNode->PerkTreeNodeWidget->SetFocus();
				OnAvailabilityToggled.Broadcast(ChildNode->PerkTreeNodeWidget, true);
			}
		}
	}
}

bool UPerkTreeNodeWidget::CanRefund() const
{
	bool IsChildNodePurchased = false;
	
	for (auto ChildNode : PerkTreeNodeData->ChildNodes)
	{
		if (ChildNode->PerkTreeNodeWidget == nullptr)
		{
			PVD_LOG(Error, TEXT("Child node (parent->%s) tree widget is not valid"), *ChildNode->ParentNodePosition.ToString());
			continue;
		}

		if (ChildNode->GetCurrentLevel() > 0)
		{
			IsChildNodePurchased = true;
			break;
		}
	}

	return !IsChildNodePurchased && PerkTreeNodeData->GetCurrentLevel() != 0;
}

uint8 UPerkTreeNodeWidget::GetPurchaseCost() const
{
	const auto PerkData = PerkTreeNodeData->PerkDataAsset;

	if (PerkData && PerkData->PerkLevelDatas.IsValidIndex(PerkTreeNodeData->GetCurrentLevel()))
	{
		const auto PerkLevelData = PerkData->PerkLevelDatas[PerkTreeNodeData->GetCurrentLevel()];
		return PerkLevelData.PurchaseCost;
	}

	return 0;
}

uint8 UPerkTreeNodeWidget::GetTotalInvested() const
{
	uint8 TotalInvested = 0;	
	
	for (int i = 0; i < PerkTreeNodeData->GetCurrentLevel(); ++i)
	{
		if (PerkTreeNodeData->PerkDataAsset->PerkLevelDatas.IsValidIndex(i))
		{
			const auto PerkLevelData = PerkTreeNodeData->PerkDataAsset->PerkLevelDatas[i];
			TotalInvested += PerkLevelData.PurchaseCost;
		}
	}
	
	return TotalInvested;
}

bool UPerkTreeNodeWidget::CanPurchase() const
{
	ensureMsgf(PlayerProgressComponentPtr.IsValid(), TEXT("Player Progress Component is not vaild"));

	const auto CurrencyAmount = PlayerProgressComponentPtr->GetDivineOfferingCurrencyAmount();
	const auto PurchaseCost = GetPurchaseCost();
	
	if (PurchaseCost == 0)
	{
		PVD_LOG(Display, TEXT("Perk Tree Node Widget->Purchase cost is 0 and it can not be."));
		return false;
	}
	
	return CurrencyAmount >= PurchaseCost;
}

void UPerkTreeNodeWidget::ToggleHighlight(bool bHighlight) const
{
	Image->SetColorAndOpacity(bHighlight ? HighlightColor : FColor::White);
}
