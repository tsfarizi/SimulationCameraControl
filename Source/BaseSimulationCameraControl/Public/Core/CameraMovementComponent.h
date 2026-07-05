#pragma once

#include "CoreMinimal.h"
#include "CameraInputComponent.h"
#include "CameraMovementComponent.generated.h"

UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), DisplayName = "Camera Movement")
class BASESIMULATIONCAMERACONTROL_API UCameraMovementComponent : public UCameraInputComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Camera Input")
	FOnCameraZoomInput OnZoomInput;

	UPROPERTY(BlueprintAssignable, Category = "Camera Input")
	FOnCameraOrbitInput OnOrbitInput;

	UPROPERTY(BlueprintAssignable, Category = "Camera Input")
	FOnCameraPanInput OnPanInput;

	UPROPERTY(BlueprintAssignable, Category = "Camera Input")
	FOnCameraModifierInput OnOrbitModifierInput;

	UPROPERTY(BlueprintAssignable, Category = "Camera Input")
	FOnCameraModifierInput OnPanModifierInput;

	virtual TArray<FCameraInputActionSpec> GetActionSpecs_Implementation() const override;
	virtual void HandleAction_Implementation(FName ActionName, const FInputActionValue& Value) override;

	UPROPERTY(Transient)
	bool bIsOrbitModifierDown = false;

	UPROPERTY(Transient)
	bool bIsPanModifierDown = false;
};
