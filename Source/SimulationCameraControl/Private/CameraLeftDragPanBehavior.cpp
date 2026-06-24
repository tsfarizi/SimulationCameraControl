// Copyright Teuku. All Rights Reserved.

#include "CameraLeftDragPanBehavior.h"
#include "SimulationCameraControlPawn.h"
#include "SimulationCameraControlPawn_Internal.h"

TArray<FCameraInputActionSpec> UCameraLeftDragPanBehavior::GetActionSpecs_Implementation() const
{
	TArray<FCameraInputActionSpec> Specs;

	// IA_LeftDrag_Modifier: left mouse button (Bool). Both Triggered (press)
	// and Completed (release) are bound in the pawn's auto-bind loop, so the
	// boolean state stays accurate across hold/release cycles.
	{
		FCameraInputActionSpec Spec;
		Spec.ActionName = FName(TEXT("IA_LeftDrag_Modifier"));
		Spec.ValueType = EInputActionValueType::Boolean;
		FCameraInputKeySpec Key;
		Key.Key = EKeys::LeftMouseButton;
		Key.Axis = FName(TEXT("X"));
		Key.bNegate = false;
		Spec.DefaultKeys.Add(Key);
		Specs.Add(Spec);
	}

	// IA_LeftDrag_Pan: Mouse2D (Axis2D) reports per-frame mouse-delta in screen
	// space (+X = right, +Y = down in screen coords). InvertAxis handles the
	// "inverse direction" feel at the behavior layer.
	{
		FCameraInputActionSpec Spec;
		Spec.ActionName = FName(TEXT("IA_LeftDrag_Pan"));
		Spec.ValueType = EInputActionValueType::Axis2D;
		FCameraInputKeySpec Key;
		Key.Key = EKeys::Mouse2D;
		Key.Axis = FName(TEXT("Y"));
		Key.bNegate = false;
		Spec.DefaultKeys.Add(Key);
		Specs.Add(Spec);
	}

	return Specs;
}

void UCameraLeftDragPanBehavior::HandleAction_Implementation(FName ActionName, const FInputActionValue& Value, UObject* Owner)
{
	ASimulationCameraControl* Pawn = Cast<ASimulationCameraControl>(Owner);
	if (!Pawn)
	{
		return;
	}

	if (ActionName == FName(TEXT("IA_LeftDrag_Modifier")))
	{
		// Triggered (pressed) -> true, Completed (released) -> false.
		bIsLeftDragActive = Value.Get<bool>();
		return;
	}

	if (ActionName == FName(TEXT("IA_LeftDrag_Pan")))
	{
		// Ignore mouse movement when the modifier isn't held. The Triggered
		// event still fires every frame the mouse moves, but the gate
		// makes sure we don't pan when the user is just moving the cursor
		// around the UI without holding the left button.
		if (!bIsLeftDragActive)
		{
			return;
		}

		const EInputActionValueType ValueType = Value.GetValueType();
		if (ValueType != EInputActionValueType::Axis2D)
		{
			UE_LOG(LogSimulationCameraControl, Warning, TEXT("UCameraLeftDragPanBehavior: IA_LeftDrag_Pan expected Axis2D, got %d."),
				static_cast<int32>(ValueType));
			return;
		}

		const FVector2D AxisValue = Value.Get<FVector2D>();

		// Invert and scale. The pawn's Pan() multiplies the result by its own
		// PanSpeed (cm/s) and applies camera-yaw rotation, so we just feed it
		// the per-axis signed delta and let it do the rest.
		const FVector2D Inverted(
			AxisValue.X * InvertAxis.X * PanSpeedMultiplier,
			AxisValue.Y * InvertAxis.Y * PanSpeedMultiplier);

		Pawn->Pan(Inverted);
	}
	else
	{
		UE_LOG(LogSimulationCameraControl, Verbose, TEXT("UCameraLeftDragPanBehavior: unhandled action '%s'."), *ActionName.ToString());
	}
}
