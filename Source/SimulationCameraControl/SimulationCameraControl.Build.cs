// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SimulationCameraControl : ModuleRules
{
	public SimulationCameraControl(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"EnhancedInput",
				"GameplayTags",
				// ... add other public dependencies that you statically link with here ...
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"InputCore",
				// ... add private dependencies that you statically link with here ...
			}
			);

		// Editor-only dependencies for the UCameraInputBindingsFactory
		// (lets the editor create new UCameraInputBindings assets via the
		// right-click menu in the Content Browser). Guarded so runtime
		// packaged builds don't pull in UnrealEd.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AssetTools" });
		}


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
