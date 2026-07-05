// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BaseSimulationCameraControl : ModuleRules
{
	public BaseSimulationCameraControl(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				ModuleDirectory + "/Public/Core",
				ModuleDirectory + "/Public/Input",
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				ModuleDirectory + "/Private/Core",
				ModuleDirectory + "/Private/Input",
				ModuleDirectory + "/Private/Module",
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
