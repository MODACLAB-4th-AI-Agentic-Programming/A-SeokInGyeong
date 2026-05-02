// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AI_NPC_Chat : ModuleRules
{
	public AI_NPC_Chat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"AI_NPC_Chat",
			"AI_NPC_Chat/Variant_Platforming",
			"AI_NPC_Chat/Variant_Platforming/Animation",
			"AI_NPC_Chat/Variant_Combat",
			"AI_NPC_Chat/Variant_Combat/AI",
			"AI_NPC_Chat/Variant_Combat/Animation",
			"AI_NPC_Chat/Variant_Combat/Gameplay",
			"AI_NPC_Chat/Variant_Combat/Interfaces",
			"AI_NPC_Chat/Variant_Combat/UI",
			"AI_NPC_Chat/Variant_SideScrolling",
			"AI_NPC_Chat/Variant_SideScrolling/AI",
			"AI_NPC_Chat/Variant_SideScrolling/Gameplay",
			"AI_NPC_Chat/Variant_SideScrolling/Interfaces",
			"AI_NPC_Chat/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
