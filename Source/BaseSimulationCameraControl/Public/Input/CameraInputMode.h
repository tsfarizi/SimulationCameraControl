// Copyright Teuku. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CameraInputBindings.h"
#include "CameraInputMode.generated.h"

class UInputMappingContext;
class UCameraInputBehavior;

/**
 * UCameraInputMode
 * Bundles a complete input configuration (behaviors + bindings + priority) into
 * a single named "mode" that can be enabled or disabled at runtime. The pawn
 * holds a TArray of registered modes and tracks an ActiveModes set; the
 * ActiveInputMappings in the Enhanced Input subsystem are recomputed whenever
 * the active set changes.
 *
 * Each mode contributes one UInputMappingContext. When multiple modes are
 * active simultaneously (e.g., Base at priority 0 + UI at priority 20), the
 * Enhanced Input subsystem's priority resolution decides which action takes
 * effect for a given key press. Higher priority wins; the first-claim-wins rule
 * in the pawn's auto-bind loop further de-duplicates by action name across
 * behaviors.
 *
 * Designer workflow:
 *   1. Create one mode per logical control scheme (Base, Combat, UI, Cinematic).
 *   2. For each mode, fill in Behaviors[] (which actions it contributes) and
 *      optionally InputBindingsOverride (designer-edited spec list, falls back
 *      to the behavior aggregate if null).
 *   3. At runtime, call EnableMode(TEXT("Combat")) / DisableMode(TEXT("UI")) /
 *      SetExclusiveMode(TEXT("Cinematic")) from gameplay code or BP.
 *
 * Common pattern: a "Default" mode is auto-registered in the pawn constructor
 * with UCameraMovementBehavior, so the pawn works out of the box. Designers
 * add additional modes (UI, Combat, etc.) in the Details panel.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, CollapseCategories, DisplayName = "Camera Input Mode")
class BASESIMULATIONCAMERACONTROL_API UCameraInputMode : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Stable name used to enable/disable this mode at runtime. Must be unique
	 * across the pawn's RegisteredModes array. Default-mode auto-registers as
	 * "Default" by the pawn constructor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input")
	FName ModeName = NAME_None;

	/**
	 * Enhanced Input mapping context priority. Higher numbers win when two
	 * active modes contribute actions that bind to the same key. Recommend:
	 *   Base = 0
	 *   Combat / Build mode = 10
	 *   UI / Cinematic = 20
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input", meta = (ClampMin = "0"))
	int32 Priority = 0;

	/** Behaviors this mode contributes. Aggregated into the mode's IMC on build. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Camera Input")
	TArray<TObjectPtr<UCameraInputBehavior>> Behaviors;

	/**
	 * Optional override DataAsset. When set, its Actions[] fully replaces the
	 * Behaviors' aggregated spec list (the override's purpose is to let
	 * designers tune key bindings without subclassing the behavior). When null,
	 * the mode's IMC is built from the union of all Behaviors' GetActionSpecs().
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input")
	TObjectPtr<UCameraInputBindings> InputBindingsOverride;

	/**
	 * Cached built IMC. Lazily constructed by GetOrBuildContext() and reused
	 * across Enable/Disable cycles. Parent (Outer) is the pawn so GC keeps it
	 * alive with the pawn.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> BuiltContext;

	/**
	 * Returns the aggregated spec list for this mode: the override DataAsset's
	 * Actions[] if set, else the union of all Behaviors' GetActionSpecs().
	 * Used by GetOrBuildContext to construct the IMC and by callers that want
	 * to inspect which actions a mode contributes without building the IMC.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Input")
	TArray<FCameraInputActionSpec> GetActionSpecs() const;

	/**
	 * Returns the cached BuiltContext, building it on first call. Subsequent
	 * calls return the same instance until InvalidateBuiltContext is called.
	 *
	 * Outer is the supplied parent (typically the pawn) so GC keeps the IMC
	 * alive with the pawn. Returns nullptr if the mode has no actions to map.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Input")
	UInputMappingContext* GetOrBuildContext(UObject* Outer);

	/**
	 * Drop the cached BuiltContext. Call after mutating Behaviors or
	 * InputBindingsOverride to force a rebuild on the next GetOrBuildContext call.
	 * The pawn calls this automatically on EnableMode/DisableMode when
	 * RegisteredModes changes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Input")
	void InvalidateBuiltContext();
};
