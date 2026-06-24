// Copyright Teuku. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraInputBehavior.h"
#include "CameraInputBindings.h"
#include "CameraLeftDragPanBehavior.generated.h"

/**
 * UCameraLeftDragPanBehavior
 * Adds a "click-and-drag to pan" input mode, the standard sim-game camera
 * control (Cities Skylines, Planet Zoo, the Unreal editor itself).
 *
 * - IA_LeftDrag_Modifier (Bool) is held by the left mouse button.
 * - IA_LeftDrag_Pan (Axis2D) reads mouse movement (Mouse2D) while the
 *   modifier is held.
 * - HandleAction forwards the pan vector to the pawn's Pan() method with
 *   InvertAxis applied, so dragging right moves the camera left (the
 *   "grab the world and pull" feel).
 *
 * This behavior is independent of UCameraMovementBehavior (which handles
 * the middle-mouse / WASD pan). The two can be stacked in the pawn's
 * Behaviors array: middle-mouse-drag pans normally, left-mouse-drag pans
 * with the inverted direction. Only one is active at a time because each
 * has its own modifier (left vs middle mouse).
 *
 * Designer can tune:
 *   - PanSpeedMultiplier : extra scale on top of the pawn's PanSpeed.
 *     Default 1.0; raise for a faster pan, lower for a slower, more
 *     precise pan.
 *   - InvertAxis : per-axis sign flip. Default (-1, -1) gives the
 *     "inverse direction" feel requested; flip to (1, 1) for direct
 *     "camera follows the cursor" feel.
 *   - bRequireExactModifierState : when true, only Triggered events
 *     (button freshly pressed) start the pan; when false, both Triggered
 *     and any value > 0 sustain it (more forgiving, matches the
 *     UCameraMovementBehavior modifier handling style).
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, CollapseCategories, DisplayName = "Camera Left Drag Pan")
class SIMULATIONCAMERACONTROL_API UCameraLeftDragPanBehavior : public UCameraInputBehavior
{
	GENERATED_BODY()

public:
	/** Extra scale on top of the pawn's PanSpeed. Default 1.0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input", meta = (ClampMin = "0.0"))
	float PanSpeedMultiplier = 1.0f;

	/**
	 * Per-axis sign flip applied to the Mouse2D vector before calling Pawn->Pan().
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
	/** Local modifier state. Read/written by HandleAction on IA_LeftDrag_Modifier events. */
	UPROPERTY(Transient)
	bool bIsLeftDragActive = false;
};
