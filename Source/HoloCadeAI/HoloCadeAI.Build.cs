// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

using UnrealBuildTool;

public class HoloCadeAI : ModuleRules
{
	public HoloCadeAI(ReadOnlyTargetRules Target) : base(Target)
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
				"VOIP",  // For IVOIPAudioVisitor interface (visitor pattern for decoupled integration)
				"HeadMountedDisplay",
				"AnimGraphRuntime",
				"AnimationCore",
				"HTTP",  // For HTTP client requests to NVIDIA ACE endpoints
				"Json",  // For JSON serialization/deserialization
				"JsonUtilities",  // For JSON utilities
				"WebSockets"  // For WebSocket client to receive streaming facial animation data
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"ImageWrapper"  // For decoding base64 image data from NVIDIA ACE
			}
		);

		// TurboLink gRPC plugin (optional - for NVIDIA Riva ASR/TTS)
		// TurboLink must be installed as a git submodule in Plugins/TurboLink/
		// Run: .\Source\HoloCadeAI\Common\SetupTurboLink.ps1 to set it up
		string TurboLinkPluginPath = System.IO.Path.Combine(ModuleDirectory, "../../../TurboLink");
		if (System.IO.Directory.Exists(TurboLinkPluginPath))
		{
			// Define WITH_TURBOLINK preprocessor macro
			PublicDefinitions.Add("WITH_TURBOLINK=1");
			
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"TurboLinkGrpc"  // TurboLink gRPC core module (main module)
				}
			);
			
			// Add TurboLink include paths
			string TurboLinkSourcePath = System.IO.Path.Combine(TurboLinkPluginPath, "Source");
			if (System.IO.Directory.Exists(TurboLinkSourcePath))
			{
				PublicIncludePaths.Add(TurboLinkSourcePath);
			}
		}
		else
		{
			// TurboLink not found - gRPC will use NOOP implementation
			// This is OK for development, but ASR/TTS will not work until TurboLink is installed
			PublicDefinitions.Add("WITH_TURBOLINK=0");
			System.Console.WriteLine("WARNING: TurboLink plugin not found at: " + TurboLinkPluginPath);
			System.Console.WriteLine("         gRPC functionality will be NOOP. Install TurboLink for full functionality.");
			System.Console.WriteLine("         Run: .\\Source\\AI\\Common\\SetupTurboLink.ps1");
		}
	}
}




