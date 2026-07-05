#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "CameraInputBindings.h"
#include "CameraInputComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraZoomInput, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraOrbitInput, FVector2D, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraPanInput, FVector2D, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraModifierInput, bool, bValue);

UCLASS(Abstract, Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class BASESIMULATIONCAMERACONTROL_API UCameraInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Input")
	TArray<FCameraInputActionSpec> GetActionSpecs() const;
	virtual TArray<FCameraInputActionSpec> GetActionSpecs_Implementation() const { return {}; }

	UFUNCTION(BlueprintNativeEvent, Category = "Camera Input")
	void HandleAction(FName ActionName, const FInputActionValue& Value);
	virtual void HandleAction_Implementation(FName ActionName, const FInputActionValue& Value) {}

	virtual bool HandlesAction(FName ActionName) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bInputEnabled = true;
};
