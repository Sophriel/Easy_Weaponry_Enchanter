// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EWE : ModuleRules
{
    public EWE(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
        PublicDependencyModuleNames.AddRange(new string[] {
            "EasyWeaponryEnchanter",
            "GameplayAbilities",
            "GameplayTasks"
        });
    }
}
