// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

using UnrealBuildTool;

public class VOIP : ModuleRules
{
	public VOIP(ReadOnlyTargetRules Target) : base(Target)
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
				"HoloCadeCore",
				"Sockets",
				"Networking",
				"AudioMixer",
				"AudioPlatformConfiguration"
				// Note: SteamAudio and MumbleLink plugins will be added as dependencies
				// when submodules are set up. Add them to PublicDependencyModuleNames:
				// "SteamAudio",
				// "MumbleLink"
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"AudioMixer",
				"SignalProcessing"
			}
		);

		// Dynamically loaded modules (plugins)
		// These will be loaded at runtime if available
		PublicDelayLoadDLLs.AddRange(
			new string[]
			{
				// Steam Audio and Mumble libraries will be loaded here if needed
			}
		);
	}
}



