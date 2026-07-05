// Copyright Teuku. All Rights Reserved.

#include "CameraInputDefaults.h"
#include "InputAction.h"
#include "InputMappingContext.h"

namespace CameraInputDefaults
{
	TArray<FCameraInputActionSpec> GetDefaultActionSpecs()
	{
		TArray<FCameraInputActionSpec> Specs;

		auto Make = [&Specs](FName Name, EInputActionValueType ValueType, std::initializer_list<FCameraInputKeySpec> Keys)
		{
			FCameraInputActionSpec Spec;
			Spec.ActionName = Name;
			Spec.ValueType = ValueType;
			for (const FCameraInputKeySpec& K : Keys)
			{
				Spec.DefaultKeys.Add(K);
			}
			Specs.Add(Spec);
		};

		Make(FName(TEXT("IA_Zoom")), EInputActionValueType::Axis1D, {
			FCameraInputKeySpec{ EKeys::MouseWheelAxis, FName(TEXT("X")), false }
		});

		Make(FName(TEXT("IA_Orbit")), EInputActionValueType::Axis2D, {
			FCameraInputKeySpec{ EKeys::Mouse2D, FName(TEXT("X")), false }
		});

		Make(FName(TEXT("IA_Orbit_Modifier")), EInputActionValueType::Boolean, {
			FCameraInputKeySpec{ EKeys::RightMouseButton, FName(TEXT("X")), false }
		});

		Make(FName(TEXT("IA_Pan")), EInputActionValueType::Axis2D, {
			FCameraInputKeySpec{ EKeys::W, FName(TEXT("Y")), false },
			FCameraInputKeySpec{ EKeys::S, FName(TEXT("Y")), true  },
			FCameraInputKeySpec{ EKeys::D, FName(TEXT("X")), false },
			FCameraInputKeySpec{ EKeys::A, FName(TEXT("X")), true  },
			FCameraInputKeySpec{ EKeys::Mouse2D, FName(TEXT("X")), false },
		});

		Make(FName(TEXT("IA_Pan_Modifier")), EInputActionValueType::Boolean, {
			FCameraInputKeySpec{ EKeys::LeftMouseButton, FName(TEXT("X")), false }
		});

		return Specs;
	}

	UInputMappingContext* MakeDefaultContext(UObject* Outer)
	{
		UObject* EffectiveOuter = Outer ? Outer : GetTransientPackage();

		// Build a transient UCameraInputBindings to reuse BuildContext.
		UCameraInputBindings* TempBindings = NewObject<UCameraInputBindings>(
			GetTransientPackage(), UCameraInputBindings::StaticClass(), TEXT("DefaultCameraInputBindings"), RF_Transient);
		if (!TempBindings)
		{
			return nullptr;
		}
		TempBindings->Actions = GetDefaultActionSpecs();
		return TempBindings->BuildContext(EffectiveOuter);
	}
}
