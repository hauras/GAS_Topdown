


#include "Actor/AuraEffectActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Components/SphereComponent.h"

AAuraEffectActor::AAuraEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("SceneRoot"));
}

void AAuraEffectActor::BeginPlay()
{
	Super::BeginPlay();
}

// 실제 효과를 타겟에게 적용하는 핵심 함수
void AAuraEffectActor::ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
    // 타겟의 ASC 가져오기 (인터페이스 구현 여부 상관없이)
    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    if (TargetASC == nullptr) return;

    // 게임플레이 이펙트 클래스가 유효하지 않으면 Crash 발생 (디버그용)
    check(GameplayEffectClass);

    // 이펙트 컨텍스트 생성 및 소스 설정 (이 이펙트 액터 자신)
    FGameplayEffectContextHandle EffectContextHandle = TargetASC->MakeEffectContext();
    EffectContextHandle.AddSourceObject(this);

    // 스펙 핸들 생성 (레벨 1, 위 컨텍스트 포함)
    const FGameplayEffectSpecHandle EffectSpecHandle = TargetASC->MakeOutgoingSpec(GameplayEffectClass, ActorLevel, EffectContextHandle);

    // 스펙을 자기 자신에게 적용
    const FActiveGameplayEffectHandle ActiveEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());

    // 이 이펙트가 무한 지속시간인지 체크
    const bool bIsInfinite = EffectSpecHandle.Data.Get()->Def.Get()->DurationPolicy == EGameplayEffectDurationType::Infinite;

    // 무한 효과이며, EndOverlap 시 제거하는 설정이라면 → 맵에 저장
    if (bIsInfinite && InfiniteEffectRemovePolicy == EEffectRemovePolicy::RemoveOnEndOverlap)
    {
        ActiveEffectHandles.Add(ActiveEffectHandle, TargetASC);
    }
}

// 오버랩 발생 시 이펙트 적용 여부를 정책에 따라 판단
void AAuraEffectActor::OnOverlap(AActor* TargetActor)
{
    if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
    {
        ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
    }
    if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
    {
        ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
    }
    if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
    {
        ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
    }
}

// 오버랩 종료 시 이펙트 적용 및 제거 정책에 따라 처리
void AAuraEffectActor::OnEndOverlap(AActor* TargetActor)
{
    // EndOverlap에서 이펙트 적용
    if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
    {
        ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
    }
    if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
    {
        ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
    }
    if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
    {
        ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
    }

    // Infinite 효과 제거 조건이면...
    if (InfiniteEffectRemovePolicy == EEffectRemovePolicy::RemoveOnEndOverlap)
    {
        // 타겟의 ASC를 가져옴
        UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
        if (!IsValid(TargetASC)) return;

        // 제거할 핸들을 임시 저장할 배열
        TArray<FActiveGameplayEffectHandle> HandlesToRemove;

        // 모든 저장된 효과 핸들을 순회
        for (auto HandlePair : ActiveEffectHandles)
        {
            // 타겟 ASC와 일치하는 경우
            if (TargetASC == HandlePair.Value)
            {
                // 1스택 제거
                TargetASC->RemoveActiveGameplayEffect(HandlePair.Key, 1);
                HandlesToRemove.Add(HandlePair.Key);
            }
        }

        // 제거된 핸들들을 맵에서도 제거
        for (auto& Handle : HandlesToRemove)
        {
            ActiveEffectHandles.FindAndRemoveChecked(Handle);
        }
    }
}


