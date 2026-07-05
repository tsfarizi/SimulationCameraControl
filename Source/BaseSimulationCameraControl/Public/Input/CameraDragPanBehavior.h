// Copyright Teuku. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraInputBehavior.h"
#include "CameraInputBindings.h"
#include "CameraDragPanBehavior.generated.h"

/**
 * UCameraDragPanBehavior
 * Adds a "click-and-drag to pan" input mode, the standard sim-game camera
 * control (Cities Skylines, Planet Zoo, the Unreal editor itself).
 *
 * - IA_Drag_Modifier (Bool) is the gate key. Default binding is LeftMouseButton
 *   but the designer can rebind it via the InputBindingsOverride DataAsset or
 *   a per-behavior DataAsset slot.
 * - IA_Drag_Pan (Axis2D) reads pointer movement (default Mouse2D) while the
 *   modifier is held.
 * - HandleAction forwards the pan vector to the pawn's Pan() method with
 *   InvertAxis applied, so the default "drag right moves camera left"
 *   (the "grab the world and pull" feel) is the default.
 *
 * The class name and action names are intentionally NOT prefixed with "Left"
 * so the same behavior can be re-bound to any modifier key (right mouse,
 * middle mouse, gamepad shoulder, etc.) without renaming. The default
 * binding is left mouse for the common sim-game convention.
 *
 * Independent of UCameraMovementBehavior (which handles the middle-mouse /
 * WASD pan). The two can be stacked: middle-mouse-drag pans normally,
 * modifier-drag pans with the inverted direction. Only one is active at a
 * time because each has its own modifier key.
 *
 * Designer can tune:
 *   - PanSpeedMultiplier : extra scale on top of the pawn's PanSpeed.
 *     Default 1.0; raise for a faster pan, lower for a slower, more
 *     precise pan.
 *   - InvertAxis : per-axis sign flip. Default (-1, -1) gives the
 *     "inverse direction" feel; flip to (1, 1) for direct
 *     "camera follows the cursor" feel.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, CollapseCategories, DisplayName = "Camera Drag Pan")
class BASESIMULATIONCAMERACONTROL_API UCameraDragPanBehavior : public UCameraInputBehavior
{
	GENERATED_BODY()

public:
	/** Extra scale on top of the pawn's PanSpeed. Default 1.0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input", meta = (ClampMin = "0.0"))
	float PanSpeedMultiplier = 1.0f;

	/**
	 * Per-axis sign flip applied to the input vector before calling Pawn->Pan().
	 * Default (-1, -1): dragging right moves camera left, dragging down moves
	 * camera up — the "inverse direction" feel typical of sim games. Set to
	 * (1, 1) for the direct "camera follows the cursor" feel used by some
	 * editor-style viewports.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input")
	FVector2D InvertAxis = FVector2D(-1.0f, -1.0f);

	//~ Begin UCameraInputBehavior
	virtual TArray<FCameraInputActionSpec> GetActionSpecs_Implementation() const override;
	virtual void HandleAction_Implementation(FName ActionName, const FInputActionValue& Value, UObject* Owner) override;
	//~ End UCameraInputBehavior

private:
	/** Local modifier state. Read/written by HandleAction on IA_Drag_Modifier events. */
	UPROPERTY(Transient)
	bool bIsDragActive = false;
};
