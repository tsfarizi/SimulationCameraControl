#include "SimulationCameraControlPawn.h"
#include "SimulationCameraControlPawn_Internal.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

void ASimulationCameraControl::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("SetupPlayerInputComponent: PlayerInputComponent null."));
		return;
	}

	UEnhancedInputComponent* EnhancedComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedComponent)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("SetupPlayerInputComponent: expected EnhancedInputComponent but received %s."),
			*PlayerInputComponent->GetName());
		return;
	}

	if (!DefaultInputMapping)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("SetupPlayerInputComponent: DefaultInputMapping is not set."));
		return;
	}

	// Auto-bind all Input Actions from DefaultInputMapping by name convention
	if (bAutoBindInputActions)
	{
		for (const FEnhancedActionKeyMapping& Mapping : DefaultInputMapping->GetMappings())
		{
			if (!Mapping.Action)
			{
				continue;
			}

			const FName ActionName = Mapping.Action->GetFName();

			// Zoom: IA_Zoom (1D Axis - float)
			if (ActionName == FName("IA_Zoom"))
			{
				EnhancedComponent->BindAction(Mapping.Action, ETriggerEvent::Triggered, this, &ASimulationCameraControl::HandleZoomAction);
				UE_LOG(LogSimulationCameraControl, Verbose, TEXT("Auto-bound: %s -> Zoom"), *ActionName.ToString());
			}
			// Orbit: IA_Orbit (2D Axis - FVector2D)
			else if (ActionName == FName("IA_Orbit"))
			{
				EnhancedComponent->BindAction(Mapping.Action, ETriggerEvent::Triggered, this, &ASimulationCameraControl::HandleOrbitAction);
				UE_LOG(LogSimulationCameraControl, Verbose, TEXT("Auto-bound: %s -> Orbit"), *ActionName.ToString());
			}
			// Orbit Modifier: IA_Orbit_Modifier (Bool)
			else if (ActionName == FName("IA_Orbit_Modifier"))
			{
				EnhancedComponent->BindAction(Mapping.Action, ETriggerEvent::Triggered, this, &ASimulationCameraControl::HandleOrbitModifierAction);
				EnhancedComponent->BindAction(Mapping.Action, ETriggerEvent::Completed, this, &ASimulationCameraControl::HandleOrbitModifierAction);
				UE_LOG(LogSimulationCameraControl, Verbose, TEXT("Auto-bound: %s -> OrbitModifier"), *ActionName.ToString());
			}
			// Pan: IA_Pan (2D Axis - FVector2D)
			else if (ActionName == FName("IA_Pan"))
			{
				EnhancedComponent->BindAction(Mapping.Action, ETriggerEvent::Triggered, this, &ASimulationCameraControl::HandlePanAction);
				UE_LOG(LogSimulationCameraControl, Verbose, TEXT("Auto-bound: %s -> Pan"), *ActionName.ToString());
			}
			// Pan Modifier: IA_Pan_Modifier (Bool)
			else if (ActionName == FName("IA_Pan_Modifier"))
			{
				EnhancedComponent->BindAction(Mapping.Action, ETriggerEvent::Triggered, this, &ASimulationCameraControl::HandlePanModifierAction);
				EnhancedComponent->BindAction(Mapping.Action, ETriggerEvent::Completed, this, &ASimulationCameraControl::HandlePanModifierAction);
				UE_LOG(LogSimulationCameraControl, Verbose, TEXT("Auto-bound: %s -> PanModifier"), *ActionName.ToString());
			}
		}
	}
}

void ASimulationCameraControl::InitializeInputMapping()
{
	if (!DefaultInputMapping)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("InitializeInputMapping skipped: DefaultInputMapping is null."));
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("InitializeInputMapping failed: no controller."));
		return;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("InitializeInputMapping failed: controller has no local player."));
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("InitializeInputMapping failed: EnhancedInput subsystem unavailable."));
		return;
	}

	if (Subsystem->HasMappingContext(DefaultInputMapping))
	{
		Subsystem->RemoveMappingContext(DefaultInputMapping);
		UE_LOG(LogSimulationCameraControl, VeryVerbose, TEXT("InitializeInputMapping: Removed existing mapping %s before re-adding."),
			*GetNameSafe(DefaultInputMapping));
	}

	Subsystem->AddMappingContext(DefaultInputMapping, InputMappingPriority);
	UE_LOG(LogSimulationCameraControl, Verbose, TEXT("InitializeInputMapping: Added %s with priority %d for %s."),
		*GetNameSafe(DefaultInputMapping), InputMappingPriority, *GetName());
}

void ASimulationCameraControl::HandleZoomAction(const FInputActionInstance& Instance)
{
	const EInputActionValueType ValueType = Instance.GetValue().GetValueType();
	if (ValueType != EInputActionValueType::Axis1D)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("HandleZoomAction: Expected Axis1D but received %d."),
			static_cast<int32>(ValueType));
		return;
	}

	Zoom(Instance.GetValue().Get<float>());
}

void ASimulationCameraControl::HandleOrbitAction(const FInputActionInstance& Instance)
{
	// Only orbit if the modifier key (Right Mouse) is held down
	if (!bIsOrbitModifierDown)
	{
		return;
	}

	const EInputActionValueType ValueType = Instance.GetValue().GetValueType();
	if (ValueType != EInputActionValueType::Axis2D)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("HandleOrbitAction: Expected Axis2D but received %d."),
			static_cast<int32>(ValueType));
		return;
	}

	Orbit(Instance.GetValue().Get<FVector2D>());
}

void ASimulationCameraControl::HandlePanAction(const FInputActionInstance& Instance)
{
	const EInputActionValueType ValueType = Instance.GetValue().GetValueType();
	if (ValueType != EInputActionValueType::Axis2D)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("HandlePanAction: Expected Axis2D but received %d."),
			static_cast<int32>(ValueType));
		return;
	}

	const FVector2D AxisValue = Instance.GetValue().Get<FVector2D>();

	// Pan if modifier is held (Middle Mouse) OR if input is strong (WASD keys usually give +/- 1.0)
	// This allows WASD to work without holding a button, while gating mouse movement.
	const bool bIsKeyInput = FMath::Abs(AxisValue.X) >= 0.5f || FMath::Abs(AxisValue.Y) >= 0.5f;

	if (bIsPanModifierDown || bIsKeyInput)
	{
		Pan(AxisValue);
	}
}

void ASimulationCameraControl::HandleOrbitModifierAction(const FInputActionInstance& Instance)
{
	bIsOrbitModifierDown = Instance.GetValue().Get<bool>();
}

void ASimulationCameraControl::HandlePanModifierAction(const FInputActionInstance& Instance)
{
	bIsPanModifierDown = Instance.GetValue().Get<bool>();
}
