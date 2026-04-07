// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IContainerManager.generated.h"

/**
 * @brief Structure for container configuration.
 */
USTRUCT(BlueprintType)
struct FContainerConfig
{
	GENERATED_BODY()

	/** Container image name (e.g., "nvcr.io/nim/llama-3.2-3b-instruct:latest") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	FString ImageName;

	/** Container name (for Docker management) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	FString ContainerName;

	/** Host port (external port) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	int32 HostPort = 8000;

	/** Container port (internal port) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	int32 ContainerPort = 8000;

	/** Whether GPU access is required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	bool bRequireGPU = true;

	/** Environment variables (key=value pairs) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	TMap<FString, FString> EnvironmentVariables;

	/** Volume mounts (host:container pairs) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	TMap<FString, FString> VolumeMounts;

	FContainerConfig()
		: HostPort(8000)
		, ContainerPort(8000)
		, bRequireGPU(true)
	{}
};

/**
 * @brief Interface for container management.
 * Enables starting, stopping, and monitoring Docker containers from Unreal Engine.
 * 
 * **Docker CLI Approach:**
 * Uses Docker CLI commands (not HTTP API) for simplicity and security:
 * - No TLS required (local socket/pipe communication)
 * - No network exposure (local Docker daemon only)
 * - No authentication setup (Docker daemon handles permissions)
 * 
 * **Platform Support:**
 * - Windows: Named pipe at `\\.\pipe\docker_engine`
 * - Linux: Unix socket at `/var/run/docker.sock`
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UContainerManager : public UInterface
{
	GENERATED_BODY()
};

class HOLOCADEAI_API IContainerManager
{
	GENERATED_BODY()

public:
	/**
	 * @brief Checks if a container is currently running.
	 * @param ContainerName The name of the container to check.
	 * @return True if the container is running, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Container Manager")
	bool IsContainerRunning(const FString& ContainerName) const;

	/**
	 * @brief Starts a container with the given configuration.
	 * @param Config Container configuration (image, ports, GPU, etc.).
	 * @return True if the container was started successfully, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Container Manager")
	bool StartContainer(const FContainerConfig& Config);

	/**
	 * @brief Stops a running container.
	 * @param ContainerName The name of the container to stop.
	 * @return True if the container was stopped successfully, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Container Manager")
	bool StopContainer(const FString& ContainerName);

	/**
	 * @brief Removes a container (must be stopped first).
	 * @param ContainerName The name of the container to remove.
	 * @return True if the container was removed successfully, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Container Manager")
	bool RemoveContainer(const FString& ContainerName);

	/**
	 * @brief Checks if Docker CLI is available and Docker daemon is running.
	 * @return True if Docker is available, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Container Manager")
	bool IsDockerAvailable() const;

	/**
	 * @brief Gets container status information.
	 * @param ContainerName The name of the container to check.
	 * @param bIsRunning Output: Whether container is running.
	 * @param bExists Output: Whether container exists (stopped or running).
	 * @return True if status was retrieved successfully.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Container Manager")
	bool GetContainerStatus(const FString& ContainerName, bool& bIsRunning, bool& bExists) const;
};

