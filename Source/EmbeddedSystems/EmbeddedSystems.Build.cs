// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

using UnrealBuildTool;

public class EmbeddedSystems : ModuleRules
{
	public EmbeddedSystems(ReadOnlyTargetRules Target) : base(Target)
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
				"Sockets",
				"Networking",
				"Json",
				"JsonUtilities",
				"HoloCadeCommon",  // Required for IHoloCadeEmbeddedDeviceInterface
				"HoloCadeCore",    // Required for UHoloCadeUDPTransport
				"RF433MHz"       // Required for URF433MHzReceiver
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore"
			}
		);
	}
}


