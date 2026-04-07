// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "HoloCadeServerManagerWidget.h"
#include "Networking/HoloCadeServerBeacon.h"
#include "Networking/HoloCadeServerCommandProtocol.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"

void UHoloCadeServerManagerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Initialize default configuration
	ServerConfig.ExperienceType = TEXT("AIFacemask");
	ServerConfig.ServerName = TEXT("HoloCade Server");
	ServerConfig.MaxPlayers = 4;
	ServerConfig.Port = 7777;
	ServerConfig.MapName = TEXT("/Game/Maps/HoloCadeMap");

	// Initialize network beacon for real-time status updates and server discovery
	ServerBeacon = NewObject<UHoloCadeServerBeacon>(this, TEXT("ServerStatusBeacon"));
	if (ServerBeacon)
	{
		// Bind to status updates
		ServerBeacon->OnServerDiscovered.AddDynamic(this, &UHoloCadeServerManagerWidget::OnServerStatusReceived);
		
		// Also bind to discovery for auto-connect (Remote mode)
		ServerBeacon->OnServerDiscovered.AddDynamic(this, &UHoloCadeServerManagerWidget::OnServerDiscoveredForConnection);
		
		ServerBeacon->StartClientDiscovery();
		AddLogMessage(TEXT("Server status beacon initialized (listening on port 7778)"));
	}

	// Initialize command protocol for remote server control
	CommandProtocol = NewObject<UHoloCadeServerCommandProtocol>(this, TEXT("CommandProtocol"));
	if (CommandProtocol)
	{
		// Configure authentication settings (only applies in Remote mode)
		CommandProtocol->bEnableAuthentication = bEnableAuthentication;
		CommandProtocol->SharedSecret = SharedSecret;
		CommandProtocol->CommandPort = RemoteCommandPort;
		
		if (bEnableAuthentication)
		{
			AddLogMessage(TEXT("Command protocol initialized with authentication enabled (port 7779)"));
		}
		else
		{
			AddLogMessage(TEXT("Command protocol initialized (port 7779, authentication disabled)"));
		}
	}

	// Set default connection mode based on whether we're in editor or standalone
	ConnectionMode = EHoloCadeConnectionMode::Local;

	AddLogMessage(FString::Printf(TEXT("Server Manager initialized (Mode: %s)"), 
		ConnectionMode == EHoloCadeConnectionMode::Local ? TEXT("Local") : TEXT("Remote")));
}

void UHoloCadeServerManagerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Tick the server beacon for network updates
	if (ServerBeacon && ServerBeacon->IsActive())
	{
		ServerBeacon->Tick(InDeltaTime);
	}

	// Tick command protocol (for remote mode)
	if (CommandProtocol && ConnectionMode == EHoloCadeConnectionMode::Remote)
	{
		CommandProtocol->TickClient(InDeltaTime);
	}

	// Poll server status
	StatusPollTimer += InDeltaTime;
	if (StatusPollTimer >= StatusPollInterval)
	{
		StatusPollTimer = 0.0f;
		PollServerStatus();
	}

	// Update uptime if server is running
	if (ServerStatus.bIsRunning)
	{
		ServerStatus.Uptime += InDeltaTime;
	}
}

bool UHoloCadeServerManagerWidget::StartServer()
{
	if (ServerStatus.bIsRunning)
	{
		AddLogMessage(TEXT("ERROR: Server is already running"));
		return false;
	}

	// Handle different connection modes
	if (ConnectionMode == EHoloCadeConnectionMode::Remote)
	{
		// Remote mode: Send command to remote server
		if (!CommandProtocol || !CommandProtocol->IsActive())
		{
			AddLogMessage(TEXT("ERROR: Not connected to remote server. Connect first."));
			return false;
		}

		// Build command parameter JSON
		FString CommandParam = FString::Printf(TEXT("{\"ExperienceType\":\"%s\",\"MaxPlayers\":%d,\"Port\":%d,\"MapName\":\"%s\"}"),
			*ServerConfig.ExperienceType, ServerConfig.MaxPlayers, ServerConfig.Port, *ServerConfig.MapName);

		FHoloCadeServerResponseMessage Response = CommandProtocol->SendCommand(EHoloCadeServerCommand::StartServer, CommandParam);
		
		if (Response.bSuccess)
		{
			ServerStatus.bIsRunning = true;
			ServerStatus.Uptime = 0.0f;
			ServerStatus.ExperienceState = TEXT("Starting...");
			ExpectedServerIP = RemoteServerIP;
			ExpectedServerPort = RemoteServerPort;
			AddLogMessage(FString::Printf(TEXT("Remote server start command sent: %s"), *Response.Message));
			return true;
		}
		else
		{
			AddLogMessage(FString::Printf(TEXT("ERROR: Failed to send start command: %s"), *Response.Message));
			return false;
		}
	}
	else
	{
		// Local mode: Launch server process
		FString ServerPath = GetServerExecutablePath();
		if (!FPaths::FileExists(ServerPath))
		{
			AddLogMessage(FString::Printf(TEXT("ERROR: Server executable not found at %s"), *ServerPath));
			AddLogMessage(TEXT("Please build the dedicated server target first."));
			return false;
		}

		// Build command line
		FString CommandLine = BuildServerCommandLine();

		AddLogMessage(FString::Printf(TEXT("Starting server: %s %s"), *ServerPath, *CommandLine));

		// Launch server process
		uint32 ProcessID = 0;
		ServerProcessHandle = FPlatformProcess::CreateProc(
			*ServerPath,
			*CommandLine,
			true,  // bLaunchDetached
			false, // bLaunchHidden
			false, // bLaunchReallyHidden
			&ProcessID,
			0,     // PriorityModifier
			nullptr, // OptionalWorkingDirectory
			nullptr, // PipeWriteChild
			nullptr  // PipeReadChild
		);
		ServerStatus.ProcessID = static_cast<int32>(ProcessID);

		if (ServerProcessHandle.IsValid())
		{
			ServerStatus.bIsRunning = true;
			ServerStatus.Uptime = 0.0f;
			ServerStatus.ExperienceState = TEXT("Starting...");
			
			// Save expected server info for beacon matching
			ExpectedServerIP = TEXT("127.0.0.1"); // Localhost for local server
			ExpectedServerPort = ServerConfig.Port;
			
			AddLogMessage(FString::Printf(TEXT("Server started successfully (PID: %d)"), ServerStatus.ProcessID));
			AddLogMessage(TEXT("Listening for server status broadcasts..."));
			return true;
		}
		else
		{
			AddLogMessage(TEXT("ERROR: Failed to start server process"));
			return false;
		}
	}
}

bool UHoloCadeServerManagerWidget::StopServer()
{
	if (!ServerStatus.bIsRunning)
	{
		AddLogMessage(TEXT("ERROR: No server is currently running"));
		return false;
	}

	// Handle different connection modes
	if (ConnectionMode == EHoloCadeConnectionMode::Remote)
	{
		// Remote mode: Send stop command
		if (!CommandProtocol || !CommandProtocol->IsActive())
		{
			AddLogMessage(TEXT("ERROR: Not connected to remote server"));
			ServerStatus.bIsRunning = false;
			return false;
		}

		FHoloCadeServerResponseMessage Response = CommandProtocol->SendCommand(EHoloCadeServerCommand::StopServer);
		
		if (Response.bSuccess)
		{
			ServerStatus.bIsRunning = false;
			ServerStatus.CurrentPlayers = 0;
			ServerStatus.ExperienceState = TEXT("Stopped");
			ServerStatus.ProcessID = 0;
			AddLogMessage(FString::Printf(TEXT("Remote server stop command sent: %s"), *Response.Message));
			return true;
		}
		else
		{
			AddLogMessage(FString::Printf(TEXT("ERROR: Failed to send stop command: %s"), *Response.Message));
			return false;
		}
	}
	else
	{
		// Local mode: Terminate process
		if (!ServerProcessHandle.IsValid())
		{
			AddLogMessage(TEXT("ERROR: Invalid server process handle"));
			ServerStatus.bIsRunning = false;
			return false;
		}

		AddLogMessage(TEXT("Stopping server..."));

		// Terminate the server process
		FPlatformProcess::TerminateProc(ServerProcessHandle);
		FPlatformProcess::CloseProc(ServerProcessHandle);

		ServerStatus.bIsRunning = false;
		ServerStatus.CurrentPlayers = 0;
		ServerStatus.ExperienceState = TEXT("Stopped");
		ServerStatus.ProcessID = 0;

		AddLogMessage(TEXT("Server stopped"));
		return true;
	}
}

void UHoloCadeServerManagerWidget::UpdateServerStatus()
{
	// Check if process is still running
	if (ServerProcessHandle.IsValid())
	{
		if (!FPlatformProcess::IsProcRunning(ServerProcessHandle))
		{
			AddLogMessage(TEXT("WARNING: Server process terminated unexpectedly"));
			ServerStatus.bIsRunning = false;
			ServerStatus.CurrentPlayers = 0;
			ServerStatus.ExperienceState = TEXT("Crashed");
			FPlatformProcess::CloseProc(ServerProcessHandle);
		}
	}

	// NOOP: TODO - Query actual server status via network beacon or log file parsing
	// For now, this is just a stub
}

void UHoloCadeServerManagerWidget::UpdateOmniverseStatus()
{
	// NOOP: TODO - Implement Omniverse Audio2Face connection check
	// This would connect to Omniverse Nucleus or Audio2Face API
	// For now, this is just a stub
	
	OmniverseStatus.bIsConnected = false;
	OmniverseStatus.StreamStatus = TEXT("Not Configured");
	OmniverseStatus.ActiveFaceStreams = 0;
}

TArray<FString> UHoloCadeServerManagerWidget::GetAvailableExperienceTypes() const
{
	TArray<FString> ExperienceTypes;
	ExperienceTypes.Add(TEXT("AIFacemask"));
	ExperienceTypes.Add(TEXT("MovingPlatform"));
	ExperienceTypes.Add(TEXT("Gunship"));
	ExperienceTypes.Add(TEXT("CarSim"));
	ExperienceTypes.Add(TEXT("FlightSim"));
	return ExperienceTypes;
}

void UHoloCadeServerManagerWidget::AddLogMessage(const FString& Message)
{
	// Log to Unreal console
	UE_LOG(LogTemp, Log, TEXT("[ServerManager] %s"), *Message);

	// NOOP: TODO - Add to UI log widget
	// This would be implemented in Blueprint by binding to this function
	// and appending to a TextBlock or ScrollBox
}

void UHoloCadeServerManagerWidget::OpenOmniverseConfig()
{
	// NOOP: TODO - Open a sub-panel or dialog for Omniverse configuration
	// This would include:
	// - Omniverse Nucleus connection settings
	// - Audio2Face server address
	// - Face stream configuration
	
	AddLogMessage(TEXT("Omniverse configuration not yet implemented"));
}

FString UHoloCadeServerManagerWidget::GetServerExecutablePath() const
{
	// Build path to dedicated server executable
	FString ProjectDir = FPaths::ProjectDir();
	FString BinariesDir = FPaths::Combine(ProjectDir, TEXT("Binaries"), FPlatformProcess::GetBinariesSubdirectory());
	
#if PLATFORM_WINDOWS
	FString ExecutableName = TEXT("HoloCade_UnrealServer.exe");
#elif PLATFORM_LINUX
	FString ExecutableName = TEXT("HoloCade_UnrealServer");
#else
	FString ExecutableName = TEXT("HoloCade_UnrealServer");
#endif

	return FPaths::Combine(BinariesDir, ExecutableName);
}

FString UHoloCadeServerManagerWidget::BuildServerCommandLine() const
{
	TArray<FString> Args;

	// Map to load
	Args.Add(ServerConfig.MapName);

	// Server flags
	Args.Add(TEXT("-server"));
	Args.Add(TEXT("-log"));

	// Port
	Args.Add(FString::Printf(TEXT("-port=%d"), ServerConfig.Port));

	// Experience type (custom parameter)
	Args.Add(FString::Printf(TEXT("-ExperienceType=%s"), *ServerConfig.ExperienceType));

	// Max players
	Args.Add(FString::Printf(TEXT("-MaxPlayers=%d"), ServerConfig.MaxPlayers));

	// Join all arguments
	return FString::Join(Args, TEXT(" "));
}

void UHoloCadeServerManagerWidget::PollServerStatus()
{
	if (!ServerStatus.bIsRunning)
	{
		return;
	}

	UpdateServerStatus();
	
	// Real-time status updates now come via OnServerStatusReceived callback
	// from the network beacon. This function just verifies process is alive.
}

void UHoloCadeServerManagerWidget::OnServerStatusReceived(const FHoloCadeServerInfo& ServerInfo)
{
	// Only process status for our managed server
	// (Match by port since IP might be reported differently for localhost)
	if (ServerInfo.ServerPort != ExpectedServerPort)
	{
		return; // This is a different server on the network
	}

	// Only process if our server is marked as running
	if (!ServerStatus.bIsRunning)
	{
		return;
	}

	// Update real-time status from server broadcast
	ServerStatus.CurrentPlayers = ServerInfo.CurrentPlayers;
	ServerStatus.ExperienceState = ServerInfo.ExperienceState;

	// Log significant state changes
	static FString LastState = TEXT("");
	if (ServerStatus.ExperienceState != LastState)
	{
		AddLogMessage(FString::Printf(TEXT("Server state changed to: %s"), *ServerStatus.ExperienceState));
		LastState = ServerStatus.ExperienceState;
	}

	// Log player count changes
	static int32 LastPlayerCount = -1;
	if (ServerStatus.CurrentPlayers != LastPlayerCount)
	{
		AddLogMessage(FString::Printf(TEXT("Player count changed to: %d/%d"), 
			ServerStatus.CurrentPlayers, ServerConfig.MaxPlayers));
		LastPlayerCount = ServerStatus.CurrentPlayers;
	}
}

bool UHoloCadeServerManagerWidget::ConnectToRemoteServer()
{
	if (ConnectionMode != EHoloCadeConnectionMode::Remote)
	{
		AddLogMessage(TEXT("ERROR: Connection mode is not set to Remote"));
		return false;
	}

	if (CommandProtocol && CommandProtocol->IsActive())
	{
		AddLogMessage(TEXT("Already connected to remote server"));
		return true;
	}

	if (!CommandProtocol)
	{
		CommandProtocol = NewObject<UHoloCadeServerCommandProtocol>(this, TEXT("CommandProtocol"));
		if (!CommandProtocol)
		{
			AddLogMessage(TEXT("ERROR: Failed to create command protocol"));
			return false;
		}
	}

	// Sync authentication settings before connecting
	CommandProtocol->bEnableAuthentication = bEnableAuthentication;
	CommandProtocol->SharedSecret = SharedSecret;
	CommandProtocol->CommandPort = RemoteCommandPort;

	// Initialize client connection
	if (CommandProtocol->InitializeClient(RemoteServerIP, RemoteCommandPort))
	{
		ExpectedServerIP = RemoteServerIP;
		ExpectedServerPort = RemoteServerPort;
		AddLogMessage(FString::Printf(TEXT("Connected to remote server at %s:%d (command port: %d)"), 
			*RemoteServerIP, RemoteServerPort, RemoteCommandPort));
		return true;
	}
	else
	{
		AddLogMessage(FString::Printf(TEXT("ERROR: Failed to connect to remote server at %s:%d"), 
			*RemoteServerIP, RemoteCommandPort));
		return false;
	}
}

void UHoloCadeServerManagerWidget::DisconnectFromRemoteServer()
{
	if (ConnectionMode != EHoloCadeConnectionMode::Remote)
	{
		return;
	}

	if (CommandProtocol && CommandProtocol->IsActive())
	{
		CommandProtocol->ShutdownClient();
		AddLogMessage(TEXT("Disconnected from remote server"));
		
		// Reset status
		ServerStatus.bIsRunning = false;
		ServerStatus.CurrentPlayers = 0;
		ServerStatus.ExperienceState = TEXT("Disconnected");
	}
}

bool UHoloCadeServerManagerWidget::IsRemoteConnected() const
{
	return CommandProtocol && CommandProtocol->IsActive();
}

void UHoloCadeServerManagerWidget::OnCommandResponse(const FHoloCadeServerResponseMessage& Response)
{
	if (Response.bSuccess)
	{
		AddLogMessage(FString::Printf(TEXT("Command response: %s"), *Response.Message));
	}
	else
	{
		AddLogMessage(FString::Printf(TEXT("Command error: %s"), *Response.Message));
	}
}

void UHoloCadeServerManagerWidget::OnServerDiscoveredForConnection(const FHoloCadeServerInfo& ServerInfo)
{
	// Log discovered server
	AddLogMessage(FString::Printf(TEXT("Discovered server: %s (%s) at %s:%d"), 
		*ServerInfo.ServerName, *ServerInfo.ExperienceType, *ServerInfo.ServerIP, ServerInfo.ServerPort));

	// If in remote mode and not connected, offer to connect
	if (ConnectionMode == EHoloCadeConnectionMode::Remote && !IsRemoteConnected())
	{
		// Update remote server info from beacon (auto-fill)
		RemoteServerIP = ServerInfo.ServerIP;
		RemoteServerPort = ServerInfo.ServerPort;
		RemoteCommandPort = 7779; // Default command port

		AddLogMessage(FString::Printf(TEXT("Auto-filled remote server info from discovery: %s:%d"), 
			*RemoteServerIP, RemoteServerPort));

		// Optionally auto-connect (could be made configurable via Blueprint)
		// ConnectToRemoteServer();
	}
}

TArray<FHoloCadeServerInfo> UHoloCadeServerManagerWidget::GetDiscoveredServers() const
{
	if (ServerBeacon && ServerBeacon->IsActive())
	{
		return ServerBeacon->GetDiscoveredServers();
	}
	return TArray<FHoloCadeServerInfo>();
}

