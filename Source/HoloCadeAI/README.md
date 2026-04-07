# HoloCadeAI Module

Low-level AI API for all generative AI capabilities in HoloCade. This module provides LLM providers, ASR providers, TTS providers, Audio2Face integration, container management, and HTTP/gRPC clients for AI service communication.

## Overview

The HoloCadeAI module provides a unified, extensible interface for integrating AI services into HoloCade experiences. It supports:

- **LLM Providers**: Ollama, OpenAI-compatible (NVIDIA NIM, vLLM, etc.)
- **ASR Providers**: NVIDIA Riva, Parakeet, Canary, Whisper (via NIM)
- **TTS Providers**: NVIDIA Riva
- **Container Management**: Docker CLI wrapper for managing AI service containers
- **HTTP/gRPC Clients**: Communication with AI services

## Quick Start

### LLM Provider Setup

```cpp
// Initialize LLM provider (NVIDIA NIM container)
ULLMProviderManager* LLMProvider = NewObject<ULLMProviderManager>();
LLMProvider->InitializeProvider(
    TEXT("http://localhost:8000"),  // Endpoint URL
    ELLMProviderType::OpenAICompatible,  // Provider type
    TEXT("llama-3.2-3b")  // Model name
);

// Request response
FLLMRequest Request;
Request.PlayerInput = "Hello!";
Request.SystemPrompt = "You are a helpful assistant.";
Request.ModelName = "llama-3.2-3b";
Request.Temperature = 0.7f;
Request.MaxTokens = 150;

LLMProvider->RequestResponse(Request, [](const FLLMResponse& Response) {
    UE_LOG(LogTemp, Log, TEXT("Response: %s"), *Response.ResponseText);
});
```

### ASR Provider Setup

```cpp
// Initialize ASR provider (NVIDIA Riva or NIM container)
UASRProviderManager* ASRProvider = NewObject<UASRProviderManager>();
ASRProvider->Initialize(
    GRPCClient,  // gRPC client instance
    TEXT("localhost:50051"),  // Endpoint URL
    EASRProviderType::Riva  // Provider type
);

// Process audio
ASRProvider->ProcessAudio(AudioData, SampleRate, [](const FString& Transcript) {
    UE_LOG(LogTemp, Log, TEXT("Transcript: %s"), *Transcript);
});
```

### Container Management

```cpp
// Create container manager
UContainerManagerDockerCLI* ContainerManager = NewObject<UContainerManagerDockerCLI>(this);

// Check if container is running
bool bIsRunning = false;
bool bExists = false;
ContainerManager->GetContainerStatus(TEXT("holocade-llm-llama"), bIsRunning, bExists);

if (!bIsRunning)
{
    // Start container
    FContainerConfig Config;
    Config.ImageName = TEXT("nvcr.io/nim/llama-3.2-3b-instruct:latest");
    Config.ContainerName = TEXT("holocade-llm-llama");
    Config.HostPort = 8000;
    Config.ContainerPort = 8000;
    Config.bRequireGPU = true;
    
    ContainerManager->StartContainer(Config);
}
```

## LLM Providers

### Provider Interface System

All LLM providers implement the `ILLMProvider` interface, enabling hot-swapping at runtime without code changes.

### Built-in Providers

1. **ULLMProviderOllama** - Ollama API provider
   - Endpoint: `http://localhost:11434`
   - Supports custom LoRA models

2. **ULLMProviderOpenAICompatible** - OpenAI-compatible API provider
   - Works with: NVIDIA NIM, vLLM, OpenAI API, Claude API (if compatible)
   - Endpoint: `http://localhost:8000` (or any OpenAI-compatible endpoint)

### NVIDIA NIM Containerized Architecture

NVIDIA NIM runs as Docker containers, making it perfect for hot-swapping:

- **Containerized Models**: Each model runs in its own Docker container
- **Independent Ports**: Each container exposes API on its own port (default 8000)
- **Hot-Swapping**: Swap models by changing endpoint URL (no code changes)
- **Multiple Models**: Run multiple models simultaneously on different ports

### Available NIM LLM Models

| Model | Container Image | GPU Memory | Recommended For |
|-------|----------------|-----------|-----------------|
| **Llama 3.2 3B** ⭐ Default | `nvcr.io/nim/llama-3.2-3b-instruct:latest` | ~6GB | Best balance for LBE |
| Llama 3.1 8B | `nvcr.io/nim/llama-3.1-8b-instruct:latest` | ~16GB | Higher quality |
| Mistral 7B | `nvcr.io/nim/mistral-7b-instruct:latest` | ~14GB | Good quality alternative |
| Phi 3 Mini | `nvcr.io/nim/phi-3-mini-4k-instruct:latest` | ~4GB | Fastest, smallest |
| Llama 2 7B | `nvcr.io/nim/llama-2-7b-instruct:latest` | ~14GB | General purpose |
| Llama 2 13B | `nvcr.io/nim/llama-2-13b-instruct:latest` | ~26GB | Higher quality |
| Gemma 2B | `nvcr.io/nim/gemma-2b-it:latest` | ~4GB | Fast, lightweight |
| Gemma 7B | `nvcr.io/nim/gemma-7b-it:latest` | ~14GB | General purpose |

**See [NIM_MODELS.md](Common/NIM_MODELS.md) for complete list.**

### Starting NIM LLM Containers

```bash
# Llama 3.2 3B (recommended for LBE - good balance of quality and speed)
docker run -d -p 8000:8000 --gpus all \
  nvcr.io/nim/llama-3.2-3b-instruct:latest

# Mistral 7B (alternative - good quality)
docker run -d -p 8001:8000 --gpus all \
  nvcr.io/nim/mistral-7b-instruct:latest

# Llama 3.1 8B (higher quality, more GPU memory required)
docker run -d -p 8002:8000 --gpus all \
  nvcr.io/nim/llama-3.1-8b-instruct:latest
```

### Creating Custom Providers

To add a custom LLM provider:

1. **Implement ILLMProvider interface:**
```cpp
UCLASS()
class MYMODULE_API ULLMProviderCustom : public UObject, public ILLMProvider
{
    GENERATED_BODY()

public:
    virtual void RequestResponse(
        const FLLMRequest& Request, 
        TFunction<void(const FLLMResponse&)> Callback
    ) override
    {
        // Your custom implementation
        FLLMResponse Response;
        Response.ResponseText = "Custom response";
        Callback(Response);
    }

    virtual bool IsAvailable() const override { return true; }
    virtual FString GetProviderName() const override { return TEXT("Custom"); }
    virtual TArray<FString> GetSupportedModels() const override { return {}; }
};
```

2. **Register with Provider Manager:**
```cpp
ULLMProviderManager* Manager = GetProviderManager();
ULLMProviderCustom* CustomProvider = NewObject<ULLMProviderCustom>();
Manager->RegisterCustomProvider(CustomProvider);
```

## ASR Providers

### Provider Interface System

All ASR providers implement the `IASRProvider` interface, enabling hot-swapping at runtime.

### Available ASR Models

#### NVIDIA Riva ASR (Current Default)

**Protocol:** gRPC  
**Endpoint:** `localhost:50051`  
**Deployment Options:**
- Containerized (Docker) - Recommended, easier setup
- Local SDK Installation - More complex, requires manual setup

**Advantages:**
- ✅ Low latency (gRPC is very efficient)
- ✅ Optimized for real-time streaming
- ✅ GPU-accelerated
- ✅ Production-ready
- ✅ Full streaming support

#### NVIDIA NIM ASR Models (Containerized)

**Important:** NIM ASR models support **gRPC** (not just HTTP REST), allowing you to maintain low latency while using containerized models.

##### Parakeet Models (Recommended)

| Model | Container Image | GPU Memory | gRPC Support | Language Support |
|-------|----------------|-----------|--------------|-----------------|
| Parakeet 0.6B CTC English | `nvcr.io/nim/parakeet-ctc-0.6b-en-us:latest` | ~2GB | ✅ Streaming + Offline | English (en-US) |
| Parakeet 1.1B RNNT Multilingual | `nvcr.io/nim/parakeet-rnnt-1.1b:latest` | ~3GB | ✅ Streaming + Offline | Multilingual |

**✅ Recommended:** Parakeet models support **streaming gRPC**, making them suitable for real-time conversation while maintaining low latency.

##### Canary Models

| Model | Container Image | GPU Memory | gRPC Support | Features |
|-------|----------------|-----------|--------------|----------|
| Canary 1B Multilingual | `nvcr.io/nim/canary-1b:latest` | ~3GB | ✅ Streaming + Offline | ASR + Translation |

**✅ Recommended:** Canary supports **streaming gRPC**, making it suitable for real-time conversation while maintaining low latency. Also provides translation capabilities.

##### Whisper Models (Not Recommended for Real-Time)

| Model | Container Image | GPU Memory | gRPC Support | Recommended For |
|-------|----------------|-----------|--------------|-----------------|
| Whisper Small | `nvcr.io/nim/whisper-small:latest` | ~2GB | ⚠️ Offline only (no streaming) | Batch processing |
| Whisper Medium | `nvcr.io/nim/whisper-medium:latest` | ~5GB | ⚠️ Offline only (no streaming) | Batch processing |
| Whisper Large | `nvcr.io/nim/whisper-large-v3:latest` | ~10GB | ⚠️ Offline only (no streaming) | Batch processing |

**⚠️ Important:** Whisper models in NIM support gRPC but **only for offline recognition** (not streaming). For real-time conversation, Whisper is not suitable via gRPC. Use Parakeet or Canary instead for streaming gRPC.

### Starting ASR Containers

```bash
# Parakeet 1.1B Multilingual (streaming gRPC) ⭐ RECOMMENDED
docker run -d -p 50052:50051 --gpus all \
  --name parakeet-rnnt-1.1b \
  nvcr.io/nim/parakeet-rnnt-1.1b:latest

# Canary 1B (ASR + Translation, streaming gRPC)
docker run -d -p 50053:50051 --gpus all \
  --name canary-1b \
  nvcr.io/nim/canary-1b:latest

# Riva ASR (via NIM)
docker run -d -p 50051:50051 --gpus all \
  --name riva-asr \
  nvcr.io/nim/riva-asr:latest
```

### Hot-Swapping ASR Models

You can run multiple ASR models simultaneously on different gRPC ports:

```bash
# Start multiple models (all via gRPC, maintaining low latency)
docker run -d -p 50051:50051 --gpus all nvcr.io/nim/riva-asr:latest  # Riva
docker run -d -p 50052:50051 --gpus all nvcr.io/nim/parakeet-rnnt-1.1b:latest  # Parakeet 1.1B
docker run -d -p 50053:50051 --gpus all nvcr.io/nim/canary-1b:latest  # Canary 1B

# In Unreal: Change LocalASREndpointURL to swap models (all gRPC, no latency sacrifice)
# localhost:50051 = Riva ASR (gRPC streaming)
# localhost:50052 = Parakeet 1.1B (gRPC streaming) ✅
# localhost:50053 = Canary 1B (gRPC streaming) ✅
```

**Key Point:** All recommended models (Riva, Parakeet, Canary) support **gRPC streaming**, so you can hot-swap between them without sacrificing latency. Whisper models are excluded because they don't support streaming gRPC.

## Container Management

The `UContainerManagerDockerCLI` class provides Docker container lifecycle management from Unreal Engine. It uses Docker CLI commands (not HTTP API) for simplicity and security.

### Requirements

- **Docker CLI** must be installed and in PATH
- **Docker daemon** must be running (Docker Desktop on Windows, Docker service on Linux)
- **User permissions** - User must have access to Docker daemon (Linux: user in `docker` group, Windows: Docker Desktop handles this)

### Basic Usage

```cpp
// Create container manager
UContainerManagerDockerCLI* ContainerManager = NewObject<UContainerManagerDockerCLI>(this);

// Check Docker availability
if (!ContainerManager->IsDockerAvailable())
{
    UE_LOG(LogTemp, Error, TEXT("Docker is not available: %s"), 
        *ContainerManager->GetLastError());
    return;
}

// Start a container
FContainerConfig Config;
Config.ImageName = TEXT("nvcr.io/nim/llama-3.2-3b-instruct:latest");
Config.ContainerName = TEXT("holocade-llm-llama");
Config.HostPort = 8000;
Config.ContainerPort = 8000;
Config.bRequireGPU = true;

if (ContainerManager->StartContainer(Config))
{
    UE_LOG(LogTemp, Log, TEXT("Container started successfully"));
}

// Check container status
bool bIsRunning = false;
bool bExists = false;
ContainerManager->GetContainerStatus(TEXT("holocade-llm-llama"), bIsRunning, bExists);

// Stop a container
ContainerManager->StopContainer(TEXT("holocade-llm-llama"));

// Remove a container
ContainerManager->RemoveContainer(TEXT("holocade-llm-llama"));
```

### Common Container Configurations

#### LLM Containers (NIM)

```cpp
// Llama 3.2 3B
FContainerConfig Config;
Config.ImageName = TEXT("nvcr.io/nim/llama-3.2-3b-instruct:latest");
Config.ContainerName = TEXT("holocade-llm-llama");
Config.HostPort = 8000;
Config.ContainerPort = 8000;
Config.bRequireGPU = true;

// Mistral 7B
Config.ImageName = TEXT("nvcr.io/nim/mistral-7b-instruct:latest");
Config.ContainerName = TEXT("holocade-llm-mistral");
Config.HostPort = 8001;
Config.ContainerPort = 8000;
Config.bRequireGPU = true;
```

#### ASR Containers (NIM)

```cpp
// Parakeet 1.1B Multilingual (Recommended)
FContainerConfig Config;
Config.ImageName = TEXT("nvcr.io/nim/parakeet-rnnt-1.1b:latest");
Config.ContainerName = TEXT("holocade-asr-parakeet");
Config.HostPort = 50052;
Config.ContainerPort = 50051;  // NIM ASR containers use 50051 internally
Config.bRequireGPU = true;

// Canary 1B (ASR + Translation)
Config.ImageName = TEXT("nvcr.io/nim/canary-1b:latest");
Config.ContainerName = TEXT("holocade-asr-canary");
Config.HostPort = 50053;
Config.ContainerPort = 50051;
Config.bRequireGPU = true;
```

### Security

- ✅ **No TLS required** - Uses local socket/pipe (not HTTP)
- ✅ **No network exposure** - Direct local communication with Docker daemon
- ✅ **No authentication setup** - Docker daemon handles permissions
- ✅ **Simpler and more secure** than Docker HTTP API approach

### Platform-Specific Notes

**Windows:**
- Docker Desktop must be running
- Docker CLI is typically in PATH automatically
- Uses named pipe: `\\.\pipe\docker_engine`

**Linux:**
- Docker service must be running: `sudo systemctl start docker`
- User must be in `docker` group: `sudo usermod -aG docker $USER`
- Uses Unix socket: `/var/run/docker.sock`

## TurboLink gRPC Integration

TurboLink is a free, open-source Unreal Engine plugin that provides native gRPC support for low-latency speech recognition and text-to-speech.

### Why TurboLink?

For real-time conversation, we need low latency:
- **HTTP REST**: ~9ms overhead per request
- **gRPC**: ~1ms overhead per request

For **two round-trips** (ASR + TTS), that's:
- **REST Gateway:** ~18ms overhead
- **Native gRPC:** ~2ms overhead

**That 16ms savings is critical** when trying to hit a 200ms total pipeline target.

### Installation

**Option A: Automated Setup (Recommended)**

Run the setup script from the project root:

```powershell
# From HoloCade_Unreal root directory
.\Plugins\HoloCade\Source\HoloCadeAI\Common\SetupTurboLink.ps1
```

**Option B: Manual Installation**

```powershell
# From HoloCade_Unreal root directory
cd Plugins
git clone https://github.com/thejinchao/turbolink.git TurboLink
```

Then download ThirdParty libraries:
1. Go to: https://github.com/thejinchao/turbolink-libraries/releases
2. Download: `turbolink-libraries.ue54.zip` (UE 5.4 version - closest to 5.5.4)
3. Extract to: `Plugins/TurboLink/Source/ThirdParty/`

**⚠️ CRITICAL:** TurboLink requires pre-built gRPC/protobuf libraries. The build **will fail** without these.

### Setup Steps

1. **Regenerate Project Files**
   - Right-click `HoloCade_Unreal.uproject`
   - Select **"Generate Visual Studio project files"**

2. **Enable TurboLink Plugin**
   - Open Unreal Editor
   - Go to **Edit → Plugins**
   - Search for **"TurboLink"**
   - Enable the plugin
   - Restart Unreal Editor

3. **Build Project**
   - Open `HoloCade_Unreal.sln` in Visual Studio
   - Build solution (Development Editor configuration)
   - First build may take 10-20 minutes

4. **Generate Protocol Buffer Classes**
   - Navigate to: `Plugins/TurboLink/Tools/`
   - Run: `generate_code.cmd <proto_file> <output_path>`
   - Copy generated `Private/` and `Public/` directories to: `Plugins/TurboLink/Source/TurboLinkGrpc/`
   - Regenerate project files and rebuild

### Resources

- **TurboLink GitHub:** https://github.com/thejinchao/turbolink
- **TurboLink Libraries:** https://github.com/thejinchao/turbolink-libraries/releases
- **NVIDIA Riva Documentation:** https://www.nvidia.com/en-us/ai-data-science/products/riva/

## NVIDIA Riva vs NIM

### Quick Answer

**Riva IS containerized** - it runs as Docker containers. Riva can be deployed in two ways:
1. **Containerized (Docker)** - Recommended, easier
2. **Local SDK Installation** - More complex, manual setup

**NIM (NVIDIA Inference Microservices)** is NVIDIA's newer containerized platform that includes Riva services as containers, plus many other models (LLMs, ASR, TTS, etc.).

### What is NVIDIA Riva?

**NVIDIA Riva** is NVIDIA's speech AI SDK for ASR (Automatic Speech Recognition) and TTS (Text-to-Speech).

- **Developed by NVIDIA** (not an acquisition - named after RIVA 128 GPU from 1997)
- **Containerized by design** - Riva runs as Docker containers
- **gRPC protocol** - Uses gRPC for low-latency communication
- **Production-ready** - Used in enterprise deployments

### What is NVIDIA NIM?

**NVIDIA NIM (NVIDIA Inference Microservices)** is NVIDIA's containerized microservices platform.

- **Containerized models** - All models run as Docker containers
- **Includes Riva services** - Riva ASR/TTS available as NIM containers
- **Plus many other models** - LLMs (Llama, Mistral, etc.), ASR (Parakeet, Canary, Whisper), etc.
- **Unified platform** - One containerized approach for all NVIDIA AI services
- **Hot-swappable** - Easy to swap between models

### Recommendation: Use NIM Containers

**Why:**
- ✅ Unified platform (all models in one place)
- ✅ Easy hot-swapping (same as other NIM models)
- ✅ Consistent deployment (same Docker approach)
- ✅ Future-proof (NVIDIA is consolidating on NIM)

## NVIDIA NIM Container Management

### What NIM Provides

✅ **What NIM Does:**
- Provides pre-built, optimized Docker containers for AI models
- Standardizes APIs (OpenAI-compatible for LLMs, gRPC for ASR/TTS)
- Simplifies model deployment (no need to build containers yourself)
- For Kubernetes: NIM Operator provides full lifecycle management

❌ **What NIM Doesn't Do (Standalone Docker):**
- **No REST API for container lifecycle** - NIM containers don't expose APIs to start/stop themselves
- **Still requires Docker CLI** - You still need `docker run` commands to start containers
- **No container orchestration API** - No HTTP endpoints to manage container instances
- **No model chain composition API** - NIM doesn't provide a workflow orchestration layer

### The Reality

**For standalone Docker (our use case):**
- NIM provides the containers, but you still need to manage them via Docker CLI/API
- NIM doesn't abstract Docker CLI - it just provides better containers
- We still need to implement container management ourselves

**For Kubernetes:**
- NIM Operator handles everything, but requires Kubernetes infrastructure
- Not suitable for simple LBE deployments (single server, no K8s)

### What We Need to Implement

Since NIM doesn't provide container lifecycle APIs, we need to:

1. **Docker CLI Wrapper** - Execute `docker run`/`docker stop` commands
2. **Container Status Checking** - `docker ps` to check if running
3. **Auto-Start on Init** - Start containers if not running
4. **Health Monitoring** - Restart crashed containers

This is implemented in `UContainerManagerDockerCLI`.

## Best Practices

1. **Use NIM for Production**: Containerized approach is ideal for LBE deployments
2. **Run Multiple Models**: Start containers on different ports for A/B testing
3. **Hot-Swap During Off-Hours**: Change models when no players are active
4. **Monitor Container Health**: Use Docker health checks
5. **GPU Memory Management**: Ensure sufficient GPU memory for each container
6. **Check Docker Availability**: Always verify Docker is available before container operations
7. **Handle Errors Gracefully**: Show user-friendly messages for Docker failures
8. **Log Container Operations**: Log all operations for debugging

## Troubleshooting

### Docker CLI Not Found

**Error:** "Docker CLI not found in PATH"

**Solution:**
- Ensure Docker CLI is installed
- Add Docker to PATH if needed
- On Windows: Docker Desktop should add it automatically
- On Linux: Install Docker CLI package

### Docker Daemon Not Running

**Error:** "Docker daemon is not running or not accessible"

**Solution:**
- On Windows: Start Docker Desktop
- On Linux: Start Docker service: `sudo systemctl start docker`
- Check Docker daemon status: `docker ps`

### Permission Denied

**Error:** Docker commands fail with permission errors

**Solution:**
- On Linux: Add user to docker group: `sudo usermod -aG docker $USER`
- Log out and log back in for group changes to take effect
- On Windows: Docker Desktop handles permissions automatically

### Provider Not Available

**Error:** LLM/ASR provider not responding

**Solution:**
- Check container is running: `docker ps`
- Verify endpoint URL is correct
- Check firewall/network connectivity
- Check container logs: `docker logs <container-id>`
- Verify GPU availability: `nvidia-smi`

## Resources

- **NVIDIA NIM Documentation:** https://docs.nvidia.com/nim/
- **NVIDIA NIM Supported Models:** https://docs.nvidia.com/nim/large-language-models/latest/supported-llm-specific-models.html
- **NVIDIA Riva Documentation:** https://docs.nvidia.com/deeplearning/riva/user-guide/docs/
- **TurboLink GitHub:** https://github.com/thejinchao/turbolink
- **Docker Documentation:** https://docs.docker.com/

