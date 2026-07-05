#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BaseSimulationCameraControl.generated.h"

class UCameraComponent;
class USpringArmComponent;
class USceneComponent;
class UEnhancedInputComponent;

UCLASS(Blueprintable)
class BASESIMULATIONCAMERACONTROL_API ABaseSimulationCameraControl : public APawn
{
	GENERATED_BODY()

public:
	ABaseSimulationCameraControl();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Camera|Control")
	void SetInputEnabled(bool bInEnabled);

	UFUNCTION(BlueprintCallable, Category = "Camera|Control")
	void Zoom(float AxisValue);

	UFUNCTION(BlueprintCallable, Category = "Camera|Control")
	void Orbit(FVector2D AxisValue);

	UFUNCTION(BlueprintCallable, Category = "Camera|Control")
	void Pan(FVector2D AxisValue);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "10.0"))
	float MinArmLength = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "100.0"))
	float MaxArmLength = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "1.0"))
	float ZoomStep = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	bool bInvertZoom = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMin = "0.0"))
	float OrbitYawSpeed = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMin = "0.0"))
	float OrbitPitchSpeed = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMax = "0.0"))
	float MinPitch = -75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMax = "0.0"))
	float MaxPitch = -30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pan", meta = (ClampMin = "0.0"))
	float PanSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus", meta = (ClampMin = "100.0"))
	float RayLength = 50000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus")
	float GroundZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus", meta = (ClampMin = "0.0"))
	float JumpThreshold = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input")
	bool bInputEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Debug")
	bool bDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing", meta = (ClampMin = "0.1"))
	float ZoomInterpSpeed = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing", meta = (ClampMin = "0.1"))
	float OrbitInterpSpeed = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing", meta = (ClampMin = "0.1"))
	float PanInterpSpeed = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing")
	bool bSmoothZoom = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing")
	bool bSmoothOrbit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing")
	bool bSmoothPan = true;

private:
	bool GetCursorWorldPoint(FVector& OutPoint);
	FVector GetStableFocusPoint();
	void ApplyZoom(float DesiredArmLength, const FVector& FocusPoint);

	FVector LastValidHitLocation = FVector::ZeroVector;
	bool bHasCachedFocus = false;

	float TargetArmLength = 400.0f;
	FRotator TargetRelativeRotation = FRotator(-60.0f, 0.0f, 0.0f);
	FVector TargetActorLocation = FVector::ZeroVector;
	bool bTargetsInitialized = false;
};