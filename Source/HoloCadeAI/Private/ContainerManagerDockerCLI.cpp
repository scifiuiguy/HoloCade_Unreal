// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "ContainerManagerDockerCLI.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

UContainerManagerDockerCLI::UContainerManagerDockerCLI()
{
	LastError = TEXT("");
	LastOutput = TEXT("");
}

bool UContainerManagerDockerCLI::IsDockerAvailable_Implementation() const
{
	// Check if Docker CLI is available
	if (!CheckDockerCLI())
	{
		LastError = TEXT("Docker CLI not found in PATH");
		return false;
	}

	// Check if Docker daemon is running
	if (!ExecuteDockerCommand(TEXT("docker ps"), false))
	{
		LastError = TEXT("Docker daemon is not running or not accessible");
		return false;
	}

	return true;
}

bool UContainerManagerDockerCLI::IsContainerRunning_Implementation(const FString& ContainerName) const
{
	if (ContainerName.IsEmpty())
	{
		LastError = TEXT("Container name is empty");
		return false;
	}

	// Check if container is running: docker ps --filter "name=ContainerName" --format "{{.Names}}"
	FString Command = FString::Printf(TEXT("docker ps --filter \"name=^%s$\" --format \"{{.Names}}\""), *EscapeDockerArgument(ContainerName));
	
	if (!ExecuteDockerCommand(Command, true))
	{
		return false;
	}

	// If output contains the container name, it's running
	return LastOutput.TrimStartAndEnd().Equals(ContainerName, ESearchCase::CaseSensitive);
}

bool UContainerManagerDockerCLI::GetContainerStatus_Implementation(const FString& ContainerName, bool& bIsRunning, bool& bExists) const
{
	bIsRunning = false;
	bExists = false;

	if (ContainerName.IsEmpty())
	{
		LastError = TEXT("Container name is empty");
		return false;
	}

	// Check if container exists (running or stopped): docker ps -a --filter "name=ContainerName" --format "{{.Names}}"
	FString Command = FString::Printf(TEXT("docker ps -a --filter \"name=^%s$\" --format \"{{.Names}}\""), *EscapeDockerArgument(ContainerName));
	
	if (!ExecuteDockerCommand(Command, true))
	{
		return false;
	}

	FString Output = LastOutput.TrimStartAndEnd();
	bExists = !Output.IsEmpty() && Output.Equals(ContainerName, ESearchCase::CaseSensitive);

	if (bExists)
	{
		// Check if it's running
		bIsRunning = IsContainerRunning_Implementation(ContainerName);
	}

	return true;
}

bool UContainerManagerDockerCLI::StartContainer_Implementation(const FContainerConfig& Config)
{
	if (Config.ImageName.IsEmpty())
	{
		LastError = TEXT("Container image name is empty");
		return false;
	}

	if (Config.ContainerName.IsEmpty())
	{
		LastError = TEXT("Container name is empty");
		return false;
	}

	// Check if container already exists and is running
	bool bIsRunning = false;
	bool bExists = false;
	if (GetContainerStatus_Implementation(Config.ContainerName, bIsRunning, bExists))
	{
		if (bIsRunning)
		{
			UE_LOG(LogTemp, Log, TEXT("ContainerManagerDockerCLI: Container '%s' is already running"), *Config.ContainerName);
			return true; // Already running, consider it success
		}

		if (bExists)
		{
			// Container exists but is stopped, start it
			FString Command = FString::Printf(TEXT("docker start %s"), *EscapeDockerArgument(Config.ContainerName));
			if (ExecuteDockerCommand(Command, true))
			{
				UE_LOG(LogTemp, Log, TEXT("ContainerManagerDockerCLI: Started existing container '%s'"), *Config.ContainerName);
				return true;
			}
			return false;
		}
	}

	// Container doesn't exist, create and start it
	FString DockerCommand = BuildDockerRunCommand(Config);
	
	if (!ExecuteDockerCommand(DockerCommand, true))
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("ContainerManagerDockerCLI: Started new container '%s' with image '%s'"), 
		*Config.ContainerName, *Config.ImageName);
	return true;
}

bool UContainerManagerDockerCLI::StopContainer_Implementation(const FString& ContainerName)
{
	if (ContainerName.IsEmpty())
	{
		LastError = TEXT("Container name is empty");
		return false;
	}

	// Check if container is running
	bool bIsRunning = false;
	bool bExists = false;
	if (!GetContainerStatus_Implementation(ContainerName, bIsRunning, bExists))
	{
		return false;
	}

	if (!bExists)
	{
		UE_LOG(LogTemp, Warning, TEXT("ContainerManagerDockerCLI: Container '%s' does not exist"), *ContainerName);
		LastError = FString::Printf(TEXT("Container '%s' does not exist"), *ContainerName);
		return false;
	}

	if (!bIsRunning)
	{
		UE_LOG(LogTemp, Log, TEXT("ContainerManagerDockerCLI: Container '%s' is already stopped"), *ContainerName);
		return true; // Already stopped, consider it success
	}

	// Stop the container
	FString Command = FString::Printf(TEXT("docker stop %s"), *EscapeDockerArgument(ContainerName));
	
	if (!ExecuteDockerCommand(Command, true))
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("ContainerManagerDockerCLI: Stopped container '%s'"), *ContainerName);
	return true;
}

bool UContainerManagerDockerCLI::RemoveContainer_Implementation(const FString& ContainerName)
{
	if (ContainerName.IsEmpty())
	{
		LastError = TEXT("Container name is empty");
		return false;
	}

	// Check if container exists
	bool bIsRunning = false;
	bool bExists = false;
	if (!GetContainerStatus_Implementation(ContainerName, bIsRunning, bExists))
	{
		return false;
	}

	if (!bExists)
	{
		UE_LOG(LogTemp, Warning, TEXT("ContainerManagerDockerCLI: Container '%s' does not exist"), *ContainerName);
		LastError = FString::Printf(TEXT("Container '%s' does not exist"), *ContainerName);
		return false;
	}

	// Stop container first if it's running
	if (bIsRunning)
	{
		if (!StopContainer_Implementation(ContainerName))
		{
			return false;
		}
	}

	// Remove the container
	FString Command = FString::Printf(TEXT("docker rm %s"), *EscapeDockerArgument(ContainerName));
	
	if (!ExecuteDockerCommand(Command, true))
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("ContainerManagerDockerCLI: Removed container '%s'"), *ContainerName);
	return true;
}

bool UContainerManagerDockerCLI::ExecuteDockerCommand(const FString& Command, bool bCaptureOutput) const
{
	LastError = TEXT("");
	LastOutput = TEXT("");

	// Execute the command
	FString StdOut;
	FString StdErr;
	int32 ReturnCode = 0;

	const FString CommandLine = FString::Printf(TEXT("/C %s"), *Command);

	bool bSuccess = FPlatformProcess::ExecProcess(
		TEXT("cmd.exe"),
		*CommandLine,
		&ReturnCode,
		bCaptureOutput ? &StdOut : nullptr,
		bCaptureOutput ? &StdErr : nullptr
	);

	if (!bSuccess)
	{
		LastError = FString::Printf(TEXT("Failed to execute Docker command: %s"), *Command);
		UE_LOG(LogTemp, Error, TEXT("ContainerManagerDockerCLI: %s"), *LastError);
		return false;
	}

	if (ReturnCode != 0)
	{
		LastError = FString::Printf(TEXT("Docker command failed with exit code %d: %s"), ReturnCode, *StdErr);
		LastOutput = StdOut;
		UE_LOG(LogTemp, Warning, TEXT("ContainerManagerDockerCLI: %s"), *LastError);
		return false;
	}

	LastOutput = StdOut;
	return true;
}

FString UContainerManagerDockerCLI::BuildDockerRunCommand(const FContainerConfig& Config) const
{
	FString Command = TEXT("docker run -d");

	// Container name
	Command += FString::Printf(TEXT(" --name %s"), *EscapeDockerArgument(Config.ContainerName));

	// Port mapping
	Command += FString::Printf(TEXT(" -p %d:%d"), Config.HostPort, Config.ContainerPort);

	// GPU access
	if (Config.bRequireGPU)
	{
		Command += TEXT(" --gpus all");
	}

	// Environment variables
	for (const auto& EnvVar : Config.EnvironmentVariables)
	{
		Command += FString::Printf(TEXT(" -e %s=%s"), 
			*EscapeDockerArgument(EnvVar.Key), 
			*EscapeDockerArgument(EnvVar.Value));
	}

	// Volume mounts
	for (const auto& Volume : Config.VolumeMounts)
	{
		Command += FString::Printf(TEXT(" -v %s:%s"), 
			*EscapeDockerArgument(Volume.Key), 
			*EscapeDockerArgument(Volume.Value));
	}

	// Image name (must be last)
	Command += FString::Printf(TEXT(" %s"), *EscapeDockerArgument(Config.ImageName));

	return Command;
}

FString UContainerManagerDockerCLI::EscapeDockerArgument(const FString& Input) const
{
	// For Docker CLI, we need to escape quotes and spaces
	// Simple approach: wrap in quotes if contains spaces or special chars
	if (Input.Contains(TEXT(" ")) || Input.Contains(TEXT("\"")) || Input.Contains(TEXT("'")))
	{
		// Escape quotes and wrap in double quotes
		FString Escaped = Input;
		Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}

	return Input;
}

bool UContainerManagerDockerCLI::CheckDockerCLI() const
{
	// Check if Docker CLI is available: docker --version
	FString StdOut;
	FString StdErr;
	int32 ReturnCode = 0;

	const FString Params = TEXT("/C docker --version");
	bool bSuccess = FPlatformProcess::ExecProcess(
		TEXT("cmd.exe"),
		*Params,
		&ReturnCode,
		&StdOut,
		&StdErr
	);

	return bSuccess && ReturnCode == 0;
}

