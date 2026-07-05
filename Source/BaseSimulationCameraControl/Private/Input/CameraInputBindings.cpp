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
	 * Returns nullptr if no swizzle is needed (i.e., the key already
	 * contributes to the correct default axis — X).
	 *
	 * NEGATION NOTE: This function ONLY returns a swizzle modifier; it
	 * does NOT apply negation. Negation for 2D/3D actions is handled by a
	 * separate UInputModifierNegate added in BuildContext when bNegate is
	 * true. This separation is necessary because FInputModifierSwizzleAxis
	 * only reorders axes and cannot negate values.
	 *
	 * The UE swizzle enum describes (src.X, src.Y, src.Z) -> (dest.X, dest.Y, dest.Z)
	 * for a 2D/3D value. The first character of the enum value is the destination
	 * X source, second is dest Y, third is dest Z.
	 *
	 * Axis routing convention:
	 *   "X"   → passthrough (no swizzle) — key's value stays in dest.X
	 *   "Y"   → YXZ — routes input.X to output.Y (dest.X = src.Y, dest.Y = src.X)
	 *   "XNeg"/"YNeg"/etc. → same swizzle as positive counterpart;
	 *                         negation applied separately via UInputModifierNegate
	 */
	UInputModifierSwizzleAxis* MakeSwizzleModifier(FName Axis)
	{
		EInputAxisSwizzle Swizzle;

		if (Axis == FName(TEXT("X")) || Axis == FName(TEXT("XNeg")))
		{
			// X is the default axis; no swizzle needed.
			return nullptr;
		}
		else if (Axis == FName(TEXT("Y")) || Axis == FName(TEXT("YNeg")))
		{
			// YXZ: output.X = input.Y, output.Y = input.X
			// Routes a 1D key's input.X (value) into the action's Y axis.
			Swizzle = EInputAxisSwizzle::YXZ;
		}
		else if (Axis == FName(TEXT("Z")) || Axis == FName(TEXT("ZNeg")))
		{
			// YZX: output.Y = input.Z, output.Z = input.X, output.X = input.Y
			// Routes input into the Z axis.
			Swizzle = EInputAxisSwizzle::YZX;
		}
		else
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
				// Swizzle reorders axes (e.g., routes a 1D key to the Y component).
				// Returns nullptr for X-axis keys (passthrough).
				if (UInputModifierSwizzleAxis* Swizzle = MakeSwizzleModifier(KeySpec.Axis))
				{
					Mapping.Modifiers.Add(Swizzle);
				}

				// Negation is applied as a separate modifier so it works
				// independently of the swizzle. This fixes the bug where
				// bNegate=true on 2D/3D actions was silently ignored.
				if (KeySpec.bNegate)
				{
					UInputModifierNegate* Negate = NewObject<UInputModifierNegate>();
					Mapping.Modifiers.Add(Negate);
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
		{ { EKeys::Mouse2D, FName(TEXT("X")), false } }
	});

	// IA_Orbit_Modifier: right mouse button (bool).
	Actions.Add({
		FName(TEXT("IA_Orbit_Modifier")),
		EInputActionValueType::Boolean,
		{ { EKeys::RightMouseButton, FName(TEXT("X")), false } }
	});

	// IA_Pan: WASD as 2D axis + middle-mouse 2D.
	// W -> Y+, S -> Y-, D -> X+, A -> X- (each routed via SwizzleAxis + Negate as needed).
	// Mouse2D uses Axis "X" so raw (DeltaX, DeltaY) passes through without a swizzle,
	// allowing full 2D mouse-drag panning (both axes preserved).
	// Pawn's HandlePanAction also accepts the modifier-held-down case for mouse movement.
	Actions.Add({
		FName(TEXT("IA_Pan")),
		EInputActionValueType::Axis2D,
		{
			{ EKeys::W, FName(TEXT("Y")),  false },
			{ EKeys::S, FName(TEXT("Y")),  true  },
			{ EKeys::D, FName(TEXT("X")),  false },
			{ EKeys::A, FName(TEXT("X")),  true  },
			{ EKeys::Mouse2D, FName(TEXT("X")), false },
		}
	});

	// IA_Pan_Modifier: left mouse button (bool). Gates left-click drag panning.
	Actions.Add({
		FName(TEXT("IA_Pan_Modifier")),
		EInputActionValueType::Boolean,
		{ { EKeys::LeftMouseButton, FName(TEXT("X")), false } }
	});
}
