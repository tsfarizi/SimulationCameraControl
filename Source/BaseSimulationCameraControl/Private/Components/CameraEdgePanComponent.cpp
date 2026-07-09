#include "CameraEdgePanComponent.h"
#include "BaseSimulationCameraControl_Internal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UCameraEdgePanComponent::UCameraEdgePanComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCameraEdgePanComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		CachedController = Cast<APlayerController>(OwnerPawn->GetController());
	}
}

APlayerController* UCameraEdgePanComponent::ResolveController()
{
	if (APlayerController* Cached = CachedController.Get())
	{
		return Cached;
	}
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
		{
			CachedController = PC;
			return PC;
		}
	}
	return nullptr;
}

float UCameraEdgePanComponent::ComputeEdgeAxis(float Cursor, float ViewportSize) const
{
	const float Denom = FMath::Max(EdgeThreshold, KINDA_SMALL_NUMBER);

	if (Cursor < EdgeThreshold)
	{
		return -FMath::Clamp((EdgeThreshold - Cursor) / Denom, 0.0f, 1.0f);
	}
	if (Cursor > ViewportSize - EdgeThreshold)
	{
		return FMath::Clamp((Cursor - (ViewportSize - EdgeThreshold)) / Denom, 0.0f, 1.0f);
	}
	return 0.0f;
}

void UCameraEdgePanComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerController* PC = ResolveController();
	if (!PC || EdgeThreshold <= 0.0f)
	{
		return;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	FVector2D PanDelta(0.0f, 0.0f);

	if (bEnableEdgePanX)
	{
		PanDelta.X = ComputeEdgeAxis(MouseX, static_cast<float>(ViewportSizeX));
	}
	if (bEnableEdgePanY)
	{
		// Y is inverted: mouse at top of screen (low Y) drives the camera
		// forward (+Y in pawn world). ComputeEdgeAxis returns positive when
		// Cursor > ZoneStart (i.e., bottom of screen), so negate for the
		// natural RTS feel (top scrolls world up = camera moves forward).
		PanDelta.Y = -ComputeEdgeAxis(MouseY, static_cast<float>(ViewportSizeY));
	}

	if (!PanDelta.IsNearlyZero())
	{
		OnEdgePanInput.Broadcast(PanDelta * EdgePanSpeed);
	}
}