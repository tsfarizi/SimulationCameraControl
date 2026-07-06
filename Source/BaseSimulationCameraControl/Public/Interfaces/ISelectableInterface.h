#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ISelectableInterface.generated.h"

UINTERFACE(BlueprintType, Blueprintable, MinimalAPI)
class USelectableInterface : public UInterface
{
	GENERATED_BODY()
};

class BASESIMULATIONCAMERACONTROL_API ISelectableInterface
{
	GENERATED_BODY()

public:
	/**
	 * Called by ASimulationCameraController when the user clicks on an actor
	 * that implements this interface. Default is a no-op; override in C++ or
	 * implement the event in Blueprint to react to the selection.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Selection")
	void OnSelected();
	virtual void OnSelected_Implementation() {}

	/**
	 * Called when the cursor enters an actor that implements this interface.
	 * The Controller tracks the currently hovered actor and invokes OnHovered
	 * on entry and OnDeselected on exit. Default is a no-op.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Selection")
	void OnHovered();
	virtual void OnHovered_Implementation() {}

	/**
	 * Called when the cursor leaves an actor that was previously hovered.
	 * Default is a no-op.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Selection")
	void OnDeselected();
	virtual void OnDeselected_Implementation() {}
};