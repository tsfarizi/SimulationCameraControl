#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SimulationCameraController.generated.h"

class UCameraInputBindings;
class UInputMappingContext;
class UEnhancedInputComponent;

UCLASS()
class BASESIMULATIONCAMERACONTROL_API ASimulationCameraController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintCallable, Category = "Camera Input")
	void SetupCameraInput();

	UFUNCTION(BlueprintCallable, Category = "Camera Input")
	void ClearCameraInput();

	UFUNCTION(BlueprintCallable, Category = "Camera Input")
	void BindActionsToEnhancedInput(UEnhancedInputComponent* EIC);

	/**
	 * Primary interaction handler. Raycasts under the cursor using the
	 * configured interaction trace channel. If the hit actor implements
	 * USelectableInterface, OnSelected is dispatched. Safe to call from
	 * Blueprint or from the IA_Click input action binding.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Interaction")
	void HandleInteractionClick();

	/** Collision channel used by HandleInteractionClick for the cursor raycast. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Interaction")
	TEnumAsByte<ECollisionChannel> InteractionTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input")
	TObjectPtr<UCameraInputBindings> InputBindingsOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input", meta = (ClampMin = "0"))
	int32 InputMappingPriority = 0;

private:
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ActiveContext;
};