// Copyright Teuku. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraInputBindings.h"

class UInputMappingContext;

/**
 * CameraInputDefaults
 * Hardcoded fallback used when the pawn has no InputBindingsOverride assigned.
 * The defaults mirror PopulateDefaultActions() on UCameraInputBindings, but
 * they are returned as a TArray so callers don't need a transient DataAsset
 * to feed BuildContext.
 */
namespace CameraInputDefaults
{
	/** The 5 actions every SimulationCameraControl pawn expects by name. */
	SIMULATIONCAMERACONTROL_API TArray<FCameraInputActionSpec> GetDefaultActionSpecs();

	/**
	 * Build a runtime UInputMappingContext from the hardcoded defaults.
	 * `Outer` becomes the IMC's UObject parent (pass the pawn so the GC
	 * keeps the context alive alongside the pawn). Falls back to the
	 * transient package if Outer is null.
	 */
	SIMULATIONCAMERACONTROL_API UInputMappingContext* MakeDefaultContext(UObject* Outer);
}
