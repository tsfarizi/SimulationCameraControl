// Copyright Teuku. All Rights Reserved.

#include "CameraMovementBehavior.h"
#include "BaseSimulationCameraControl.h"
#include "BaseSimulationCameraControl_Internal.h"
#include "CameraInputDefaults.h"

TArray<FCameraInputActionSpec> UCameraMovementBehavior::GetActionSpecs_Implementation() const
{
	// Reuse the C++ defaults so there's one source of truth for the
	// action layout. Behavior subclasses are free to return their own list
	// (e.g., a focus behavior that adds IA_Focus_Next / IA_Focus_Prev).
	return CameraInputDefaults::GetDefaultActionSpecs();
}

void UCameraMovementBehavior::HandleAction_Implementation(FName ActionName, const FInputActionValue& Value, UObject* Owner)
{
	ABaseSimulationCameraControl* Pawn = Cast<ABaseSimulationCameraControl>(Owner);
	if (!Pawn)
	{
		return;
	}

	const EInputActionValueType ValueType = Value.GetValueType();

	if (ActionName == FName(TEXT("IA_Zoom")))
	{
		if (ValueType != EInputActionValueType::Axis1D)
		{
			UE_LOG(LogSimulationCameraControl, Warning, TEXT("UCameraMovementBehavior: IA_Zoom expected Axis1D, got %d."),
				static_cast<int32>(ValueType));
			return;
		}
		Pawn->Zoom(Value.Get<float>());
	}
	else if (ActionName == FName(TEXT("IA_Orbit")))
	{
		if (!Pawn->IsOrbitModifierDown())
		{
			return;
		}
		if (ValueType != EInputActionValueType::Axis2D)
		{
			UE_LOG(LogSimulationCameraControl, Warning, TEXT("UCameraMovementBehavior: IA_Orbit expected Axis2D, got %d."),
				static_cast<int32>(ValueType));
			return;
		}
		Pawn->Orbit(Value.Get<FVector2D>());
	}
	else if (ActionName == FName(TEXT("IA_Orbit_Modifier")))
	{
		Pawn->SetOrbitModifierDown(Value.Get<bool>());
	}
	else if (ActionName == FName(TEXT("IA_Pan")))
	{
		if (ValueType != EInputActionValueType::Axis2D)
		{
			UE_LOG(LogSimulationCameraControl, Warning, TEXT("UCameraMovementBehavior: IA_Pan expected Axis2D, got %d."),
				static_cast<int32>(ValueType));
			return;
		}
		const FVector2D AxisValue = Value.Get<FVector2D>();
		const bool bIsKeyInput = FMath::Abs(AxisValue.X) >= 0.5f || FMath::Abs(AxisValue.Y) >= 0.5f;

		// Gate: pan only when the modifier (left mouse button) is held (click-drag),
		// or when input comes from a keyboard key (WASD ±1.0).
		if (Pawn->IsPanModifierDown() || bIsKeyInput)
		{
			if (Pawn->IsPanModifierDown() && !bIsKeyInput)
			{
				// Mouse-drag: invert axes so dragging right moves the camera left
				// ("grab the world and pull" feel, standard sim-game convention).
				Pawn->Pan(FVector2D(-AxisValue.X, -AxisValue.Y));
			}
			else
			{
				// WASD keyboard: pass through as-is (W = forward, S = backward,
				// D = right, A = left).
				Pawn->Pan(AxisValue);
			}
		}
	}
	else if (ActionName == FName(TEXT("IA_Pan_Modifier")))
	{
		Pawn->SetPanModifierDown(Value.Get<bool>());
	}
	else
	{
		UE_LOG(LogSimulationCameraControl, Verbose, TEXT("UCameraMovementBehavior: unhandled action '%s' (this behavior only owns the 5 standard camera-movement actions)."),
			*ActionName.ToString());
	}
}
