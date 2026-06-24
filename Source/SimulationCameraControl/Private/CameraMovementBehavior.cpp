// Copyright Teuku. All Rights Reserved.

#include "CameraMovementBehavior.h"
#include "SimulationCameraControlPawn.h"
#include "SimulationCameraControlPawn_Internal.h"
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
	ASimulationCameraControl* Pawn = Cast<ASimulationCameraControl>(Owner);
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
		// Pan if the modifier (middle mouse) is held OR if the input is strong
		// (WASD keys usually give +/- 1.0). This lets WASD work without holding
		// the modifier while gating plain mouse movement.
		const bool bIsKeyInput = FMath::Abs(AxisValue.X) >= 0.5f || FMath::Abs(AxisValue.Y) >= 0.5f;
		if (Pawn->IsPanModifierDown() || bIsKeyInput)
		{
			Pawn->Pan(AxisValue);
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
