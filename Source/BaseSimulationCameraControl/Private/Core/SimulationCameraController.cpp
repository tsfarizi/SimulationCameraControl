#include "SimulationCameraController.h"
#include "CameraInputComponent.h"
#include "CameraInputBindings.h"
#include "CameraInputDefaults.h"
#include "ISelectableInterface.h"
#include "BaseSimulationCameraControl_Internal.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/HitResult.h"
#include "GameFramework/Actor.h"

void ASimulationCameraController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	SetupCameraInput();
}

void ASimulationCameraController::OnUnPossess()
{
	ClearCameraInput();
	Super::OnUnPossess();
}

void ASimulationCameraController::SetupCameraInput()
{
	APawn* CurrentPawn = GetPawn();
	if (!CurrentPawn)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("SetupCameraInput: no possessed pawn."));
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("SetupCameraInput: no local player."));
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("SetupCameraInput: no EnhancedInput subsystem."));
		return;
	}

	TArray<UCameraInputComponent*> InputComps;
	CurrentPawn->GetComponents(InputComps);

	if (InputComps.Num() == 0)
	{
		UE_LOG(LogSimulationCameraControl, Verbose, TEXT("SetupCameraInput: no UCameraInputComponent on pawn."));
		return;
	}

	TArray<FCameraInputActionSpec> AllSpecs;
	for (const UCameraInputComponent* Comp : InputComps)
	{
		if (Comp && Comp->bInputEnabled)
		{
			AllSpecs.Append(Comp->GetActionSpecs());
		}
	}

	// Controller-owned IA_Click: drives HandleInteractionClick (selection raycast).
	// Appended last so component-owned actions resolve first in the IMC dispatch.
	{
		FCameraInputActionSpec ClickSpec;
		ClickSpec.ActionName = FName(TEXT("IA_Click"));
		ClickSpec.ValueType = EInputActionValueType::Boolean;
		ClickSpec.DefaultKeys.Add(FCameraInputKeySpec{ EKeys::LeftMouseButton, FName(TEXT("X")), false });
		AllSpecs.Add(ClickSpec);
	}

	if (AllSpecs.Num() == 0)
	{
		UE_LOG(LogSimulationCameraControl, Verbose, TEXT("SetupCameraInput: no action specs from components."));
		return;
	}

	ClearCameraInput();

	UCameraInputBindings* Bindings = InputBindingsOverride;
	if (!Bindings)
	{
		Bindings = NewObject<UCameraInputBindings>(CurrentPawn, UCameraInputBindings::StaticClass(),
			TEXT("CameraInputBindings_Runtime"), RF_Transient);
		Bindings->Actions = AllSpecs;
	}

	ActiveContext = Bindings->BuildContext(CurrentPawn);
	if (!ActiveContext)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("SetupCameraInput: BuildContext returned null."));
		return;
	}

	Subsystem->AddMappingContext(ActiveContext, InputMappingPriority);

	UE_LOG(LogSimulationCameraControl, Verbose, TEXT("SetupCameraInput: registered IMC with %d mappings."),
		ActiveContext->GetMappings().Num());
}

void ASimulationCameraController::ClearCameraInput()
{
	if (!ActiveContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (LocalPlayer)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->RemoveMappingContext(ActiveContext);
		}
	}

	ActiveContext = nullptr;
}

void ASimulationCameraController::BindActionsToEnhancedInput(UEnhancedInputComponent* EIC)
{
	if (!EIC || !ActiveContext)
	{
		UE_LOG(LogSimulationCameraControl, Verbose, TEXT("BindActionsToEnhancedInput: no EIC or no ActiveContext."));
		return;
	}

	APawn* CurrentPawn = GetPawn();
	if (!CurrentPawn)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("BindActionsToEnhancedInput: no possessed pawn."));
		return;
	}

	TArray<UCameraInputComponent*> InputComps;
	CurrentPawn->GetComponents(InputComps);

	if (InputComps.Num() == 0)
	{
		UE_LOG(LogSimulationCameraControl, Verbose, TEXT("BindActionsToEnhancedInput: no input components on pawn."));
		return;
	}

	for (const FEnhancedActionKeyMapping& Mapping : ActiveContext->GetMappings())
	{
		if (!Mapping.Action)
		{
			continue;
		}

		const UInputAction* Action = Mapping.Action.Get();
		const FName ActionName = Action->GetFName();

		// Controller-owned actions: route to the controller itself, not to a
		// component. IA_Click is the only one today; add more here as needed.
		if (ActionName == FName(TEXT("IA_Click")))
		{
			TWeakObjectPtr<ASimulationCameraController> WeakSelf(this);
			EIC->BindActionValueLambda(Action, ETriggerEvent::Started,
				[WeakSelf](const FInputActionValue& /*Value*/)
				{
					if (ASimulationCameraController* Self = WeakSelf.Get())
					{
						Self->HandleInteractionClick();
					}
				});

			UE_LOG(LogSimulationCameraControl, Verbose, TEXT("Bound: %s -> %s (Controller)"),
				*ActionName.ToString(), *GetNameSafe(this));
			continue;
		}

		UCameraInputComponent* HandlerComp = nullptr;
		for (UCameraInputComponent* Comp : InputComps)
		{
			if (Comp && Comp->bInputEnabled && Comp->HandlesAction(ActionName))
			{
				HandlerComp = Comp;
				break;
			}
		}

		if (!HandlerComp)
		{
			UE_LOG(LogSimulationCameraControl, Warning,
				TEXT("BindActionsToEnhancedInput: no component handles action '%s'."),
				*ActionName.ToString());
			continue;
		}

		TWeakObjectPtr<UCameraInputComponent> WeakComp(HandlerComp);

		EIC->BindActionValueLambda(Action, ETriggerEvent::Triggered,
			[WeakComp, ActionName](const FInputActionValue& Value)
			{
				if (UCameraInputComponent* Comp = WeakComp.Get())
				{
					Comp->HandleAction(ActionName, Value);
				}
			});

		if (Action->ValueType == EInputActionValueType::Boolean)
		{
			EIC->BindActionValueLambda(Action, ETriggerEvent::Completed,
				[WeakComp, ActionName](const FInputActionValue& Value)
				{
					if (UCameraInputComponent* Comp = WeakComp.Get())
					{
						Comp->HandleAction(ActionName, Value);
					}
				});
		}

		UE_LOG(LogSimulationCameraControl, Verbose, TEXT("Bound: %s -> %s"),
			*ActionName.ToString(), *GetNameSafe(HandlerComp));
	}
}

void ASimulationCameraController::HandleInteractionClick()
{
	FHitResult HitResult;
	if (!GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(InteractionTraceChannel), false, HitResult))
	{
		return;
	}

	AActor* HitActor = HitResult.GetActor();
	if (!HitActor)
	{
		return;
	}

	if (HitActor->Implements<USelectableInterface>())
	{
		ISelectableInterface::Execute_OnSelected(HitActor);
	}
}