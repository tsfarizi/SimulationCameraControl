// Copyright Teuku. All Rights Reserved.

#include "SimulationCameraControlPawn.h"
#include "SimulationCameraControlPawn_Internal.h"
#include "CameraInputBindings.h"
#include "CameraInputDefaults.h"
#include "CameraInputBehavior.h"
#include "CameraMovementBehavior.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
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

	// Build (or reuse) the active input context. InitializeInputMapping stores it in
	// the pawn's outer so it survives until the pawn is destroyed.
	if (!ActiveInputMapping)
	{
		InitializeInputMapping();
	}

	if (!ActiveInputMapping)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("SetupPlayerInputComponent: no active mapping context (InitializeInputMapping failed)."));
		return;
	}

	AutoBindBehaviorsToContext(EnhancedComponent);
}

void ASimulationCameraControl::AutoBindBehaviorsToContext(UEnhancedInputComponent* EnhancedComponent)
{
	if (!EnhancedComponent || !ActiveInputMapping || Behaviors.Num() == 0)
	{
		return;
	}

	// Build a TMap<UInputAction*, TWeakObjectPtr<UCameraInputBehavior>> for O(1) dispatch.
	// We iterate the IMC's mappings (the source of truth for what was actually
	// registered) and look up which behavior claimed each action name.
	TMap<const UInputAction*, TWeakObjectPtr<UCameraInputBehavior>> ActionToBehavior;
	ActionToBehavior.Reserve(ActiveInputMapping->GetMappings().Num());

	for (const FEnhancedActionKeyMapping& Mapping : ActiveInputMapping->GetMappings())
	{
		if (!Mapping.Action)
		{
			continue;
		}
		const UInputAction* ActionPtr = Mapping.Action.Get();
		const FName ActionName = Mapping.Action->GetFName();

		// First behavior that claims this action wins. The pawn's Behaviors order
		// is designer-controlled via the Details panel — drag the higher-priority
		// behavior first to override defaults from later behaviors.
		TWeakObjectPtr<UCameraInputBehavior>* Existing = ActionToBehavior.Find(ActionPtr);
		if (Existing && Existing->IsValid())
		{
			continue;
		}

		for (UCameraInputBehavior* Behavior : Behaviors)
		{
			if (Behavior && Behavior->HandlesAction(ActionName))
			{
				ActionToBehavior.Add(ActionPtr, Behavior);
				break;
			}
		}
	}

	// Bind each (action, behavior) pair. The lambda captures a TWeakObjectPtr so a
	// behavior that gets GC'd between action registration and the event fire
	// becomes a no-op rather than a crash.
	for (const TPair<const UInputAction*, TWeakObjectPtr<UCameraInputBehavior>>& Pair : ActionToBehavior)
	{
		const UInputAction* Action = Pair.Key;
		TWeakObjectPtr<UCameraInputBehavior> WeakBehavior = Pair.Value;

		if (!Action)
		{
			continue;
		}

		const FName ActionName = Action->GetFName();

		// All actions get the Triggered event.
		EnhancedComponent->BindActionValueLambda(Action, ETriggerEvent::Triggered,
			[WeakBehavior, ActionName, this](const FInputActionValue& Value)
			{
				if (UCameraInputBehavior* Behavior = WeakBehavior.Get())
				{
					Behavior->HandleAction(ActionName, Value, this);
				}
			});

		// Boolean actions (modifier keys) also need the Completed event so the
		// "released" state is captured.
		if (Action->ValueType == EInputActionValueType::Boolean)
		{
			EnhancedComponent->BindActionValueLambda(Action, ETriggerEvent::Completed,
				[WeakBehavior, ActionName, this](const FInputActionValue& Value)
				{
					if (UCameraInputBehavior* Behavior = WeakBehavior.Get())
					{
						Behavior->HandleAction(ActionName, Value, this);
					}
				});
		}

		UE_LOG(LogSimulationCameraControl, Verbose, TEXT("Auto-bound: %s -> %s"),
			*Action->GetName(),
			*GetNameSafe(WeakBehavior.Get()));
	}

	// Warn about registered actions with no behavior claim — likely a typo or a
	// forgotten Behaviors array entry. This makes typos loud at startup.
	for (const FEnhancedActionKeyMapping& Mapping : ActiveInputMapping->GetMappings())
	{
		if (Mapping.Action && !ActionToBehavior.Contains(Mapping.Action.Get()))
		{
			UE_LOG(LogSimulationCameraControl, Warning, TEXT("Action '%s' is registered in the active mapping context but no behavior in Behaviors[] claims it. Add a behavior with GetActionSpecs() declaring this action, or remove the entry from InputBindingsOverride."),
				*Mapping.Action->GetName());
		}
	}
}

void ASimulationCameraControl::InitializeInputMapping()
{
	// Build the active input context. Order of preference:
	//   1. InputBindingsOverride (DataAsset) - designer-edited spec list
	//   2. Behaviors' GetActionSpecs() (concatenated in array order)
	//   3. C++ defaults (CameraInputDefaults::GetDefaultActionSpecs)
	// Result is parented to `this` so the GC keeps it alive alongside the pawn.

	if (ActiveInputMapping)
	{
		return;
	}

	TArray<FCameraInputActionSpec> AllSpecs;

	if (InputBindingsOverride)
	{
		AllSpecs = InputBindingsOverride->Actions;
	}
	else
	{
		for (UCameraInputBehavior* Behavior : Behaviors)
		{
			if (Behavior)
			{
				AllSpecs.Append(Behavior->GetActionSpecs());
			}
		}
	}

	if (AllSpecs.Num() == 0)
	{
		// Final fallback: hardcoded defaults so the pawn still has SOME actions
		// registered even if the designer cleared the Behaviors array and didn't
		// assign an override DataAsset. (Behaviors are not used because there
		// would be no handler — the warnings above in AutoBindBehaviorsToContext
		// will surface the missing-claim issue.)
		AllSpecs = CameraInputDefaults::GetDefaultActionSpecs();
	}

	// Wrap the spec list in a transient UCameraInputBindings so we can reuse
	// BuildContext. Outer is the pawn so GC keeps both alive together.
	UCameraInputBindings* TempBindings = NewObject<UCameraInputBindings>(
		this, UCameraInputBindings::StaticClass(), TEXT("TransientCameraInputBindings"), RF_Transient);
	if (TempBindings)
	{
		TempBindings->Actions = MoveTemp(AllSpecs);
		ActiveInputMapping = TempBindings->BuildContext(this);
	}

	if (!ActiveInputMapping)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("InitializeInputMapping: failed to build any input context."));
		return;
	}

	// Hook: post-build, pre-registration. Subscribers can mutate the IMC
	// (add UInputTrigger* to specific mappings, swap UInputModifier* chains,
	// add runtime-only keys, etc.) before the player's input pipeline
	// actually subscribes to it.
	OnInputContextBuilt.Broadcast(ActiveInputMapping);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("InitializeInputMapping: no controller (will register when possessed)."));
		return;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("InitializeInputMapping: controller has no local player."));
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("InitializeInputMapping: EnhancedInput subsystem unavailable."));
		return;
	}

	if (Subsystem->HasMappingContext(ActiveInputMapping))
	{
		Subsystem->RemoveMappingContext(ActiveInputMapping);
	}

	Subsystem->AddMappingContext(ActiveInputMapping, InputMappingPriority);
	UE_LOG(LogSimulationCameraControl, Verbose, TEXT("InitializeInputMapping: Added %s with priority %d for %s."),
		*GetNameSafe(ActiveInputMapping), InputMappingPriority, *GetName());

	// Hook: post-registration. At this point the Enhanced Input pipeline is
	// live and the pawn's behaviors will start receiving Triggered/Completed
	// events on the next player input. Use this to play UI sounds, animate
	// HUD elements, kick off "input is now ready" notifications, etc.
	OnInputContextRegistered.Broadcast(ActiveInputMapping);
}

void ASimulationCameraControl::AddInputBehavior(UCameraInputBehavior* Behavior)
{
	if (!Behavior)
	{
		return;
	}
	if (Behaviors.Contains(Behavior))
	{
		return;
	}
	Behaviors.Add(Behavior);
	// Force a rebuild on next setup. ActiveInputMapping is left in place so
	// the current session's bindings stay valid until the next restart.
	ActiveInputMapping = nullptr;
}

void ASimulationCameraControl::RemoveInputBehavior(UCameraInputBehavior* Behavior)
{
	if (Behaviors.RemoveSingle(Behavior) > 0)
	{
		ActiveInputMapping = nullptr;
	}
}
