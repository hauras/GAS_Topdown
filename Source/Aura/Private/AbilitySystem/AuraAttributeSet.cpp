


#include "AbilitySystem/AuraAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "GameplayEffectExtension.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Controller/AuraPlayerController.h"
#include "Interaction/CombatInterface.h"
#include "Kismet/GameplayStatics.h"

UAuraAttributeSet::UAuraAttributeSet()
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

	//Primary Attributes
	TagsToAttribute.Add(GameplayTags.Attributes_Primary_Strength,GetStrengthAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Primary_Intelligence,GetIntelligenceAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Primary_Resilience, GetResilienceAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Primary_Vigor, GetVigorAttribute);

	//Secondary Attributes
	TagsToAttribute.Add(GameplayTags.Attributes_Secondary_Armor, GetArmorAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Secondary_ArmorPenetration, GetArmorPenetrationAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Secondary_BlockChance, GetBlockChanceAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Secondary_CriticalHitChance, GetCriticalHitChanceAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Secondary_CriticalHitDamage, GetCriticalHitDamageAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Secondary_CriticalHitResistance, GetCriticalHitResistanceAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Secondary_HealthRegeneration, GetHealthRegenerationAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Secondary_ManaRegeneration, GetManaRegenerationAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Secondary_MaxHealth, GetMaxHealthAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Secondary_MaxMana, GetMaxManaAttribute);

	//Resistance Attributes
	TagsToAttribute.Add(GameplayTags.Attributes_Resistance_Arcane, GetArcaneResistanceAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Resistance_Fire, GetFireResistanceAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Resistance_Lightning, GetLightningResistanceAttribute);
	TagsToAttribute.Add(GameplayTags.Attributes_Resistance_Physical, GetPhysicalResistanceAttribute);
	
}

void UAuraAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// ▶ ReplicatedUsing 이 설정된 속성들을 실제로 "네트워크 복제" 등록하는 부분
	// (서버에서 변경된 값을 클라이언트로 보내주기 위해 필수)

	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Resilience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Vigor, COND_None, REPNOTIFY_Always);

	//Vital Attributes
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Mana, COND_None, REPNOTIFY_Always);

	// Secondary Attributes
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, ArmorPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, BlockChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, CriticalHitChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, CriticalHitDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, CriticalHitResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, HealthRegeneration, COND_None, REPNOTIFY_Always);	
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, ManaRegeneration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);	
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);

	//Resistance Attributes
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, FireResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, LightningResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, ArcaneResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, PhysicalResistance, COND_None, REPNOTIFY_Always);

}

void UAuraAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	
	if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
	
}

void UAuraAttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props) const
{
	// Source = 효과를 가한 주체 (예: 공격자)
    // Target = 효과를 받은 객체 = 이 AttributeSet을 소유한 액터

    // 효과를 가한 주체(Source)에 대한 정보 수집

    // 효과를 가한 주체의 EffectContextHandle을 가져옴 (누가, 어떻게, 어떤 방식으로 이펙트를 유발했는가?)
    Props.EffectContextHandle = Data.EffectSpec.GetContext();

    // 원래 이펙트를 가한 주체의 ASC (AbilitySystemComponent) 가져오기
    Props.SourceASC = Props.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();

    // SourceASC가 유효하고, 그것의 ActorInfo와 AvatarActor도 유효한 경우
    if (IsValid(Props.SourceASC) && Props.SourceASC->AbilityActorInfo.IsValid() && Props.SourceASC->AbilityActorInfo->AvatarActor.IsValid())
    {
        // AvatarActor는 일반적으로 컨트롤러에 의해 제어되는 Pawn (ex. 캐릭터)
        Props.SourceAvatarActor = Props.SourceASC->AbilityActorInfo->AvatarActor.Get();

        // PlayerController 가져오기 (직접 연결되어 있으면 사용, 없으면 나중에 Pawn을 통해 얻을 수 있음)
        Props.SourceController = Props.SourceASC->AbilityActorInfo->PlayerController.Get();

        // 만약 Controller가 없고 AvatarActor는 유효하다면
        // → Pawn으로 캐스팅해서 거기서 Controller 가져오기
        if (Props.SourceController == nullptr && Props.SourceAvatarActor != nullptr)
        {
            if (const APawn* Pawn = Cast<APawn>(Props.SourceAvatarActor))
            {
                Props.SourceController = Pawn->GetController(); // Pawn을 통해 컨트롤러 추적
            }
        }

        // 컨트롤러가 유효한 경우, 해당 컨트롤러가 소유한 Pawn → 캐릭터로 캐스팅
        if (Props.SourceController)
        {
            Props.SourceCharacter = Cast<ACharacter>(Props.SourceController->GetPawn());
            // 이 시점에서 SourceCharacter 사용 가능
        }
    }

    // 효과를 받은 대상(Target)에 대한 정보 수집

    // Target의 ASC에서 Actor 정보와 AvatarActor가 유효한 경우
    if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
    {
        // 이 AttributeSet을 가진 주체 (즉, 현재 타겟의 아바타 액터)
        Props.TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();

        // Target의 PlayerController
        Props.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();

        // AvatarActor를 캐릭터로 캐스팅
        Props.TargetCharacter = Cast<ACharacter>(Props.TargetAvatarActor);

        // Target의 ASC 얻기 (블루프린트 라이브러리 활용)
        Props.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Props.TargetAvatarActor);

    }
}

void UAuraAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FEffectProperties Props;
	SetEffectProperties(Data, Props);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
		UE_LOG(LogTemp, Warning, TEXT("Changed Health on %s, Health: %f"), *Props.TargetAvatarActor->GetName(), GetHealth());
	}

	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalIncomingDamage = GetIncomingDamage();
		SetIncomingDamage(0.f);
		if (LocalIncomingDamage > 0.f)
		{
			const float NewHealth = GetHealth() - LocalIncomingDamage;
			SetHealth(FMath::Clamp(NewHealth, 0.f, GetMaxHealth()));

			const bool bFatal = NewHealth <= 0.f;
			if (bFatal)
			{
				ICombatInterface* CombatInterface = Cast<ICombatInterface>(Props.TargetAvatarActor);
				if (CombatInterface)
				{
					CombatInterface->Die();
				}
			}
			else
			{
				FGameplayTagContainer TagContainer;
				TagContainer.AddTag(FAuraGameplayTags::Get().Effects_HitReact);
				Props.TargetASC->TryActivateAbilitiesByTag(TagContainer);
			}

			const bool bBlock = UAuraAbilitySystemLibrary::IsBlockedHit(Props.EffectContextHandle);
			const bool bCriticalHit = UAuraAbilitySystemLibrary::IsCriticalHit(Props.EffectContextHandle);
			ShowFloatingText(Props, LocalIncomingDamage, bBlock, bCriticalHit);
		}
	}
	
}

void UAuraAttributeSet::ShowFloatingText(const FEffectProperties& Props, float Damage, bool bBlockedHit, bool bCriticalHit) const
{
	if (Props.SourceCharacter != Props.TargetCharacter)
	{
		if(AAuraPlayerController* PC = Cast<AAuraPlayerController>(Props.SourceCharacter->Controller))
		{
			PC->ShowDamageNumber(Damage, Props.TargetCharacter, bBlockedHit, bCriticalHit);
		}
	}
}

// 속성이 서버에서 클라이언트로 복제될 때 호출되는 함수
void UAuraAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Health, OldHealth);
}

void UAuraAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Mana, OldMana);

}

void UAuraAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldStrength) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Strength, OldStrength);
}

void UAuraAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldIntelligence) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Intelligence, OldIntelligence);

}

void UAuraAttributeSet::OnRep_Resilience(const FGameplayAttributeData& OldResilience) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Resilience, OldResilience);
}

void UAuraAttributeSet::OnRep_Vigor(const FGameplayAttributeData& OldVigor) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Resilience, OldVigor);

}

void UAuraAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Armor, OldArmor);
}

void UAuraAttributeSet::OnRep_ArmorPenetration(const FGameplayAttributeData& OldArmorPenetration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, ArmorPenetration, OldArmorPenetration);
}

void UAuraAttributeSet::OnRep_BlockChance(const FGameplayAttributeData& OldBlockChance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, BlockChance, OldBlockChance);
}

void UAuraAttributeSet::OnRep_CriticalHitChance(const FGameplayAttributeData& OldCriticalHitChance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, CriticalHitChance, OldCriticalHitChance);
}

void UAuraAttributeSet::OnRep_CriticalHitDamage(const FGameplayAttributeData& OldCriticalHitDamage) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, CriticalHitDamage, OldCriticalHitDamage);
}

void UAuraAttributeSet::OnRep_CriticalHitResistance(const FGameplayAttributeData& OldCriticalHitResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, CriticalHitResistance, OldCriticalHitResistance);
}

void UAuraAttributeSet::OnRep_HealthRegeneration(const FGameplayAttributeData& OldHealthRegeneration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, HealthRegeneration, OldHealthRegeneration);
}

void UAuraAttributeSet::OnRep_ManaRegeneration(const FGameplayAttributeData& OldManaRegeneration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, ManaRegeneration, OldManaRegeneration);
}

void UAuraAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, MaxHealth, OldMaxHealth);
}

void UAuraAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, MaxMana, OldMaxMana);
}

void UAuraAttributeSet::OnRep_FireResistance(const FGameplayAttributeData& OldFireResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, FireResistance, OldFireResistance);
}

void UAuraAttributeSet::OnRep_LightningResistance(const FGameplayAttributeData& OldLightningResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, LightningResistance, OldLightningResistance);
}

void UAuraAttributeSet::OnRep_ArcaneResistance(const FGameplayAttributeData& OldArcaneResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, ArcaneResistance, OldArcaneResistance);
}

void UAuraAttributeSet::OnRep_PhysicalResistance(const FGameplayAttributeData& OldPhysicalResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, PhysicalResistance, OldPhysicalResistance);
}
