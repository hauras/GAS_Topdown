
#include "Controller/AuraPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Interaction/EnemyInterface.h"

AAuraPlayerController::AAuraPlayerController()
{
	// 이 컨트롤러가 네트워크 상에서 복제되도록 설정 (멀티플레이 대응)
	bReplicates = true;
}

void AAuraPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CursorTrace();
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


void AAuraPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 블루프린트에서 설정한 InputContext가 유효한지 확인 (실행 중 nullptr이면 중단)
	check(InputContext);

	// 로컬 플레이어의 입력 서브시스템(EnhancedInput)을 가져온다
	// 이 시스템을 통해 입력 매핑 컨텍스트를 적용할 수 있음
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());

	// Subsystem이 유효한지 확인 (nullptr일 경우 실행 중단 → 버그 조기 탐지)
	check(Subsystem);

	// InputContext를 입력 시스템에 등록하여 해당 입력 매핑을 활성화함
	// 우선순위 0은 기본 입력으로 설정됨 (숫자가 높을수록 우선됨)
	Subsystem->AddMappingContext(InputContext, 0);

	// 마우스 커서를 보이게 설정 (UI 조작 등 필요할 때 필수)
	bShowMouseCursor = true;

	// 기본 마우스 커서 모양 설정 (디폴트는 화살표)
	DefaultMouseCursor = EMouseCursor::Default;

	// 마우스 입력 모드를 "게임 + UI"로 설정
	// 마우스를 뷰포트에 고정하지 않고, 입력 중에도 커서를 숨기지 않음
	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // 마우스를 뷰포트 밖으로 움직일 수 있음
	InputModeData.SetHideCursorDuringCapture(false); // 클릭하고 드래그 중에도 커서 유지

	// 설정한 입력 모드 적용
	SetInputMode(InputModeData);
}

void AAuraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAuraPlayerController::Move);
}

void AAuraPlayerController::Move(const FInputActionValue& InputActionValue)
{
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis((EAxis::X));
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis((EAxis::Y));

	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X);
	}

}

