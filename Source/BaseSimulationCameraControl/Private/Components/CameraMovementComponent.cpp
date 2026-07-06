#include "CameraMovementComponent.h"
#include "CameraInputDefaults.h"
#include "BaseSimulationCameraControl_Internal.h"

TArray<FCameraInputActionSpec> UCameraMovementComponent::GetActionSpecs_Implementation() const
{
	return CameraInputDefaults::GetDefaultActionSpecs();
}

void UCameraMovementComponent::HandleAction_Implementation(FName ActionName, const FInputActionValue& Value)
{
	const EInputActionValueType ValueType = Value.GetValueType();

	if (ActionName == FName(TEXT("IA_Zoom")))
	{
		if (ValueType != EInputActionValueType::Axis1D)
		{
			UE_LOG(LogSimulationCameraControl, Warning,
				TEXT("UCameraMovementComponent: IA_Zoom expected Axis1D, got %d."),
				static_cast<int32>(ValueType));
			return;
		}
		OnZoomInput.Broadcast(Value.Get<float>());
	}
	else if (ActionName == FName(TEXT("IA_Orbit")))
	{
		if (!bIsOrbitModifierDown)
		{
			return;
		}
		if (ValueType != EInputActionValueType::Axis2D)
		{
			UE_LOG(LogSimulationCameraControl, Warning,
				TEXT("UCameraMovementComponent: IA_Orbit expected Axis2D, got %d."),
				static_cast<int32>(ValueType));
			return;
		}
		OnOrbitInput.Broadcast(Value.Get<FVector2D>());
	}
	else if (ActionName == FName(TEXT("IA_Orbit_Modifier")))
	{
		bIsOrbitModifierDown = Value.Get<bool>();
		OnOrbitModifierInput.Broadcast(bIsOrbitModifierDown);
	}
	else if (ActionName == FName(TEXT("IA_Pan")))
	{
		if (ValueType != EInputActionValueType::Axis2D)
		{
			UE_LOG(LogSimulationCameraControl, Warning,
				TEXT("UCameraMovementComponent: IA_Pan expected Axis2D, got %d."),
				static_cast<int32>(ValueType));
			return;
		}
		const FVector2D AxisValue = Value.Get<FVector2D>();
		const bool bIsKeyInput = FMath::Abs(AxisValue.X) >= 0.5f || FMath::Abs(AxisValue.Y) >= 0.5f;

		if (bIsPanModifierDown || bIsKeyInput)
		{
			if (bIsPanModifierDown && !bIsKeyInput)
			{
				OnPanInput.Broadcast(FVector2D(-AxisValue.X, -AxisValue.Y));
			}
			else
			{
				OnPanInput.Broadcast(AxisValue);
			}
		}
	}
	else if (ActionName == FName(TEXT("IA_Pan_Modifier")))
	{
		bIsPanModifierDown = Value.Get<bool>();
		OnPanModifierInput.Broadcast(bIsPanModifierDown);
	}
	else
	{
		UE_LOG(LogSimulationCameraControl, Verbose,
			TEXT("UCameraMovementComponent: unhandled action '%s'."),
			*ActionName.ToString());
	}
}
