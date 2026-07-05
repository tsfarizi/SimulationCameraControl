#include "CameraInputComponent.h"

bool UCameraInputComponent::HandlesAction(FName ActionName) const
{
	if (ActionName.IsNone())
	{
		return false;
	}
	for (const FCameraInputActionSpec& Spec : GetActionSpecs())
	{
		if (Spec.ActionName == ActionName)
		{
			return true;
		}
	}
	return false;
}
