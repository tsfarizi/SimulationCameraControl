#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraInputComponent.h"
#include "CameraEdgePanComponent.generated.h"

class APlayerController;

UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), DisplayName = "Camera Edge Pan")
class BASESIMULATIONCAMERACONTROL_API UCameraEdgePanComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCameraEdgePanComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Broadcast every Tick with a 2D pan axis when the cursor is inside an
	 * edge zone. The owning Pawn subscribes to this delegate to drive its
	 * Pan() method. Convention matches the Pawn Pan: X = right (+)/left (-),
	 * Y = forward (+)/backward (-) in world space.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Camera Control")
	FOnCameraPanInput OnEdgePanInput;

	/**
	 * Pixel distance from each viewport edge inside which the cursor triggers
	 * a pan. The component ramps the pan magnitude from 0 (at the zone
	 * boundary) to 1 (at the very edge) based on the cursor''s penetration
	 * into the zone.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Control", meta = (ClampMin = "0.0"))
	float EdgeThreshold = 10.0f;

	/**
	 * Multiplier on the broadcast axis value. The owning Pawn multiplies
	 * this by its own PanSpeed and DeltaTime, so 1.0 yields full pawn pan
	 * speed; 0.5 yields half.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Control", meta = (ClampMin = "0.0"))
	float EdgePanSpeed = 1.0f;

	/** Master toggle for horizontal (left/right) edge panning. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Control")
	bool bEnableEdgePanX = true;

	/** Master toggle for vertical (top/bottom) edge panning. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Control")
	bool bEnableEdgePanY = true;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> CachedController;

	float ComputeEdgeAxis(float Cursor, float ViewportSize) const;

	/** Returns the cached PlayerController, re-resolving from the owner pawn if the cache is stale. */
	APlayerController* ResolveController();
};