#include "CameraDragPanComponent.h"
#include "BaseSimulationCameraControl_Internal.h"

TArray<FCameraInputActionSpec> UCameraDragPanComponent::GetActionSpecs_Implementation() const
{
	TArray<FCameraInputActionSpec> Specs;

	FCameraInputActionSpec DragModifier;
	DragModifier.ActionName = FName(TEXT("IA_Drag_Modifier"));
	DragModifier.ValueType = EInputActionValueType::Boolean;
	DragModifier.DefaultKeys.Add({ EKeys::LeftMouseButton, FName(TEXT("X")), false });
	Specs.Add(DragModifier);

	FCameraInputActionSpec DragPan;
	DragPan.ActionName = FName(TEXT("IA_Drag_Pan"));
	DragPan.ValueType = EInputActionValueType::Axis2D;
	DragPan.DefaultKeys.Add({ EKeys::Mouse2D, FName(TEXT("X")), false });
	Specs.Add(DragPan);

	return Specs;
}

void UCameraDragPanComponent::HandleAction_Implementation(FName ActionName, const FInputActionValue& Value)
{
	if (ActionName == FName(TEXT("IA_Drag_Modifier")))
	{
		bIsDragActive = Value.Get<bool>();
		OnDragModifierInput.Broadcast(bIsDragActive);
		return;
	}

	if (ActionName == FName(TEXT("IA_Drag_Pan")))
	{
		if (!bIsDragActive)
		{
			return;
		}

		if (Value.GetValueType() != EInputActionValueType::Axis2D)
		{
			UE_LOG(LogSimulationCameraControl, Warning,
				TEXT("UCameraDragPanComponent: IA_Drag_Pan expected Axis2D, got %d."),
				static_cast<int32>(Value.GetValueType()));
			return;
		}

		const FVector2D AxisValue = Value.Get<FVector2D>();
		const FVector2D Inverted(
			AxisValue.X * InvertAxis.X * PanSpeedMultiplier,
			AxisValue.Y * InvertAxis.Y * PanSpeedMultiplier);
		OnDragPanInput.Broadcast(Inverted);
		return;
	}

	UE_LOG(LogSimulationCameraControl, Verbose,
		TEXT("UCameraDragPanComponent: unhandled action '%s'."), *ActionName.ToString());
}