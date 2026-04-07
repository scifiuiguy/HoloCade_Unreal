// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

using UnrealBuildTool;

public class HoloCadeExperiences : ModuleRules
{
	public HoloCadeExperiences(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
			}
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UMG",
				"HoloCadeCommon",  // Interfaces and shared types
				"HoloCadeCore",    // Implementation classes (InputAdapter, ServerCommandProtocol, etc.)
				"HoloCadeAI",      // Generic AI module (for base classes: UAIScriptManager, UAIImprovManager, UAIASRManager)
				"Sockets",        // For GoKart ECU UDP communication
				"Networking",     // For GoKart ECU UDP communication
				"EnhancedInput",  // For GoKart input handling
				"InputCore",      // For GoKart input handling
				"Json",           // For GoKart data serialization
				"JsonUtilities"   // For GoKart data serialization
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"HoloCadeAI",        // Generic AI module (LLM, ASR, TTS, Audio2Face, etc.)
				"VOIP",            // For IVOIPAudioVisitor interface (used by ASR manager)
				"LargeHaptics",    // Implementation details - not exposed in public API
				"EmbeddedSystems", // Implementation details - not exposed in public API
				"RF433MHz"         // Required for SuperheroFlightExperience RF433MHz receiver
			}
		);
	}
}




