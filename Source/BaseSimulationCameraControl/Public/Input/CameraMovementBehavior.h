// Copyright Teuku. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraInputBehavior.h"
#include "CameraMovementBehavior.generated.h"

class ABaseSimulationCameraControl;

/**
 * UCameraMovementBehavior
 * Default camera-movement input behavior. Owns the 5 standard actions the
 * SimulationCameraControl pawn expects (IA_Zoom, IA_Orbit, IA_Orbit_Modifier,
 * IA_Pan, IA_Pan_Modifier) and dispatches HandleAction to the pawn's
 * Zoom/Orbit/Pan methods, with the modifier state held on the pawn.
 *
 * The pawn's constructor instantiates one of these by default so the camera
 * works out of the box. Designers can add more behaviors (boost, focus, etc.)
 * alongside this one without modifying any C++.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, CollapseCategories, DisplayName = "Camera Movement")
class BASESIMULATIONCAMERACONTROL_API UCameraMovementBehavior : public UCameraInputBehavior
{
	GENERATED_BODY()

public:
	//~ Begin UCameraInputBehavior
	virtual TArray<FCameraInputActionSpec> GetActionSpecs_Implementation() const override;
	virtual void HandleAction_Implementation(FName ActionName, const FInputActionValue& Value, UObject* Owner) override;
	//~ End UCameraInputBehavior
};
