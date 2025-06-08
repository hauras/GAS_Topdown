


#include "AbilitySystem/Data/AttributeInfo.h"

FAuraAttributInfo UAttributeInfo::FindAttributeInfoForTag(const FGameplayTag& AttributeTag, bool bLogNotFound)
{
	for (const FAuraAttributInfo& Info : AttributeInformation)
	{
		
		if (Info.AttributeTag.MatchesTagExact(AttributeTag))
		{
			return Info;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Cant find Info for AttributeTag %s on AttributeInfo %s"), *AttributeTag.ToString(), *GetNameSafe(this));
	}

	return FAuraAttributInfo();
}
