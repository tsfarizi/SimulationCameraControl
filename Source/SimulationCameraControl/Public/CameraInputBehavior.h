// Copyright Teuku. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InputActionValue.h"
#include "CameraInputBindings.h"
#include "CameraInputBehavior.generated.h"

/**
 * UCameraInputBehavior
 * Base class for modular input behaviors attached to ASimulationCameraControl.
 *
 * Each behavior contributes zero or more action specs (FCameraInputActionSpec)
 * via GetActionSpecs() and implements HandleAction() to receive Triggered/
 * Completed events for the actions it owns. The pawn's SetupPlayerInputComponent
 * auto-binds each action to the first behavior whose HandlesAction() returns true
 * for that action name.
 *
 * To add a new input-driven feature:
 *   1. Subclass UCameraInputBehavior (or UCameraMovementBehavior for movement
 *      that needs access to the pawn's camera state).
 *   2. Override GetActionSpecs() to declare the actions this behavior owns
 *      (action name, value type, default keys).
 *   3. Override HandleAction() to implement the behavior.
 *   4. Add an instance of the new behavior to the pawn's Behaviors array in
 *      the editor (EditInline). No C++ changes to the pawn are required.
 *
 * Behaviors are stored as UObject instances owned by the pawn (Outer = pawn),
 * so they're GC-safe and Bind lambdas should hold TWeakObjectPtr<UCameraInputBehavior>.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, CollapseCategories, DefaultToInstanced)
class SIMULATIONCAMERACONTROL_API UCameraInputBehavior : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * The action specs this behavior wants to register with the active input context.
	 * Default: empty. Subclasses override to declare their actions.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Input")
	TArray<FCameraInputActionSpec> GetActionSpecs() const;
	virtual TArray<FCameraInputActionSpec> GetActionSpecs_Implementation() const { return TArray<FCameraInputActionSpec>(); }

	/**
	 * Called by the pawn's SetupPlayerInputComponent for each Triggered (and
	 * Completed, for boolean modifier actions) event on any of the actions
	 * this behavior declares. ActionName identifies which action fired.
	 *
	 * Default: no-op. Subclasses dispatch on ActionName to their own handlers.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Input")
	void HandleAction(FName ActionName, const FInputActionValue& Value, UObject* Owner);
	virtual void HandleAction_Implementation(FName ActionName, const FInputActionValue& Value, UObject* Owner) {}

	/**
	 * Returns true if this behavior declares an action with the given name.
	 * The pawn uses this to dispatch input events to the right behavior when
	 * multiple behaviors are stacked.
	 */
	virtual bool HandlesAction(FName ActionName) const
	{
		if (ActionName.IsNone())
		{
			return false;
		}
		for (const FCameraInputActionSpec& Spec : GetActionSpecs())
		{
			if (Spec.ActionName == ActionName)
			{
				return true;
			}
		}
		return false;
	}
};
