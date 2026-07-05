// Copyright Teuku. All Rights Reserved.

#include "CameraInputMode.h"
#include "CameraInputBehavior.h"
#include "InputAction.h"
#include "InputMappingContext.h"

TArray<FCameraInputActionSpec> UCameraInputMode::GetActionSpecs() const
{
	if (InputBindingsOverride)
	{
		return InputBindingsOverride->Actions;
	}

	TArray<FCameraInputActionSpec> AllSpecs;
	for (UCameraInputBehavior* Behavior : Behaviors)
	{
		if (Behavior)
		{
			AllSpecs.Append(Behavior->GetActionSpecs());
		}
	}
	return AllSpecs;
}

UInputMappingContext* UCameraInputMode::GetOrBuildContext(UObject* Outer)
{
	if (BuiltContext)
	{
		return BuiltContext;
	}

	UObject* EffectiveOuter = Outer ? Outer : GetTransientPackage();

	const TArray<FCameraInputActionSpec> Specs = GetActionSpecs();
	if (Specs.Num() == 0)
	{
		// No actions to register. Return nullptr so the caller knows to skip
		// this mode (e.g., the user created an empty "placeholder" mode that's
		// not yet configured).
		return nullptr;
	}

	// Build via the same UCameraInputBindings::BuildContext path the legacy
	// single-IMC flow used, so we get identical behavior (key binding,
	// swizzle, negate) without duplicating the construction logic.
	UCameraInputBindings* TempBindings = NewObject<UCameraInputBindings>(
		EffectiveOuter, UCameraInputBindings::StaticClass(),
		FName(*FString::Printf(TEXT("ModeBindings_%s"), *ModeName.ToString())),
		RF_Transient);
	if (TempBindings)
	{
		TempBindings->Actions = Specs;
		BuiltContext = TempBindings->BuildContext(EffectiveOuter);
	}

	return BuiltContext;
}

void UCameraInputMode::InvalidateBuiltContext()
{
	BuiltContext = nullptr;
}
