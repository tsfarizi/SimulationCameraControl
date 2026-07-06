#pragma once

#include "CoreMinimal.h"
#include "CameraInputComponent.h"
#include "CameraDragPanComponent.generated.h"

UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), DisplayName = "Camera Drag Pan")
class BASESIMULATIONCAMERACONTROL_API UCameraDragPanComponent : public UCameraInputComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Camera Input")
	FOnCameraPanInput OnDragPanInput;

	UPROPERTY(BlueprintAssignable, Category = "Camera Input")
	FOnCameraModifierInput OnDragModifierInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.0"))
	float PanSpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FVector2D InvertAxis = FVector2D(-1.0f, -1.0f);

	virtual TArray<FCameraInputActionSpec> GetActionSpecs_Implementation() const override;
	virtual void HandleAction_Implementation(FName ActionName, const FInputActionValue& Value) override;

private:
	UPROPERTY(Transient)
	bool bIsDragActive = false;
};