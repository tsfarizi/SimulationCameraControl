// Copyright Teuku. All Rights Reserved.

#include "CameraInputBindingsFactory.h"

#if WITH_EDITOR

#include "CameraInputBindings.h"

UCameraInputBindingsFactory::UCameraInputBindingsFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;
	SupportedClass = UCameraInputBindings::StaticClass();
}

UObject* UCameraInputBindingsFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* /*Context*/, FFeedbackContext* /*Warn*/)
{
	check(InClass && InClass->IsChildOf(UCameraInputBindings::StaticClass()));

	UCameraInputBindings* NewAsset = NewObject<UCameraInputBindings>(InParent, InClass, InName, Flags);
	if (NewAsset)
	{
		// Pre-populate with the 5 standard actions so the designer can
		// drag-and-drop the new asset onto a pawn without further setup.
		NewAsset->PopulateDefaultActions();
	}
	return NewAsset;
}

#endif // WITH_EDITOR
