// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "IContainerManager.h"
#include "ContainerManagerDockerCLI.generated.h"

/**
 * @brief Docker CLI-based container manager implementation.
 * 
 * Uses Docker CLI commands to manage containers (not HTTP API):
 * - No TLS required (local socket/pipe communication)
 * - No network exposure (local Docker daemon only)
 * - Simpler and more secure than Docker API approach
 * 
 * **Platform Support:**
 * - Windows: Named pipe at `\\.\pipe\docker_engine`
 * - Linux: Unix socket at `/var/run/docker.sock`
 * 
 * **Requirements:**
 * - Docker CLI must be installed and in PATH
 * - Docker daemon must be running
 * - User must have permissions to access Docker daemon
 */
UCLASS(BlueprintType)
class HOLOCADEAI_API UContainerManagerDockerCLI : public UObject, public IContainerManager
{
	GENERATED_BODY()

public:
	UContainerManagerDockerCLI();

	virtual bool IsContainerRunning_Implementation(const FString& ContainerName) const override;
	virtual bool StartContainer_Implementation(const FContainerConfig& Config) override;
	virtual bool StopContainer_Implementation(const FString& ContainerName) override;
	virtual bool RemoveContainer_Implementation(const FString& ContainerName) override;
	virtual bool IsDockerAvailable_Implementation() const override;
	virtual bool GetContainerStatus_Implementation(const FString& ContainerName, bool& bIsRunning, bool& bExists) const override;

	/**
	 * @brief Gets the last error message from Docker operations.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Container Manager")
	FString GetLastError() const { return LastError; }

	/**
	 * @brief Gets the last output from Docker operations.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Container Manager")
	FString GetLastOutput() const { return LastOutput; }

private:
	/** Last error message from Docker operations */
	mutable FString LastError;

	/** Last output from Docker operations */
	mutable FString LastOutput;

	/**
	 * @brief Executes a Docker command and returns the result.
	 * @param Command Docker command to execute (e.g., "docker ps", "docker run ...").
	 * @param bCaptureOutput Whether to capture command output.
	 * @return True if command executed successfully (exit code 0).
	 */
	bool ExecuteDockerCommand(const FString& Command, bool bCaptureOutput = true) const;

	/**
	 * @brief Builds Docker run command from configuration.
	 * @param Config Container configuration.
	 * @return Docker run command string.
	 */
	FString BuildDockerRunCommand(const FContainerConfig& Config) const;

	/**
	 * @brief Escapes a string for use in Docker command (handles spaces, special chars).
	 * @param Input String to escape.
	 * @return Escaped string.
	 */
	FString EscapeDockerArgument(const FString& Input) const;

	/**
	 * @brief Checks if Docker CLI is in PATH.
	 * @return True if Docker CLI is available.
	 */
	bool CheckDockerCLI() const;
};

