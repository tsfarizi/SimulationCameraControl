#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SimulationCameraControlPawn.generated.h"

// Forward declarations for faster compile time
class UCameraComponent;
class USpringArmComponent;
class USceneComponent;
class UInputAction;
class UInputMappingContext;
class UCameraInputBindings;
class UCameraInputBehavior;
class UCameraInputMode;
struct FInputActionInstance;

/** Fires when the pawn has built (or rebuilt) its in-memory UInputMappingContext,
 *  before registration with the Enhanced Input subsystem. Use to inject runtime-only
 *  triggers, dynamically add keys, or tag the context for downstream systems.
 *  The context is parented to the pawn, so any UObject you NewObject with this
 *  context as Outer gets GC'd with the pawn.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputContextBuilt, UInputMappingContext*, NewContext);

/** Fires when the pawn has registered its IMC with the Enhanced Input subsystem
 *  (i.e., the player's input pipeline is now hot). Use to play UI sounds, animate
 *  HUD elements, or start auxiliary systems that need to know input is live.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputContextRegistered, UInputMappingContext*, RegisteredContext);

/**
 * Specialized camera pawn for simulation controls.
 * Implements RTS-style camera controls (Zoom, Orbit, Pan) with smooth interpolation.
 */
UCLASS()
class SIMULATIONCAMERACONTROL_API ASimulationCameraControl : public APawn
{
	GENERATED_BODY()

public:
	ASimulationCameraControl();

	//~ Begin APawn Interface
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void PawnClientRestart() override;
	//~ End APawn Interface

	/**
	 * Enables or disables all camera input (e.g., when cursor hovers UI widgets).
	 * Caller: Enhanced Input System IA_* bound to UI focus events.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	void SetInputEnabled(bool bInEnabled);

	/**
	 * Zooms by adjusting spring arm length while sliding pawn to keep cursor focus steady.
	 * Axis source: mouse wheel (+/-1). Flip sign via bInvertZoom if required.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	void Zoom(float AxisValue);

	/**
	 * Orbits the spring arm around the pawn using yaw/pitch deltas.
	 * Axis.X = MouseX, Axis.Y = MouseY, values typically in +/-1 per Enhanced Input sample.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	void Orbit(FVector2D AxisValue);

	/**
	 * Pans the pawn in world X/Y based on camera yaw so controls remain screen-relative.
	 * Suggested bindings: WASD or middle-mouse drag; expects continuous IA_* Triggered events.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	void Pan(FVector2D AxisValue);

	// Setter BP-callable
	UFUNCTION(BlueprintCallable, Category="Camera|Input")
	void SetInputMappingPriority(int32 InPriority);

	// Modifier state accessors for input behaviors.
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	bool IsOrbitModifierDown() const { return bIsOrbitModifierDown; }
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	void SetOrbitModifierDown(bool bDown) { bIsOrbitModifierDown = bDown; }
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	bool IsPanModifierDown() const { return bIsPanModifierDown; }
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	void SetPanModifierDown(bool bDown) { bIsPanModifierDown = bDown; }

	/**
	 * Add a behavior to a registered mode (creating the mode first if ModeName
	 * doesn't exist). Useful for feature plugins that want to extend the
	 * camera's input surface at runtime without modifying the pawn's modes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	void AddInputBehavior(UCameraInputBehavior* Behavior, FName ModeName = TEXT("Default"));

	/**
	 * Remove a previously-added behavior. Safe to call with a stale pointer.
	 * If the behavior is currently inside a mode's Behaviors[] array, it's
	 * removed from there and that mode's IMC is invalidated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	void RemoveInputBehavior(UCameraInputBehavior* Behavior);

	/**
	 * Drop all cached IMCs and force a full rebuild on the next input tick.
	 * Use after mutating mode data (InputBindingsOverride, Behaviors, etc.) or
	 * after a settings reload. The actual rebuild happens on the next
	 * PossessedBy / PawnClientRestart cycle, or immediately if you call
	 * RefreshActiveInputMappings() right after.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	void RebuildInputContext();

	/**
	 * Force a full rebuild of the active IMCs and re-register them with the
	 * Enhanced Input subsystem right now. Useful when you want the input
	 * change to take effect immediately (e.g., after editing a key binding
	 * via an in-game settings menu).
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	void RefreshActiveInputMappings();

	/**
	 * Enable a mode by name. The mode's IMC is built (if not cached) and
	 * registered with the Enhanced Input subsystem at the mode's Priority.
	 * No-op if the mode is already active. No-op if ModeName doesn't match
	 * any registered mode.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	bool EnableMode(FName ModeName);

	/**
	 * Disable a mode by name. Its IMC is unregistered from the Enhanced Input
	 * subsystem and removed from ActiveMappingContexts. No-op if the mode
	 * wasn't active.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	bool DisableMode(FName ModeName);

	/**
	 * Disable all currently-active modes and enable only the named one. Use
	 * for "switch to this single mode" transitions (e.g., entering a
	 * cinematic that needs to lock all gameplay input).
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	bool SetExclusiveMode(FName ModeName);

	/** Returns true if ModeName is in the ActiveModes set. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	bool IsModeActive(FName ModeName) const { return ActiveModes.Contains(ModeName); }

	/** Toggles the mode: enables if disabled, disables if enabled. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	bool ToggleMode(FName ModeName);

	/**
	 * Fired after the in-memory UInputMappingContext has been built but before
	 * it is registered with the Enhanced Input subsystem. Subscribers can mutate
	 * the context (add triggers, swap modifiers, change key bindings) before
	 * the player's input pipeline sees it. Fires once per active mode.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Camera|Input")
	FOnInputContextBuilt OnInputContextBuilt;

	/**
	 * Fired after the IMC has been registered with the Enhanced Input subsystem.
	 * At this point the player's input pipeline is hot and the pawn will start
	 * receiving Triggered/Completed events on its behaviors. Fires once per
	 * active mode.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Camera|Input")
	FOnInputContextRegistered OnInputContextRegistered;

protected:
	/** Root component - keeps explicit hierarchy Root -> SpringArm -> Camera. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	/** Spring arm driving orbital rotation; collision test disabled for unobstructed control. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArm;

	/** Active camera placed at spring arm tip. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

	/** Minimum boom length in centimeters (cm). Safe range: 200-800. Smaller risks clipping geometry when zooming in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "10.0"))
	float MinArmLength = 400.0f;

	/** Maximum boom length in centimeters (cm). Safe range: 1200-4000. Larger values exaggerate parallax and increase skybox exposure. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "100.0"))
	float MaxArmLength = 2500.0f;

	/** Zoom step in centimeters (cm) per wheel tick. Safe range: 25-250. Bigger steps feel snappier but reduce precision. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "1.0"))
	float ZoomStep = 120.0f;

	/** Optional inversion for zoom axis; set true to swap wheel direction. Side effect: affects all input devices uniformly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	bool bInvertZoom = false;

	/** Yaw orbit speed in degrees/second. Safe range: 45-360. Values too high can induce motion sickness. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMin = "0.0"))
	float OrbitYawSpeed = 120.0f;

	/** Pitch orbit speed in degrees/second. Safe range: 30-180. Higher speeds risk overshooting clamps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMin = "0.0"))
	float OrbitPitchSpeed = 90.0f;

	/** Minimum pitch in degrees (negative keeps top-down). Safe range: -89 to -10. Prevents flipping underneath the world. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMax = "0.0"))
	float MinPitch = -75.0f;

	/** Maximum pitch in degrees (negative for downward tilt). Safe range: -89 to -5. Clamped to avoid gimbal flip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMax = "0.0"))
	float MaxPitch = -30.0f;

	/** Pan speed in centimeters/second. Safe range: 300-3000. Directly scales WASD traversal across the ground plane. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pan", meta = (ClampMin = "0.0"))
	float PanSpeed = 1500.0f;

	/** Ray length in centimeters (cm) for cursor focus traces. Safe range: 5000-200000. Longer rays cover tall levels but cost trace time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus", meta = (ClampMin = "100.0"))
	float RayLength = 50000.0f;

	/** World Z plane (cm) used when traces miss. Choose ground height; fallback plane stabilizes zoom over empty space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus")
	float GroundZ = 0.0f;

	/** Distance tolerance in centimeters to accept new focus hits. Safe range: 1-500. Higher tolerates cursor jumps; lower keeps micro precision. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus", meta = (ClampMin = "0.0"))
	float JumpThreshold = 100.0f;

	/** Master input gate. False disables Zoom/Orbit/Pan; use when interacting with UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input")
	bool bInputEnabled = true;

	/** Optional debug visualizations (draws rays/spheres). Safe to toggle at runtime; may spam logs/lines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Debug")
	bool bDebug = false;

	/**
	 * Registered input modes. Each mode bundles behaviors + bindings override +
	 * priority into a named "package" that can be enabled or disabled at
	 * runtime. The pawn builds and registers one UInputMappingContext per
	 * active mode; multiple active modes stack with priority-based conflict
	 * resolution (Enhanced Input subsystem's native behavior).
	 *
	 * The default constructor auto-registers a "Default" mode containing a
	 * UCameraMovementBehavior, so the pawn works out of the box. Designers
	 * add additional modes (UI, Combat, Cinematic, etc.) in the Details panel.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Camera|Input")
	TArray<TObjectPtr<UCameraInputMode>> RegisteredModes;

	/** Priority fallback used by the (legacy) single-IMC initialization when no mode is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input", meta = (ClampMin = "0"))
	int32 InputMappingPriority = 0;

	/**
	 * Active IMCs currently registered with the Enhanced Input subsystem.
	 * Populated by InitializeInputMapping (and re-populated by EnableMode /
	 * DisableMode). One entry per active mode.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputMappingContext>> ActiveMappingContexts;

	/** Interpolation speed for zoom smoothing (arm length). Higher = faster response. Safe range: 5.0-30.0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing", meta = (ClampMin = "0.1"))
	float ZoomInterpSpeed = 15.0f;

	/** Interpolation speed for orbit smoothing (rotation). Higher = faster response. Safe range: 5.0-30.0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing", meta = (ClampMin = "0.1"))
	float OrbitInterpSpeed = 15.0f;

	/** Interpolation speed for pan smoothing (location). Higher = faster response. Safe range: 5.0-30.0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing", meta = (ClampMin = "0.1"))
	float PanInterpSpeed = 15.0f;

	/** Enables smooth interpolation for zoom. Disable for instant response. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing")
	bool bSmoothZoom = true;

	/** Enables smooth interpolation for orbit. Disable for instant response. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing")
	bool bSmoothOrbit = true;

	/** Enables smooth interpolation for pan. Disable for instant response. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing")
	bool bSmoothPan = true;

private:
	/** Returns cursor world point, preferring hits then falling back to GroundZ plane; logs failure reasons. */
	bool GetCursorWorldPoint(FVector& OutPoint);

	/** Provides a stable focus by caching previous hits and rejecting large jumps. */
	FVector GetStableFocusPoint();

	/** Applies zoom by clamping arm length and repositioning pawn along focus direction. */
	void ApplyZoom(float DesiredArmLength, const FVector& FocusPoint);

	/**
	 * Builds and registers IMCs for all active modes. Each mode contributes one
	 * IMC; modes stack with the Enhanced Input subsystem's priority resolution.
	 * No-op if ActiveMappingContexts is already populated.
	 */
	void InitializeInputMapping();

	/**
	 * Tears down and rebuilds the active IMC set. Called by EnableMode /
	 * DisableMode / SetExclusiveMode / RefreshActiveInputMappings.
	 */
	void RebuildActiveMappingContexts();

	/**
	 * Auto-binds every action across all active IMCs to the first behavior in
	 * the active modes' Behaviors[] arrays that claims it. Called from
	 * SetupPlayerInputComponent (first init) and from RebuildActiveMappingContexts
	 * (mode swap). For boolean (modifier) actions, both Triggered and Completed
	 * events are bound so the pressed/released state is captured.
	 */
	void AutoBindBehaviorsToActiveContexts(class UEnhancedInputComponent* EnhancedComponent);

	/** The set of mode names currently enabled. The pawn's "active" set. */
	UPROPERTY(Transient)
	TSet<FName> ActiveModes;

	/** Tracks whether the Orbit Modifier (Right Mouse) is held down. Read/write by input behaviors. */
	UPROPERTY(Transient)
	bool bIsOrbitModifierDown = false;

	/** Tracks whether the Pan Modifier (Middle Mouse) is held down. Read/write by input behaviors. */
	UPROPERTY(Transient)
	bool bIsPanModifierDown = false;

	/** Cached focus location to smooth zoom operations. */
	FVector LastValidHitLocation = FVector::ZeroVector;

	/** Tracks whether LastValidHitLocation is initialized. */
	bool bHasCachedFocus = false;

	/** Tracks whether the Orbit Modifier (Right Mouse) is held down. (Read/write by input behaviors; declared as UPROPERTY above for accessor support.) */

	/** Tracks whether the Pan Modifier (Middle Mouse) is held down. (Read/write by input behaviors; declared as UPROPERTY above for accessor support.) */


	// Smoothing state variables
	/** Target arm length for smooth zoom interpolation. */
	float TargetArmLength = 400.0f;

	/** Target relative rotation for smooth orbit interpolation. */
	FRotator TargetRelativeRotation = FRotator(-60.0f, 0.0f, 0.0f);

	/** Target actor location for smooth pan interpolation. */
	FVector TargetActorLocation = FVector::ZeroVector;

	/** Tracks if initial target values have been set. */
	bool bTargetsInitialized = false;
};
