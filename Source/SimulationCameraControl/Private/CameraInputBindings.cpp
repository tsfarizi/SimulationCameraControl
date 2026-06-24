// Copyright Teuku. All Rights Reserved.

#include "CameraInputBindings.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"

namespace
{
	/**
	 * Build a FInputModifierSwizzleAxis from the (Axis, bNegate) tuple.
	 * Returns nullptr if the modifier would be a no-op (plain X, no negate).
	 *
	 * The UE swizzle enum describes (src.X, src.Y, src.Z) -> (dest.X, dest.Y, dest.Z)
	 * for a 2D/3D value. The first character of the enum value is the destination
	 * X source, second is dest Y, third is dest Z. We only care about 2D, so the
	 * third slot is always Z (the dest Z for a 2D output is just dropped).
	 */
	UInputModifierSwizzleAxis* MakeSwizzleModifier(FName Axis, bool bNegate)
	{
		EInputAxisSwizzle Swizzle;
		bool bNeeded = bNegate; // Negate alone is a no-op without a swizzle unless the user wants pure negation

		if (Axis == FName(TEXT("X")))        { Swizzle = EInputAxisSwizzle::YXZ; bNeeded = bNegate; }
		else if (Axis == FName(TEXT("XNeg"))) { Swizzle = EInputAxisSwizzle::YXZ; bNeeded = true; }
		else if (Axis == FName(TEXT("Y")))    { Swizzle = EInputAxisSwizzle::ZXY; bNeeded = true; }
		else if (Axis == FName(TEXT("YNeg"))) { Swizzle = EInputAxisSwizzle::ZXY; bNeeded = true; }
		else if (Axis == FName(TEXT("Z")))    { Swizzle = EInputAxisSwizzle::YZX; bNeeded = true; }
		else if (Axis == FName(TEXT("ZNeg"))) { Swizzle = EInputAxisSwizzle::YZX; bNeeded = true; }
		else                                  { Swizzle = EInputAxisSwizzle::YXZ; bNeeded = bNegate; }

		if (!bNeeded)
		{
			return nullptr;
		}

		UInputModifierSwizzleAxis* Modifier = NewObject<UInputModifierSwizzleAxis>();
		Modifier->Order = Swizzle;
		return Modifier;
	}
}

UInputMappingContext* UCameraInputBindings::BuildContext(UObject* Outer) const
{
	UObject* EffectiveOuter = Outer ? Outer : const_cast<UCameraInputBindings*>(this);
	UInputMappingContext* Context = NewObject<UInputMappingContext>(
		EffectiveOuter, UInputMappingContext::StaticClass(), NAME_None, RF_Transient);

	if (!Context)
	{
		return nullptr;
	}

	// De-duplicate: if two specs share an ActionName, the second is ignored.
	TSet<FName> SeenNames;

	for (const FCameraInputActionSpec& Spec : Actions)
	{
		if (Spec.ActionName.IsNone() || SeenNames.Contains(Spec.ActionName))
		{
			continue;
		}
		SeenNames.Add(Spec.ActionName);

		UInputAction* Action = NewObject<UInputAction>(
			Context, UInputAction::StaticClass(), Spec.ActionName, RF_Transient);
		Action->ValueType = Spec.ValueType;

		for (const FCameraInputKeySpec& KeySpec : Spec.DefaultKeys)
		{
			if (!KeySpec.Key.IsValid())
			{
				continue;
			}

			FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, KeySpec.Key);

			// For 2D/3D value types, route the key's contribution to a specific
			// axis component via FInputModifierSwizzleAxis.
			if (Spec.ValueType != EInputActionValueType::Boolean &&
				Spec.ValueType != EInputActionValueType::Axis1D)
			{
				if (UInputModifierSwizzleAxis* Swizzle = MakeSwizzleModifier(KeySpec.Axis, KeySpec.bNegate))
				{
					Mapping.Modifiers.Add(Swizzle);
				}
			}
			else if (KeySpec.bNegate)
			{
				// 1D or Boolean with bNegate=true: just negate.
				UInputModifierNegate* Negate = NewObject<UInputModifierNegate>();
				Mapping.Modifiers.Add(Negate);
			}
		}
	}

	return Context;
}

void UCameraInputBindings::PopulateDefaultActions()
{
	// Reset to a clean slate, then fill in the five actions the pawn's
	// auto-bind loop expects by name convention.
	Actions.Reset();

	// IA_Zoom: mouse wheel axis (1D, signed).
	Actions.Add({
		FName(TEXT("IA_Zoom")),
		EInputActionValueType::Axis1D,
		{ { EKeys::MouseWheelAxis, FName(TEXT("X")), false } }
	});

	// IA_Orbit: mouse XY (2D). Right mouse modifier (set on a separate IA below)
	// gates the orbit so moving the mouse alone doesn't spin the camera.
	Actions.Add({
		FName(TEXT("IA_Orbit")),
		EInputActionValueType::Axis2D,
		{ { EKeys::Mouse2D, FName(TEXT("Y")), false } }
	});

	// IA_Orbit_Modifier: right mouse button (bool).
	Actions.Add({
		FName(TEXT("IA_Orbit_Modifier")),
		EInputActionValueType::Boolean,
		{ { EKeys::RightMouseButton, FName(TEXT("X")), false } }
	});

	// IA_Pan: WASD as 2D axis + middle-mouse 2D.
	// W -> Y+, S -> Y-, D -> X+, A -> X- (each routed via SwizzleAxis + Negate as needed).
	// Pawn's HandlePanAction also accepts the modifier-held-down case for mouse movement.
	Actions.Add({
		FName(TEXT("IA_Pan")),
		EInputActionValueType::Axis2D,
		{
			{ EKeys::W, FName(TEXT("Y")),  false },
			{ EKeys::S, FName(TEXT("Y")),  true  },
			{ EKeys::D, FName(TEXT("X")),  false },
			{ EKeys::A, FName(TEXT("X")),  true  },
			{ EKeys::Mouse2D, FName(TEXT("Y")), false },
		}
	});

	// IA_Pan_Modifier: middle mouse button (bool). Gates middle-mouse drag panning.
	Actions.Add({
		FName(TEXT("IA_Pan_Modifier")),
		EInputActionValueType::Boolean,
		{ { EKeys::MiddleMouseButton, FName(TEXT("X")), false } }
	});
}
