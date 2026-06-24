// Copyright Teuku. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "CameraInputBindings.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Per-key binding spec. For 2D/3D value-type actions, the Axis field routes
 * the key's contribution to a specific axis component (X, Y, XNeg, YNeg).
 * The factory translates this into the appropriate FInputModifierSwizzleAxis.
 */
USTRUCT(BlueprintType)
struct SIMULATIONCAMERACONTROL_API FCameraInputKeySpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FKey Key;

	/** Which axis component this key contributes to. Only meaningful for 2D/3D actions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FName Axis = FName(TEXT("X"));

	/** If true, the value is negated (wraps FInputModifierNegate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bNegate = false;
};

/**
 * Per-action spec. The name must match the convention used by the pawn's
 * auto-binding (IA_Zoom, IA_Orbit, IA_Orbit_Modifier, IA_Pan, IA_Pan_Modifier).
 * The pawn calls Handle<ActionName>Action on Triggered events.
 */
USTRUCT(BlueprintType)
struct SIMULATIONCAMERACONTROL_API FCameraInputActionSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FName ActionName = NAME_None;

	/** EInputActionValueType: Boolean (Bool), Axis1D (float), Axis2D (FVector2D), Axis3D (FVector). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	EInputActionValueType ValueType = EInputActionValueType::Axis1D;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TArray<FCameraInputKeySpec> DefaultKeys;
};

/**
 * UCameraInputBindings
 * Designer-editable DataAsset listing all the camera's input actions and
 * their default key bindings. At runtime the pawn (or the editor factory)
 * converts this into a UInputMappingContext with one UInputAction per spec.
 *
 * The pawn's SetupPlayerInputComponent() auto-binds actions by ActionName
 * convention, so a new action only needs:
 *   1. A matching handler in the pawn (Handle<ActionName>Action)
 *   2. An entry in this DataAsset (or the C++ defaults if no override is set)
 */
UCLASS(BlueprintType)
class SIMULATIONCAMERACONTROL_API UCameraInputBindings : public UDataAsset
{
	GENERATED_BODY()

public:
	/** All actions this camera recognises, in the order the pawn auto-binds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input")
	TArray<FCameraInputActionSpec> Actions;

	/** Priority applied when registering the IMC with the local player. Higher wins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input", meta = (ClampMin = "0"))
	int32 MappingPriority = 0;

	/**
	 * Build a UInputMappingContext in memory from the spec list. Each spec
	 * becomes one UInputAction (parented to the context) with its DefaultKeys
	 * mapped via FInputModifierSwizzleAxis/Negate as appropriate.
	 *
	 * The returned IMC and its UInputActions are transient (RF_Transient) and
	 * live as long as Outer is alive. Pass `this` (the pawn) as Outer so the
	 * GC keeps the context alive with the pawn.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Input")
	UInputMappingContext* BuildContext(UObject* Outer) const;

	/**
	 * Replace the spec list with the built-in defaults (5 actions: Zoom, Orbit,
	 * Orbit_Modifier, Pan, Pan_Modifier). Useful as a one-click reset from the
	 * Details panel, and as the entry point for the editor "Generate" command.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Camera Input")
	void PopulateDefaultActions();
};
