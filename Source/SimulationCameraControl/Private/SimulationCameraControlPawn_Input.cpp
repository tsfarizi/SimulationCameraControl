// Copyright Teuku. All Rights Reserved.

#include "SimulationCameraControlPawn.h"
#include "SimulationCameraControlPawn_Internal.h"
#include "CameraInputBindings.h"
#include "CameraInputDefaults.h"
#include "CameraInputBehavior.h"
#include "CameraInputMode.h"
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

	// Build (or reuse) the active IMC set. InitializeInputMapping stores them in
	// ActiveMappingContexts so they survive until the pawn is destroyed.
	if (ActiveMappingContexts.Num() == 0)
	{
		InitializeInputMapping();
	}

	if (ActiveMappingContexts.Num() == 0)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("SetupPlayerInputComponent: no active mapping contexts (InitializeInputMapping failed)."));
		return;
	}

	AutoBindBehaviorsToActiveContexts(EnhancedComponent);
}

void ASimulationCameraControl::AutoBindBehaviorsToActiveContexts(UEnhancedInputComponent* EnhancedComponent)
{
	if (!EnhancedComponent || ActiveMappingContexts.Num() == 0)
	{
		return;
	}

	// Aggregate behaviors from all active modes. The first mode in the list
	// has the highest priority claim on shared action names (because its IMC
	// is registered first; see RebuildActiveMappingContexts). The order in
	// which the behaviors are visited here breaks any remaining ties.
	TArray<UCameraInputBehavior*> AggregatedBehaviors;
	for (UCameraInputMode* Mode : RegisteredModes)
	{
		if (Mode && ActiveModes.Contains(Mode->ModeName))
		{
			for (UCameraInputBehavior* Behavior : Mode->Behaviors)
			{
				if (Behavior)
				{
					AggregatedBehaviors.AddUnique(Behavior);
				}
			}
		}
	}

	if (AggregatedBehaviors.Num() == 0)
	{
		return;
	}

	// Build a TMap<UInputAction*, TWeakObjectPtr<UCameraInputBehavior>> for O(1) dispatch.
	// We iterate every action across all active IMCs and look up which behavior
	// claimed it. The first behavior in AggregatedBehaviors that claims a given
	// action name wins.
	TMap<const UInputAction*, TWeakObjectPtr<UCameraInputBehavior>> ActionToBehavior;

	for (UInputMappingContext* Context : ActiveMappingContexts)
	{
		if (!Context)
		{
			continue;
		}

		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (!Mapping.Action)
			{
				continue;
			}
			const UInputAction* ActionPtr = Mapping.Action.Get();
			const FName ActionName = Mapping.Action->GetFName();

			TWeakObjectPtr<UCameraInputBehavior>* Existing = ActionToBehavior.Find(ActionPtr);
			if (Existing && Existing->IsValid())
			{
				continue;
			}

			for (UCameraInputBehavior* Behavior : AggregatedBehaviors)
			{
				if (Behavior && Behavior->HandlesAction(ActionName))
				{
					ActionToBehavior.Add(ActionPtr, Behavior);
					break;
				}
			}
		}
	}

	// Bind each (action, behavior) pair. The lambda captures a TWeakObjectPtr so
	// a behavior that gets GC'd between registration and event fire becomes a
	// no-op rather than a crash.
	for (const TPair<const UInputAction*, TWeakObjectPtr<UCameraInputBehavior>>& Pair : ActionToBehavior)
	{
		const UInputAction* Action = Pair.Key;
		TWeakObjectPtr<UCameraInputBehavior> WeakBehavior = Pair.Value;

		if (!Action)
		{
			continue;
		}

		const FName ActionName = Action->GetFName();

		EnhancedComponent->BindActionValueLambda(Action, ETriggerEvent::Triggered,
			[WeakBehavior, ActionName, this](const FInputActionValue& Value)
			{
				if (UCameraInputBehavior* Behavior = WeakBehavior.Get())
				{
					Behavior->HandleAction(ActionName, Value, this);
				}
			});

		// Boolean actions (modifier keys) also need the Completed event.
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

	// Warn about registered actions with no behavior claim.
	for (UInputMappingContext* Context : ActiveMappingContexts)
	{
		if (!Context) continue;
		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (Mapping.Action && !ActionToBehavior.Contains(Mapping.Action.Get()))
			{
				UE_LOG(LogSimulationCameraControl, Warning, TEXT("Action '%s' is registered in an active mapping context but no behavior in any active mode claims it. Add a behavior with GetActionSpecs() declaring this action, or remove the entry from the mode's InputBindingsOverride."),
					*Mapping.Action->GetName());
			}
		}
	}
}

void ASimulationCameraControl::InitializeInputMapping()
{
	// First-time init: register all currently-active modes. The constructor
	// auto-adds "Default" to ActiveModes so the pawn works out of the box.
	if (ActiveMappingContexts.Num() > 0)
	{
		return;
	}

	// If the designer hasn't set any active modes (e.g., cleared everything
	// in the editor), default to "Default" so the pawn is still functional.
	if (ActiveModes.Num() == 0)
	{
		bool bHasDefault = false;
		for (UCameraInputMode* Mode : RegisteredModes)
		{
			if (Mode && Mode->ModeName == FName(TEXT("Default")))
			{
				bHasDefault = true;
				break;
			}
		}
		if (bHasDefault)
		{
			ActiveModes.Add(FName(TEXT("Default")));
		}
	}

	RebuildActiveMappingContexts();
}

void ASimulationCameraControl::RebuildActiveMappingContexts()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	UEnhancedInputLocalPlayerSubsystem* Subsystem = nullptr;
	if (PC)
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		}
	}

	// Tear down existing contexts.
	if (Subsystem)
	{
		for (UInputMappingContext* Context : ActiveMappingContexts)
		{
			if (Context)
			{
				Subsystem->RemoveMappingContext(Context);
			}
		}
	}
	ActiveMappingContexts.Reset();

	// Build new contexts. Modes are visited in RegisteredModes order; the first
	// mode in the array gets its IMC registered first, so the Enhanced Input
	// subsystem's priority resolution favors it for any shared action name.
	for (UCameraInputMode* Mode : RegisteredModes)
	{
		if (!Mode || !ActiveModes.Contains(Mode->ModeName))
		{
			continue;
		}

		UInputMappingContext* Context = Mode->GetOrBuildContext(this);
		if (!Context)
		{
			UE_LOG(LogSimulationCameraControl, Verbose, TEXT("RebuildActiveMappingContexts: mode '%s' produced no IMC (empty spec list)."),
				*Mode->ModeName.ToString());
			continue;
		}

		// Hook: post-build, pre-registration.
		OnInputContextBuilt.Broadcast(Context);

		if (Subsystem)
		{
			const int32 EffectivePriority = (Mode->Priority > 0) ? Mode->Priority : InputMappingPriority;
			Subsystem->AddMappingContext(Context, EffectivePriority);
		}

		ActiveMappingContexts.Add(Context);

		UE_LOG(LogSimulationCameraControl, Verbose, TEXT("RebuildActiveMappingContexts: mode '%s' -> %s (priority %d)"),
			*Mode->ModeName.ToString(),
			*GetNameSafe(Context),
			Mode->Priority);
	}

	// Re-bind to behaviors across all active IMCs.
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		AutoBindBehaviorsToActiveContexts(EIC);
	}

	// Hook: post-registration, per active mode. Subscribers can react to
	// "this mode is now live" individually.
	for (UInputMappingContext* Context : ActiveMappingContexts)
	{
		if (Context)
		{
			OnInputContextRegistered.Broadcast(Context);
		}
	}
}

bool ASimulationCameraControl::EnableMode(FName ModeName)
{
	UCameraInputMode* Mode = nullptr;
	for (UCameraInputMode* Candidate : RegisteredModes)
	{
		if (Candidate && Candidate->ModeName == ModeName)
		{
			Mode = Candidate;
			break;
		}
	}

	if (!Mode)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("EnableMode('%s'): no registered mode with that name."), *ModeName.ToString());
		return false;
	}

	if (ActiveModes.Contains(ModeName))
	{
		// Already active; no-op.
		return true;
	}

	ActiveModes.Add(ModeName);

	// Force a rebuild so the new mode's IMC is added to the subsystem.
	ActiveMappingContexts.Reset();
	RebuildActiveMappingContexts();
	return true;
}

bool ASimulationCameraControl::DisableMode(FName ModeName)
{
	if (!ActiveModes.Contains(ModeName))
	{
		// Already disabled; no-op.
		return true;
	}

	ActiveModes.Remove(ModeName);

	// Force a rebuild so the mode's IMC is removed.
	ActiveMappingContexts.Reset();
	RebuildActiveMappingContexts();
	return true;
}

bool ASimulationCameraControl::SetExclusiveMode(FName ModeName)
{
	UCameraInputMode* Mode = nullptr;
	for (UCameraInputMode* Candidate : RegisteredModes)
	{
		if (Candidate && Candidate->ModeName == ModeName)
		{
			Mode = Candidate;
			break;
		}
	}

	if (!Mode)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("SetExclusiveMode('%s'): no registered mode with that name."), *ModeName.ToString());
		return false;
	}

	// Disable everything except the target.
	for (UCameraInputMode* Other : RegisteredModes)
	{
		if (Other && Other->ModeName != ModeName)
		{
			ActiveModes.Remove(Other->ModeName);
		}
	}
	ActiveModes.Add(ModeName);

	ActiveMappingContexts.Reset();
	RebuildActiveMappingContexts();
	return true;
}

bool ASimulationCameraControl::ToggleMode(FName ModeName)
{
	if (IsModeActive(ModeName))
	{
		return DisableMode(ModeName);
	}
	return EnableMode(ModeName);
}

void ASimulationCameraControl::AddInputBehavior(UCameraInputBehavior* Behavior, FName ModeName)
{
	if (!Behavior)
	{
		return;
	}

	// Find the named mode, or the first registered mode if ModeName is None.
	UCameraInputMode* TargetMode = nullptr;
	if (!ModeName.IsNone())
	{
		for (UCameraInputMode* Mode : RegisteredModes)
		{
			if (Mode && Mode->ModeName == ModeName)
			{
				TargetMode = Mode;
				break;
			}
		}
	}
	else
	{
		for (UCameraInputMode* Mode : RegisteredModes)
		{
			if (Mode)
			{
				TargetMode = Mode;
				break;
			}
		}
	}

	if (!TargetMode)
	{
		UE_LOG(LogSimulationCameraControl, Warning, TEXT("AddInputBehavior: no registered mode to attach to."));
		return;
	}

	if (TargetMode->Behaviors.Contains(Behavior))
	{
		return;
	}
	TargetMode->Behaviors.Add(Behavior);

	// Force rebuild only if the target mode is currently active.
	if (ActiveModes.Contains(TargetMode->ModeName))
	{
		TargetMode->InvalidateBuiltContext();
		ActiveMappingContexts.Reset();
		RebuildActiveMappingContexts();
	}
}

void ASimulationCameraControl::RemoveInputBehavior(UCameraInputBehavior* Behavior)
{
	if (!Behavior)
	{
		return;
	}

	bool bFoundInActive = false;
	for (UCameraInputMode* Mode : RegisteredModes)
	{
		if (!Mode) continue;
		const int32 Removed = Mode->Behaviors.RemoveSingle(Behavior);
		if (Removed > 0)
		{
			if (ActiveModes.Contains(Mode->ModeName))
			{
				Mode->InvalidateBuiltContext();
				bFoundInActive = true;
			}
		}
	}

	if (bFoundInActive)
	{
		ActiveMappingContexts.Reset();
		RebuildActiveMappingContexts();
	}
}

void ASimulationCameraControl::RefreshActiveInputMappings()
{
	ActiveMappingContexts.Reset();
	RebuildActiveMappingContexts();
}
