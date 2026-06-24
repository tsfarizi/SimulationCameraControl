// Copyright Teuku. All Rights Reserved.

#include "CameraDragPanBehavior.h"
#include "SimulationCameraControlPawn.h"
#include "SimulationCameraControlPawn_Internal.h"

TArray<FCameraInputActionSpec> UCameraDragPanBehavior::GetActionSpecs_Implementation() const
{
	TArray<FCameraInputActionSpec> Specs;

	// IA_Drag_Modifier: default left mouse button (Bool). Both Triggered (press)
	// and Completed (release) are bound in the pawn's auto-bind loop, so the
	// boolean state stays accurate across hold/release cycles. Designer can
	// rebind this to any key via InputBindingsOverride (right-mouse drag,
	// gamepad shoulder, etc.) without touching C++.
	{
		FCameraInputActionSpec Spec;
		Spec.ActionName = FName(TEXT("IA_Drag_Modifier"));
		Spec.ValueType = EInputActionValueType::Boolean;
		FCameraInputKeySpec Key;
		Key.Key = EKeys::LeftMouseButton;
		Key.Axis = FName(TEXT("X"));
		Key.bNegate = false;
		Spec.DefaultKeys.Add(Key);
		Specs.Add(Spec);
	}

	// IA_Drag_Pan: default Mouse2D (Axis2D) reports per-frame pointer-delta in
	// screen space (+X = right, +Y = down in screen coords). InvertAxis handles
	// the "inverse direction" feel at the behavior layer.
	{
		FCameraInputActionSpec Spec;
		Spec.ActionName = FName(TEXT("IA_Drag_Pan"));
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

void UCameraDragPanBehavior::HandleAction_Implementation(FName ActionName, const FInputActionValue& Value, UObject* Owner)
{
	ASimulationCameraControl* Pawn = Cast<ASimulationCameraControl>(Owner);
	if (!Pawn)
	{
		return;
	}

	if (ActionName == FName(TEXT("IA_Drag_Modifier")))
	{
		// Triggered (pressed) -> true, Completed (released) -> false.
		bIsDragActive = Value.Get<bool>();
		return;
	}

	if (ActionName == FName(TEXT("IA_Drag_Pan")))
	{
		// Ignore pointer movement when the modifier isn't held. The Triggered
		// event still fires every frame the pointer moves, but the gate
		// makes sure we don't pan when the user is just moving the cursor
		// around the UI without holding the modifier key.
		if (!bIsDragActive)
		{
			return;
		}

		const EInputActionValueType ValueType = Value.GetValueType();
		if (ValueType != EInputActionValueType::Axis2D)
		{
			UE_LOG(LogSimulationCameraControl, Warning, TEXT("UCameraDragPanBehavior: IA_Drag_Pan expected Axis2D, got %d."),
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
		UE_LOG(LogSimulationCameraControl, Verbose, TEXT("UCameraDragPanBehavior: unhandled action '%s'."), *ActionName.ToString());
	}
}
