// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "AIFacemask/AIFacemaskFaceController.h"
#include "Components/SkeletalMeshComponent.h"
#include "IWebSocketsManager.h"
#include "WebSocketsModule.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Base64.h"
#include "Serialization/BulkData.h"

UAIFacemaskFaceController::UAIFacemaskFaceController()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsInitialized = false;
	bIsConnected = false;
	
	// Initialize default blend shape name mapping (can be customized in Blueprint)
	// These are common mappings - developers can override in Blueprint
	BlendShapeNameMapping.Add(TEXT("eyeBlinkLeft"), TEXT("eyeBlinkLeft"));
	BlendShapeNameMapping.Add(TEXT("eyeBlinkRight"), TEXT("eyeBlinkRight"));
	BlendShapeNameMapping.Add(TEXT("jawOpen"), TEXT("jawOpen"));
	BlendShapeNameMapping.Add(TEXT("mouthSmileLeft"), TEXT("mouthSmileLeft"));
	BlendShapeNameMapping.Add(TEXT("mouthSmileRight"), TEXT("mouthSmileRight"));
	BlendShapeNameMapping.Add(TEXT("browInnerUp"), TEXT("browInnerUp"));
	BlendShapeNameMapping.Add(TEXT("browOuterUpLeft"), TEXT("browOuterUpLeft"));
	BlendShapeNameMapping.Add(TEXT("browOuterUpRight"), TEXT("browOuterUpRight"));
}

void UAIFacemaskFaceController::BeginPlay()
{
	Super::BeginPlay();
	
	// Auto-initialize if config is set
	if (Config.TargetMesh)
	{
		InitializeAIFace(Config);
	}
}

void UAIFacemaskFaceController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up WebSocket connection
	DisconnectFromACEEndpoint();
	
	Super::EndPlay(EndPlayReason);
}

void UAIFacemaskFaceController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// WebSocket handles incoming messages asynchronously via callbacks
	// No polling needed here - messages are processed in OnWebSocketMessageReceived()
}

bool UAIFacemaskFaceController::InitializeAIFace(const FAIFaceConfig& InConfig)
{
	Config = InConfig;

	if (!Config.TargetMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("AIFaceController: Cannot initialize - no target mesh specified"));
		return false;
	}

	// Create dynamic material instance for texture updates
	if (Config.TargetMesh->GetMaterial(0))
	{
		DynamicMaterial = Config.TargetMesh->CreateDynamicMaterialInstance(0);
		if (!DynamicMaterial)
		{
			UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Failed to create dynamic material instance - texture updates may not work"));
		}
	}

	bIsInitialized = true;
	UE_LOG(LogTemp, Log, TEXT("AIFaceController: Initialized successfully (TargetMesh: %s, Endpoint: %s)"), 
		*Config.TargetMesh->GetName(), *Config.NVIDIAACEEndpointURL);
	
	return true;
}

void UAIFacemaskFaceController::ReceiveFacialAnimationData(const FFacialAnimationData& AnimationData)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Cannot receive animation data - not initialized"));
		return;
	}

	CurrentAnimationData = AnimationData;

	// Apply blend shapes from NVIDIA ACE to target mesh
	if (Config.TargetMesh && AnimationData.BlendShapeWeights.Num() > 0)
	{
		ApplyBlendShapesToMesh(AnimationData.BlendShapeWeights);
	}

	// Apply facial texture from NVIDIA ACE to target mesh
	if (Config.TargetMesh && AnimationData.FacialTexture)
	{
		ApplyFacialTextureToMesh(AnimationData.FacialTexture);
	}
}

void UAIFacemaskFaceController::ApplyBlendShapesToMesh(const TMap<FName, float>& BlendShapeWeights)
{
	if (!Config.TargetMesh)
	{
		return;
	}

	// Apply blend shape weights to skeletal mesh morph targets
	for (const auto& BlendShapePair : BlendShapeWeights)
	{
		FName MorphTargetName = BlendShapePair.Key;
		
		// Check if we have a name mapping (NVIDIA ACE name → Unreal morph target name)
		if (const FName* MappedName = BlendShapeNameMapping.Find(BlendShapePair.Key))
		{
			MorphTargetName = *MappedName;
		}
		
		// Clamp weight to valid range (0-1)
		float ClampedWeight = FMath::Clamp(BlendShapePair.Value, 0.0f, 1.0f);
		
		// Apply morph target
		Config.TargetMesh->SetMorphTarget(MorphTargetName, ClampedWeight);
	}
}

void UAIFacemaskFaceController::ApplyFacialTextureToMesh(UTexture2D* FacialTexture)
{
	if (!Config.TargetMesh || !FacialTexture)
	{
		return;
	}

	// Apply facial texture to mesh material
	if (DynamicMaterial)
	{
		DynamicMaterial->SetTextureParameterValue(FacialTextureParameterName, FacialTexture);
	}
	else
	{
		// Fallback: try to create dynamic material instance if we don't have one
		DynamicMaterial = Config.TargetMesh->CreateDynamicMaterialInstance(0);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetTextureParameterValue(FacialTextureParameterName, FacialTexture);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Cannot apply facial texture - no dynamic material instance"));
		}
	}
}

bool UAIFacemaskFaceController::ConnectToACEEndpoint()
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Error, TEXT("AIFaceController: Cannot connect - not initialized"));
		return false;
	}

	if (bIsConnected)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Already connected to ACE endpoint"));
		return true;
	}

	if (Config.NVIDIAACEEndpointURL.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("AIFaceController: Cannot connect - no endpoint URL specified"));
		return false;
	}

	// Ensure WebSockets module is loaded
	FWebSocketsModule* WebSocketsModule = &FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));
	if (!WebSocketsModule)
	{
		UE_LOG(LogTemp, Error, TEXT("AIFaceController: WebSockets module not available"));
		return false;
	}

	// Convert HTTP URL to WebSocket URL (ws:// or wss://)
	FString WebSocketURL = Config.NVIDIAACEEndpointURL;
	if (WebSocketURL.StartsWith(TEXT("http://")))
	{
		WebSocketURL = WebSocketURL.Replace(TEXT("http://"), TEXT("ws://"));
	}
	else if (WebSocketURL.StartsWith(TEXT("https://")))
	{
		WebSocketURL = WebSocketURL.Replace(TEXT("https://"), TEXT("wss://"));
	}
	else if (!WebSocketURL.StartsWith(TEXT("ws://")) && !WebSocketURL.StartsWith(TEXT("wss://")))
	{
		// Assume HTTP if no protocol specified
		WebSocketURL = TEXT("ws://") + WebSocketURL;
	}

	// Create WebSocket connection
	WebSocket = WebSocketsModule->CreateWebSocket(WebSocketURL, TEXT(""));
	if (!WebSocket.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("AIFaceController: Failed to create WebSocket connection"));
		return false;
	}

	// Set up callbacks
	WebSocket->OnConnected().AddUObject(this, &UAIFacemaskFaceController::OnWebSocketConnected);
	WebSocket->OnConnectionError().AddUObject(this, &UAIFacemaskFaceController::OnWebSocketError);
	WebSocket->OnClosed().AddUObject(this, &UAIFacemaskFaceController::OnWebSocketConnectionClosed);
	WebSocket->OnMessage().AddUObject(this, &UAIFacemaskFaceController::OnWebSocketMessageReceived);

	// Connect
	WebSocket->Connect();
	
	UE_LOG(LogTemp, Log, TEXT("AIFaceController: Connecting to ACE endpoint: %s"), *WebSocketURL);
	return true;
}

void UAIFacemaskFaceController::DisconnectFromACEEndpoint()
{
	if (WebSocket.IsValid() && bIsConnected)
	{
		WebSocket->Close();
		WebSocket.Reset();
		bIsConnected = false;
		UE_LOG(LogTemp, Log, TEXT("AIFaceController: Disconnected from ACE endpoint"));
	}
}

void UAIFacemaskFaceController::OnWebSocketConnected()
{
	bIsConnected = true;
	UE_LOG(LogTemp, Log, TEXT("AIFaceController: Connected to NVIDIA ACE endpoint"));
}

void UAIFacemaskFaceController::OnWebSocketConnectionClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	bIsConnected = false;
	UE_LOG(LogTemp, Warning, TEXT("AIFaceController: WebSocket connection closed (Code: %d, Reason: %s, Clean: %d)"), 
		StatusCode, *Reason, bWasClean);
}

void UAIFacemaskFaceController::OnWebSocketMessageReceived(const FString& Message)
{
	if (!bIsInitialized)
	{
		return;
	}

	// Parse JSON facial animation data
	FFacialAnimationData AnimationData;
	if (ParseFacialAnimationData(Message, AnimationData))
	{
		// Apply received animation data
		ReceiveFacialAnimationData(AnimationData);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Failed to parse facial animation data from message"));
	}
}

void UAIFacemaskFaceController::OnWebSocketError(const FString& Error)
{
	UE_LOG(LogTemp, Error, TEXT("AIFaceController: WebSocket error: %s"), *Error);
	bIsConnected = false;
}

bool UAIFacemaskFaceController::ParseFacialAnimationData(const FString& JsonString, FFacialAnimationData& OutAnimationData)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Failed to parse JSON"));
		return false;
	}

	// Parse timestamp
	if (JsonObject->HasField(TEXT("timestamp")))
	{
		OutAnimationData.Timestamp = JsonObject->GetNumberField(TEXT("timestamp"));
	}

	// Parse blend shape weights
	if (JsonObject->HasField(TEXT("blendShapes")))
	{
		const TSharedPtr<FJsonObject>* BlendShapesObject = nullptr;
		if (JsonObject->TryGetObjectField(TEXT("blendShapes"), BlendShapesObject) && BlendShapesObject->IsValid())
		{
			for (const auto& Pair : (*BlendShapesObject)->Values)
			{
				FName BlendShapeName(*Pair.Key);
				float Weight = Pair.Value->AsNumber();
				OutAnimationData.BlendShapeWeights.Add(BlendShapeName, Weight);
			}
		}
	}

	// Parse facial texture (base64 encoded image)
	if (JsonObject->HasField(TEXT("facialTexture")))
	{
		FString Base64Texture = JsonObject->GetStringField(TEXT("facialTexture"));
		if (!Base64Texture.IsEmpty())
		{
			UTexture2D* Texture = CreateTextureFromBase64(Base64Texture);
			if (Texture)
			{
				OutAnimationData.FacialTexture = Texture;
			}
		}
	}

	return true;
}

UTexture2D* UAIFacemaskFaceController::CreateTextureFromBase64(const FString& Base64Data)
{
	// Decode base64 string to binary data
	TArray<uint8> DecodedData;
	if (!FBase64::Decode(Base64Data, DecodedData))
	{
		UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Failed to decode base64 texture data"));
		return nullptr;
	}

	if (DecodedData.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Decoded texture data is empty"));
		return nullptr;
	}

	// Load image wrapper module
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	
	// Detect image format from data
	EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(DecodedData.GetData(), DecodedData.Num());
	if (ImageFormat == EImageFormat::Invalid)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Could not detect image format from data"));
		return nullptr;
	}

	// Create image wrapper
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
	if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(DecodedData.GetData(), DecodedData.Num()))
	{
		UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Failed to create image wrapper or set compressed data"));
		return nullptr;
	}

	// Get uncompressed image data
	TArray<uint8> UncompressedData;
	if (!ImageWrapper->GetRaw(ERGBFormat::RGBA, 8, UncompressedData))
	{
		UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Failed to get uncompressed image data"));
		return nullptr;
	}

	// Create texture from uncompressed data
	int32 Width = ImageWrapper->GetWidth();
	int32 Height = ImageWrapper->GetHeight();
	
	if (Width <= 0 || Height <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Invalid texture dimensions: %dx%d"), Width, Height);
		return nullptr;
	}

	// Create texture
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
	if (!Texture)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIFaceController: Failed to create transient texture"));
		return nullptr;
	}

	// Copy image data to texture
	void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, UncompressedData.GetData(), UncompressedData.Num());
	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();

	// Configure texture settings for real-time updates
	Texture->CompressionSettings = TC_Default;
	Texture->SRGB = true;
	Texture->UpdateResource();

	return Texture;
}

