// Copyright Teuku. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "Factories/Factory.h"
#endif

#include "CameraInputBindingsFactory.generated.h"

#if WITH_EDITOR

/**
 * UCameraInputBindingsFactory
 * Editor-only factory: right-click in the Content Browser -> Misc -> Data Asset
 * -> pick UCameraInputBindings -> pre-populate with the built-in defaults
 * (5 actions: IA_Zoom, IA_Orbit, IA_Orbit_Modifier, IA_Pan, IA_Pan_Modifier).
 *
 * Without this factory the user has to (a) create an empty DataAsset and (b)
 * call PopulateDefaultActions from the Details panel. This entry point wraps
 * the same flow in a single menu click.
 */
UCLASS()
class UCameraInputBindingsFactory : public UFactory
{
	GENERATED_BODY()

public:
	UCameraInputBindingsFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

#endif // WITH_EDITOR

