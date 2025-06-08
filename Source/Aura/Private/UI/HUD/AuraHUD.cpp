// AuraHUD.cpp

#include "UI/HUD/AuraHUD.h"
#include "UI/Widget/AuraUserWidget.h"
#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "UI/WidgetController/OverlayWidgetController.h"

// 오버레이 위젯 컨트롤러를 반환하는 함수
// 위젯 컨트롤러가 아직 생성되지 않았다면 새로 생성하고, 이미 있다면 기존 컨트롤러를 반환
UOverlayWidgetController* AAuraHUD::GetOverlayWidgetController(const FWidgetControllerParams& WCParams)
{
	if (OverlayWidgetController == nullptr)
	{
		// OverlayWidgetController 객체를 생성 (this를 outer로 설정, 클래스는 지정된 BlueprintClass)
		OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
		
		// 생성된 위젯 컨트롤러에 필요한 데이터(플레이어 정보 등) 세팅
		OverlayWidgetController->SetWidgetControllerParams(WCParams);
		OverlayWidgetController->BindCallbacksToDependencies();
		
		
	}

	return OverlayWidgetController;
}

UAttributeMenuWidgetController* AAuraHUD::GetAttributeMenuWidgetController(const FWidgetControllerParams& WcParams)
{
	if (AttributeMenuWidgetController == nullptr)
	{
		AttributeMenuWidgetController = NewObject<UAttributeMenuWidgetController>(this, AttributeMenuWidgetControllerClass);

		AttributeMenuWidgetController->SetWidgetControllerParams(WcParams);
		AttributeMenuWidgetController->BindCallbacksToDependencies();
	}
	return AttributeMenuWidgetController;
}

// HUD에서 오버레이 위젯과 위젯 컨트롤러를 초기화하고, UI를 화면에 표시하는 함수
void AAuraHUD::InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	checkf(OverlayWidgetClass, TEXT("Overlay Widget Class - BP_AuraHUD"));
	checkf(OverlayWidgetControllerClass, TEXT("Overlay Widget - BP Aura HUD"));

	// Overlay UI 위젯 생성 (UUserWidget 기반으로 생성됨)
	UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass);

	// 생성한 위젯을 AuraUserWidget으로 캐스팅하여 OverlayWidget으로 저장
	OverlayWidget = Cast<UAuraUserWidget>(Widget);

	// 위젯 컨트롤러 생성에 필요한 4가지 정보(PlayerController, PlayerState, ASC, AttributeSet)를 담은 구조체 생성
	const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);

	// 위젯 컨트롤러 생성 또는 기존 컨트롤러 반환
	UOverlayWidgetController* WidgetController = GetOverlayWidgetController(WidgetControllerParams);

	// 위젯에 위젯 컨트롤러를 연결하여 데이터 연동 가능하게 함
	OverlayWidget->SetWidgetController(WidgetController);
	WidgetController->BroadcastInitialValues();
	
	// 위젯을 화면에 표시
	Widget->AddToViewport();
}
