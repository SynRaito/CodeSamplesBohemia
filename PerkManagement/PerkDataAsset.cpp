#include "../Data/PerkDataAsset.h"

#if WITH_EDITOR

bool UPerkDataAsset::CanEditChange(const FProperty* InProperty) const
{
	const FName PropertyName = InProperty->GetFName();

	if(	PropertyName == GET_MEMBER_NAME_CHECKED(FPerkLevelData, Ability) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FPerkLevelData, UseAbilityOnAdd))
	{
		if(PerkType == EPerkType::GameplayAbility)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	return Super::CanEditChange(InProperty);
}

#endif
