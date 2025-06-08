
#include "Controller/AuraPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AuraGameplayTags.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Input/AuraInputComponent.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Components/SplineComponent.h"
#include "Interaction/EnemyInterface.h"

AAuraPlayerController::AAuraPlayerController()
{
	// 이 컨트롤러가 네트워크 상에서 복제되도록 설정 (멀티플레이 대응)
	bReplicates = true;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	
}

void AAuraPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CursorTrace();
	AutoRun();
	
}

void AAuraPlayerController::AutoRun()
{
	if (!bAutoRunning) return;
	if (APawn* ControlledPawn = GetPawn())
	{
		const FVector LocationOnPSpline = Spline->FindLocationClosestToWorldLocation(ControlledPawn->GetActorLocation(), ESplineCoordinateSpace::World);
		const FVector Direction = Spline->FindDirectionClosestToWorldLocation(LocationOnPSpline, ESplineCoordinateSpace::World);
		ControlledPawn->AddMovementInput(Direction);

		const float DistanceToDestination = (LocationOnPSpline - CachedDestination).Length();
		if (DistanceToDestination <= AutoRunAcceptanceRadius)
		{
			bAutoRunning = false;
		}
	}
}

void AAuraPlayerController::CursorTrace()
{
	// FHitResult : 충돌 결과를 저장하는 구조체
	FHitResult CursorHit;
	GetHitResultUnderCursor(ECC_Visibility, false, CursorHit);
	if (!CursorHit.bBlockingHit) return;

	LastActor = ThisActor;
	ThisActor = CursorHit.GetActor();

	/**
	 * 의사 코드 작성
	 * 1. LastActor 와 ThisActor가 null인 경우
	 * - 호출 안함
	 * 2. LastActor는 null 이고 ThisActor(적의 인터페이스)가 유효할 경우
	 * - highlight -> ThisActor
	 * 3. LastActor가 유효하고 ThisActor가 null인 경우
	 * - Unhighlight -> LastActor
	 * 4. LastActor 와 ThisActor가 둘다 유효하지만 둘이 같지 않을 경우
	 * - Unhighlight -> LastActor && Highlight -> ThisActor
	 * 5. LastActor 와 ThisActor가 둘다 유효하고 둘이 같을 경우
	 * - 호출 안함
	 **/

	if (LastActor == nullptr)
	{
		if (ThisActor == nullptr)
		{
			//1번	
		}
		else
		{
			// 2번
			ThisActor->HighlightActor();
		}
	}
	else
	{
		if (ThisActor == nullptr)
		{
			// 3번
			LastActor->UnHighlightActor();
		}
		else
		{
			if (LastActor != ThisActor)
			{
				// 4번
				LastActor->UnHighlightActor();
				ThisActor->HighlightActor();
			}
			else
			{
				// 5번
			}
		}
	}
}

// 어빌리티 입력 태그가 눌렸을 때 호출됨
void AAuraPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	// 왼쪽 마우스 버튼 입력 처리
	if (InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
	{
		// ThisActor가 유효하면 타게팅 중 상태로 설정
		bTargeting = ThisActor ? true : false;

		// 자동 이동 중이 아니라는 플래그 설정
		bAutoRunning = false;
	}
}

// 어빌리티 입력 태그가 떼졌을 때 호출됨
void AAuraPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
	{
		if (GetASC())
		{
			GetASC()->AbilityInputTagReleased(InputTag);
		}
		return;
	}

	if (GetASC()) GetASC()->AbilityInputTagHeld(InputTag);
	
	if (!bTargeting && !bShiftKeyDown)
	{
		APawn* ControlledPawn = GetPawn();
		if (FollowTime <= ShortPressThreshold && ControlledPawn)
		{
			if (UNavigationPath* NewPath = UNavigationSystemV1::FindPathToLocationSynchronously(this, ControlledPawn->GetActorLocation(), CachedDestination))
			{
				Spline->ClearSplinePoints();
				for (const FVector& PointLoc : NewPath->PathPoints)
				{
					Spline->AddSplinePoint(PointLoc, ESplineCoordinateSpace::World);
					DrawDebugSphere(GetWorld(), PointLoc, 8.f, 8.f, FColor::Green, false, 5.f);
				}
				bAutoRunning = true;
			}
		}
		FollowTime = 0.f;
		bTargeting = false;
	}
	
}

// 어빌리티 입력 태그가 눌려진 상태일 때 매 프레임 호출
void AAuraPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
	// LMB가 아닌 경우: 단순 어빌리티 유지
	if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
	{
		if (GetASC())
		{
			GetASC()->AbilityInputTagHeld(InputTag);
		}
		return;
	}

	// 타게팅 중이면 Ability 유지
	if (bTargeting || bShiftKeyDown)
	{
		if (GetASC())
		{
			GetASC()->AbilityInputTagHeld(InputTag);
		}
	}
	else
	{
		// 타겟팅 중이 아닐 경우 마우스 클릭으로 자동 이동 처리
		FollowTime += GetWorld()->GetDeltaSeconds();

		FHitResult Hit;
		if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			// 클릭된 위치 저장
			CachedDestination = Hit.ImpactPoint;
		}

		// 캐릭터를 클릭된 위치로 이동시키기 위한 입력 처리
		if (APawn* ControlledPawn = GetPawn())
		{
			const FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
			ControlledPawn->AddMovementInput(WorldDirection);
		}
	}
}

// AuraAbilitySystemComponent를 캐싱해서 가져옴
UAuraAbilitySystemComponent* AAuraPlayerController::GetASC()
{
	// 이미 캐시되어 있다면 바로 반환
	if (AuraAbilitySystemComponent == nullptr)
	{
		// Pawn에서 ASC를 가져와 캐싱
		AuraAbilitySystemComponent = Cast<UAuraAbilitySystemComponent>(
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>())
		);
	}
	return AuraAbilitySystemComponent;
}


// 게임 시작 시 호출되는 함수
void AAuraPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// InputContext가 설정되지 않았으면 중단
	check(InputContext);

	// EnhancedInput 시스템에서 로컬 플레이어의 서브시스템 가져오기
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());

	if (Subsystem)
	{
		// InputContext를 등록하여 해당 입력 바인딩 적용
		Subsystem->AddMappingContext(InputContext, 0);
	}

	// 마우스 커서 활성화 및 UI 인터랙션 설정
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	// 게임 + UI 모드로 마우스 입력 설정
	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputModeData.SetHideCursorDuringCapture(false);

	SetInputMode(InputModeData);
}

// 입력 컴포넌트 설정 함수 (입력 바인딩)
void AAuraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// AuraInputComponent로 캐스팅
	UAuraInputComponent* AuraInputComponent = CastChecked<UAuraInputComponent>(InputComponent);

	// 이동 액션 바인딩
	AuraInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAuraPlayerController::Move);
	AuraInputComponent->BindAction(ShiftAction, ETriggerEvent::Started, this, &AAuraPlayerController::ShiftPressed);
	AuraInputComponent->BindAction(ShiftAction, ETriggerEvent::Completed, this, &AAuraPlayerController::ShiftReleased);

	// 어빌리티 태그 입력 바인딩
	AuraInputComponent->BindAbilityActions(
		InputConfig, 
		this, 
		&ThisClass::AbilityInputTagPressed, 
		&ThisClass::AbilityInputTagReleased, 
		&ThisClass::AbilityInputTagHeld
	);
}

void AAuraPlayerController::Move(const FInputActionValue& InputActionValue)
{
	// 2D 입력값 (X, Y)
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();

	// 현재 컨트롤러의 방향(Yaw) 가져오기
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	// 전방 및 우측 방향 벡터 계산
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// 캐릭터에 이동 입력 적용
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X);
	}
}