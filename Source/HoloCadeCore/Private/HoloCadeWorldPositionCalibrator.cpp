// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "HoloCadeWorldPositionCalibrator.h"
#include "HoloCadeCore.h"
#include "HoloCadeTrackingInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Net/UnrealNetwork.h"

UHoloCadeWorldPositionCalibrator::UHoloCadeWorldPositionCalibrator()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsCalibrating = false;
	CalibrationMode = ECalibrationMode::Manual;
	bCalibrationModeEnabled = false;
	CalibrationTrackerIndex = 0;
	ExpectedTrackerPosition = FVector::ZeroVector;
	bTrackerCalibrationComplete = false;
	WorldOriginOffset = FVector::ZeroVector;
	AxisDetectionThreshold = 5.0f;
	bAxisDetected = false;
	SetIsReplicatedByDefault(true);
}

void UHoloCadeWorldPositionCalibrator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// Replicate calibration mode state and offset to all clients
	DOREPLIFETIME(UHoloCadeWorldPositionCalibrator, bCalibrationModeEnabled);
	DOREPLIFETIME(UHoloCadeWorldPositionCalibrator, WorldOriginOffset);
}

void UHoloCadeWorldPositionCalibrator::BeginPlay()
{
	Super::BeginPlay();
	
	// Handle different calibration modes
	if (CalibrationMode == ECalibrationMode::CalibrateToTracker)
	{
		// Tracker-based calibration: perform once at launch on each client
		PerformTrackerCalibration();
	}
	else
	{
		// Manual calibration: only load saved offset on server (server is authoritative)
		if (IsServer())
		{
			// Auto-load saved calibration offset on startup
			// Try to get experience name from owner actor
			FString ExperienceName = TEXT("Default");
			if (AActor* Owner = GetOwner())
			{
				ExperienceName = Owner->GetClass()->GetName();
			}
			
			LoadCalibrationOffset(ExperienceName);
		}
	}
}

void UHoloCadeWorldPositionCalibrator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// NOOP: Will handle continuous calibration updates while trigger is held
}

void UHoloCadeWorldPositionCalibrator::StartCalibration(const FVector& InInitialGrabLocation)
{
	// Only allow manual calibration mode
	if (CalibrationMode != ECalibrationMode::Manual)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeWorldPositionCalibrator: StartCalibration only works in Manual mode"));
		return;
	}

	// Only allow calibration if calibration mode is enabled on server
	if (!bCalibrationModeEnabled)
	{
		UE_LOG(LogTemp, Verbose, TEXT("HoloCadeWorldPositionCalibrator: Calibration mode is not enabled on server"));
		return;
	}

	// Only allow calibration on client (not server directly)
	if (IsClient())
	{
		// Send to server via RPC
		ServerStartCalibration(InInitialGrabLocation);
	}
	
	// Local state for client-side preview
	bIsCalibrating = true;
	InitialGrabLocation = InInitialGrabLocation;
	LastGrabLocation = InInitialGrabLocation;
	bAxisDetected = false;
	DragAxis = FVector::ZeroVector;
}

void UHoloCadeWorldPositionCalibrator::UpdateCalibration(const FVector& CurrentGrabLocation)
{
	if (!bIsCalibrating || !bCalibrationModeEnabled)
	{
		return;
	}

	// Only allow calibration on client (not server directly)
	if (IsClient())
	{
		// Send to server via RPC
		ServerUpdateCalibration(CurrentGrabLocation);
	}

	// Detect drag axis if not yet detected (client-side preview)
	if (!bAxisDetected)
	{
		DetectDragAxis(CurrentGrabLocation);
	}

	// Client-side preview (will be overwritten by server replication)
	if (bAxisDetected)
	{
		// Calculate movement along detected axis
		FVector Movement = CurrentGrabLocation - InitialGrabLocation;
		FVector AxisMovement = FVector::DotProduct(Movement, DragAxis) * DragAxis;

		// Update world origin offset (inverse of movement - we move the world, not the grab point)
		// This is just a preview - server will send authoritative value
		WorldOriginOffset = -AxisMovement;
	}

	LastGrabLocation = CurrentGrabLocation;
}

void UHoloCadeWorldPositionCalibrator::EndCalibration()
{
	// Called when trigger is released (calibration ends)
	// This immediately triggers server save via RPC
	
	if (!bCalibrationModeEnabled)
	{
		return;
	}

	// Only allow calibration on client (not server directly)
	if (IsClient())
	{
		// Send to server via RPC - server will save to JSON file immediately
		ServerEndCalibration();
	}
	
	bIsCalibrating = false;
	bAxisDetected = false;
	DragAxis = FVector::ZeroVector;
}

void UHoloCadeWorldPositionCalibrator::ResetCalibration()
{
	WorldOriginOffset = FVector::ZeroVector;
}

void UHoloCadeWorldPositionCalibrator::EnableCalibrationMode()
{
	// Only allow on server
	if (!IsServer())
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeWorldPositionCalibrator: EnableCalibrationMode can only be called on server"));
		return;
	}
	
	bCalibrationModeEnabled = true;
	UE_LOG(LogTemp, Log, TEXT("HoloCadeWorldPositionCalibrator: Calibration mode enabled - clients can now calibrate"));
}

void UHoloCadeWorldPositionCalibrator::DisableCalibrationMode()
{
	// Only allow on server
	if (!IsServer())
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeWorldPositionCalibrator: DisableCalibrationMode can only be called on server"));
		return;
	}
	
	bCalibrationModeEnabled = false;
	UE_LOG(LogTemp, Log, TEXT("HoloCadeWorldPositionCalibrator: Calibration mode disabled"));
}

FVector UHoloCadeWorldPositionCalibrator::GetCalibratedPosition(const FVector& RawPosition) const
{
	return RawPosition + WorldOriginOffset;
}

void UHoloCadeWorldPositionCalibrator::DetectDragAxis(const FVector& CurrentLocation)
{
	FVector Movement = CurrentLocation - InitialGrabLocation;
	float MovementMagnitude = Movement.Size();

	if (MovementMagnitude < AxisDetectionThreshold)
	{
		return; // Not enough movement yet
	}

	// Normalize movement to get direction
	FVector Direction = Movement.GetSafeNormal();

	// Check which axis is dominant (X, Y, or Z)
	float AbsX = FMath::Abs(Direction.X);
	float AbsY = FMath::Abs(Direction.Y);
	float AbsZ = FMath::Abs(Direction.Z);

	if (AbsX > AbsY && AbsX > AbsZ)
	{
		// X axis dominant (horizontal - left/right)
		DragAxis = FVector(1.0f, 0.0f, 0.0f);
		bAxisDetected = true;
	}
	else if (AbsY > AbsX && AbsY > AbsZ)
	{
		// Y axis dominant (horizontal - forward/back)
		DragAxis = FVector(0.0f, 1.0f, 0.0f);
		bAxisDetected = true;
	}
	else if (AbsZ > AbsX && AbsZ > AbsY)
	{
		// Z axis dominant (vertical - up/down)
		DragAxis = FVector(0.0f, 0.0f, 1.0f);
		bAxisDetected = true;
	}
}

void UHoloCadeWorldPositionCalibrator::SaveCalibrationOffset(const FString& ExperienceName)
{
	// Only save on server (server is authoritative, saves to server's hard drive)
	// This is called immediately when trigger is released (via ServerEndCalibration RPC)
	// FFileHelper::SaveStringToFile() is synchronous - file is written immediately, no delay
	if (!IsServer())
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeWorldPositionCalibrator: SaveCalibrationOffset can only be called on server"));
		return;
	}
	
	CurrentExperienceName = ExperienceName;
	
	// Create JSON object
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	
	// Store offset as array [X, Y, Z]
	TArray<TSharedPtr<FJsonValue>> OffsetArray;
	OffsetArray.Add(MakeShareable(new FJsonValueNumber(WorldOriginOffset.X)));
	OffsetArray.Add(MakeShareable(new FJsonValueNumber(WorldOriginOffset.Y)));
	OffsetArray.Add(MakeShareable(new FJsonValueNumber(WorldOriginOffset.Z)));
	JsonObject->SetArrayField(TEXT("WorldOriginOffset"), OffsetArray);
	
	// Store timestamp
	JsonObject->SetStringField(TEXT("LastCalibrated"), FDateTime::Now().ToString());
	JsonObject->SetStringField(TEXT("ExperienceName"), ExperienceName);
	
	// Serialize to string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	
	// Write to file on server's hard drive (synchronous - writes immediately)
	FString FilePath = GetCalibrationFilePath(ExperienceName);
	bool bSaveSuccess = FFileHelper::SaveStringToFile(OutputString, *FilePath);
	
	if (bSaveSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("HoloCadeWorldPositionCalibrator: Server saved calibration offset to %s (saved immediately on trigger release)"), *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("HoloCadeWorldPositionCalibrator: Failed to save calibration offset to %s"), *FilePath);
	}
}

bool UHoloCadeWorldPositionCalibrator::LoadCalibrationOffset(const FString& ExperienceName)
{
	// Only load on server (server is authoritative)
	if (!IsServer())
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeWorldPositionCalibrator: LoadCalibrationOffset can only be called on server"));
		return false;
	}
	
	CurrentExperienceName = ExperienceName;
	
	FString FilePath = GetCalibrationFilePath(ExperienceName);
	
	// Check if file exists
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		UE_LOG(LogTemp, Verbose, TEXT("HoloCadeWorldPositionCalibrator: No saved calibration found at %s"), *FilePath);
		return false;
	}
	
	// Read file
	FString FileContents;
	if (!FFileHelper::LoadFileToString(FileContents, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeWorldPositionCalibrator: Failed to read calibration file %s"), *FilePath);
		return false;
	}
	
	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeWorldPositionCalibrator: Failed to parse calibration JSON from %s"), *FilePath);
		return false;
	}
	
	// Extract offset array
	const TArray<TSharedPtr<FJsonValue>>* OffsetArrayPtr = nullptr;
	if (!JsonObject->TryGetArrayField(TEXT("WorldOriginOffset"), OffsetArrayPtr) || !OffsetArrayPtr || OffsetArrayPtr->Num() != 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeWorldPositionCalibrator: Invalid offset format in calibration file"));
		return false;
	}
	
	// Parse offset values
	WorldOriginOffset.X = (*OffsetArrayPtr)[0]->AsNumber();
	WorldOriginOffset.Y = (*OffsetArrayPtr)[1]->AsNumber();
	WorldOriginOffset.Z = (*OffsetArrayPtr)[2]->AsNumber();
	
	FString LastCalibrated;
	if (JsonObject->TryGetStringField(TEXT("LastCalibrated"), LastCalibrated))
	{
		FString OffsetStr = FString::Printf(TEXT("(%.2f, %.2f, %.2f)"), WorldOriginOffset.X, WorldOriginOffset.Y, WorldOriginOffset.Z);
		UE_LOG(LogTemp, Log, TEXT("HoloCadeWorldPositionCalibrator: Loaded calibration offset %s from %s (calibrated: %s)"), 
			*OffsetStr, *FilePath, *LastCalibrated);
	}
	else
	{
		FString OffsetStr = FString::Printf(TEXT("(%.2f, %.2f, %.2f)"), WorldOriginOffset.X, WorldOriginOffset.Y, WorldOriginOffset.Z);
		UE_LOG(LogTemp, Log, TEXT("HoloCadeWorldPositionCalibrator: Loaded calibration offset %s from %s"), 
			*OffsetStr, *FilePath);
	}
	
	return true;
}

FString UHoloCadeWorldPositionCalibrator::GetCalibrationFilePath(const FString& ExperienceName) const
{
	// If custom path is set, use it (must be absolute path on server)
	if (!CalibrationSavePath.IsEmpty())
	{
		return CalibrationSavePath;
	}
	
	// Otherwise, use default path: Saved/Config/HoloCade/Calibration_[ExperienceName].json
	// This is similar to Unity's Application.persistentDataPath
	FString ConfigDir = FPaths::ProjectSavedDir() / TEXT("Config") / TEXT("HoloCade");
	
	// Create directory if it doesn't exist
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*ConfigDir))
	{
		PlatformFile.CreateDirectoryTree(*ConfigDir);
	}
	
	FString FileName = FString::Printf(TEXT("Calibration_%s.json"), *ExperienceName);
	return ConfigDir / FileName;
}

bool UHoloCadeWorldPositionCalibrator::IsServer() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetNetMode() == NM_DedicatedServer || World->GetNetMode() == NM_ListenServer;
	}
	return false;
}

bool UHoloCadeWorldPositionCalibrator::IsClient() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetNetMode() == NM_Client;
	}
	return false;
}

// =====================================
// Server RPCs (Client -> Server)
// =====================================

void UHoloCadeWorldPositionCalibrator::ServerStartCalibration_Implementation(const FVector& InInitialGrabLocation)
{
	// Server receives calibration start from client
	// Only process if calibration mode is enabled
	if (!bCalibrationModeEnabled)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeWorldPositionCalibrator: Server rejected calibration start - calibration mode not enabled"));
		return;
	}
	
	InitialGrabLocation = InInitialGrabLocation;
	LastGrabLocation = InInitialGrabLocation;
	bAxisDetected = false;
	DragAxis = FVector::ZeroVector;
	
	UE_LOG(LogTemp, Log, TEXT("HoloCadeWorldPositionCalibrator: Server received calibration start from client"));
}

bool UHoloCadeWorldPositionCalibrator::ServerStartCalibration_Validate(const FVector& InInitialGrabLocation)
{
	return bCalibrationModeEnabled; // Only validate if calibration mode is enabled
}

void UHoloCadeWorldPositionCalibrator::ServerUpdateCalibration_Implementation(const FVector& CurrentGrabLocation)
{
	// Server receives calibration update from client
	// Only process if calibration mode is enabled
	if (!bCalibrationModeEnabled)
	{
		return;
	}
	
	// Detect drag axis if not yet detected
	if (!bAxisDetected)
	{
		DetectDragAxis(CurrentGrabLocation);
	}

	if (bAxisDetected)
	{
		// Calculate movement along detected axis
		FVector Movement = CurrentGrabLocation - InitialGrabLocation;
		FVector AxisMovement = FVector::DotProduct(Movement, DragAxis) * DragAxis;

		// Update world origin offset (inverse of movement - we move the world, not the grab point)
		WorldOriginOffset = -AxisMovement;
		
		// Replicate to all clients
		ClientUpdateCalibrationOffset(WorldOriginOffset);
	}

	LastGrabLocation = CurrentGrabLocation;
}

bool UHoloCadeWorldPositionCalibrator::ServerUpdateCalibration_Validate(const FVector& CurrentGrabLocation)
{
	return bCalibrationModeEnabled; // Only validate if calibration mode is enabled
}

void UHoloCadeWorldPositionCalibrator::ServerEndCalibration_Implementation()
{
	// Server receives calibration end from client (trigger released)
	// Only process if calibration mode is enabled
	if (!bCalibrationModeEnabled)
	{
		return;
	}
	
	// Save calibration offset to JSON file immediately (synchronous write)
	// This happens as soon as trigger is released - no delay
	FString ExperienceName = TEXT("Default");
	if (AActor* Owner = GetOwner())
	{
		ExperienceName = Owner->GetClass()->GetName();
	}
	
	SaveCalibrationOffset(ExperienceName);
	
	UE_LOG(LogTemp, Log, TEXT("HoloCadeWorldPositionCalibrator: Server saved calibration offset to file (trigger released)"));
}

bool UHoloCadeWorldPositionCalibrator::ServerEndCalibration_Validate()
{
	return bCalibrationModeEnabled; // Only validate if calibration mode is enabled
}

// =====================================
// Client RPCs (Server -> Client)
// =====================================

void UHoloCadeWorldPositionCalibrator::ClientUpdateCalibrationOffset_Implementation(const FVector& NewOffset)
{
	// Client receives updated offset from server
	WorldOriginOffset = NewOffset;
}

bool UHoloCadeWorldPositionCalibrator::PerformTrackerCalibration()
{
	// Only perform if not already completed (one-time calibration at launch)
	if (bTrackerCalibrationComplete)
	{
		UE_LOG(LogTemp, Verbose, TEXT("HoloCadeWorldPositionCalibrator: Tracker calibration already completed"));
		return true;
	}

	// Get tracking interface from owner
	IHoloCadeTrackingInterface* TrackingInterface = GetTrackingInterface();
	if (!TrackingInterface)
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeWorldPositionCalibrator: No tracking interface available for tracker calibration"));
		return false;
	}

	// Check if tracker is tracking
	if (!TrackingInterface->IsDeviceTracking(CalibrationTrackerIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeWorldPositionCalibrator: Tracker is not tracking"));
		return false;
	}

	// Get actual tracker position
	FTransform TrackerTransform;
	if (!TrackingInterface->GetTrackedDeviceTransform(CalibrationTrackerIndex, TrackerTransform))
	{
		UE_LOG(LogTemp, Warning, TEXT("HoloCadeWorldPositionCalibrator: Failed to get tracker transform"));
		return false;
	}

	FVector ActualTrackerPosition = TrackerTransform.GetLocation();

	// Calculate offset: ExpectedPosition - ActualPosition
	// This moves the world so the tracker appears at ExpectedPosition
	WorldOriginOffset = ExpectedTrackerPosition - ActualTrackerPosition;

	bTrackerCalibrationComplete = true;

	// Format offset string using SanitizeFloat to avoid format string sanitizer issues
	FString OffsetStr = FString::Printf(TEXT("(%s, %s, %s)"), 
		*FString::SanitizeFloat(WorldOriginOffset.X, 2),
		*FString::SanitizeFloat(WorldOriginOffset.Y, 2),
		*FString::SanitizeFloat(WorldOriginOffset.Z, 2));
	UE_LOG(LogTemp, Log, TEXT("HoloCadeWorldPositionCalibrator: Tracker calibration complete - Offset: %s"), *OffsetStr);

	return true;
}

IHoloCadeTrackingInterface* UHoloCadeWorldPositionCalibrator::GetTrackingInterface() const
{
	// Try to get tracking interface from owner actor
	AActor* Owner = GetOwner();
	if (Owner)
	{
		// Check if owner implements IHoloCadeTrackingInterface
		// Use ImplementsInterface to check if interface is implemented
		if (Owner->GetClass()->ImplementsInterface(UHoloCadeTrackingInterface::StaticClass()))
		{
			return Cast<IHoloCadeTrackingInterface>(Owner);
		}
	}
	
	return nullptr;
}

