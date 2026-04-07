// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "RF433MHzReceiver.h"
#include "RF433MHz.h"
#include "I433MHzReceiver.h"
#include "Generic433MHzReceiver.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"

URF433MHzReceiver::URF433MHzReceiver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f / 20.0f; // Default 20 Hz
}

URF433MHzReceiver::~URF433MHzReceiver()
{
	ShutdownReceiver();
}

void URF433MHzReceiver::BeginPlay()
{
	Super::BeginPlay();

	// Load saved button mappings on startup
	LoadButtonMappings();

	if (!Config.USBDevicePath.IsEmpty())
	{
		InitializeReceiver(Config);
	}
}

void URF433MHzReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownReceiver();
	Super::EndPlay(EndPlayReason);
}

void URF433MHzReceiver::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ReceiverImpl || !ReceiverImpl->IsConnected())
	{
		return;
	}

	// Update tick interval based on config
	if (Config.UpdateRate > 0.0f)
	{
		PrimaryComponentTick.TickInterval = 1.0f / Config.UpdateRate;
	}

	// Poll for button events
	TArray<FRF433MHzButtonEvent> Events;
	if (ReceiverImpl->GetButtonEvents(Events))
	{
		ProcessButtonEvents(Events);
	}
}

bool URF433MHzReceiver::InitializeReceiver(const FRF433MHzReceiverConfig& InConfig)
{
	Config = InConfig;

	if (ReceiverImpl && ReceiverImpl->IsConnected())
	{
		UE_LOG(LogRF433MHz, Warning, TEXT("RF433MHzReceiver: Already initialized"));
		return false;
	}

	// Create receiver using factory
	ReceiverImpl = I433MHzReceiver::CreateReceiver(Config);

	if (!ReceiverImpl || !ReceiverImpl->Initialize(Config))
	{
		UE_LOG(LogRF433MHz, Error, TEXT("RF433MHzReceiver: Failed to initialize receiver (Type: %d, Device: %s)"), 
			(uint8)Config.ReceiverType, *Config.USBDevicePath);
		ReceiverImpl.Reset();
		return false;
	}

	UE_LOG(LogRF433MHz, Log, TEXT("RF433MHzReceiver: Initialized (Type: %d, Device: %s)"), 
		(uint8)Config.ReceiverType, *Config.USBDevicePath);

	return true;
}

void URF433MHzReceiver::ShutdownReceiver()
{
	if (ReceiverImpl)
	{
		ReceiverImpl->Shutdown();
		ReceiverImpl.Reset();
		UE_LOG(LogRF433MHz, Log, TEXT("RF433MHzReceiver: Shutdown"));
	}
}

bool URF433MHzReceiver::IsConnected() const
{
	return ReceiverImpl && ReceiverImpl->IsConnected();
}

bool URF433MHzReceiver::GetButtonEvents(TArray<FRF433MHzButtonEvent>& OutEvents)
{
	if (!ReceiverImpl || !ReceiverImpl->IsConnected())
	{
		return false;
	}

	return ReceiverImpl->GetButtonEvents(OutEvents);
}

bool URF433MHzReceiver::IsRollingCodeValid() const
{
	if (!ReceiverImpl)
	{
		return false;
	}

	return ReceiverImpl->IsRollingCodeValid();
}

int32 URF433MHzReceiver::GetRollingCodeDrift() const
{
	if (!ReceiverImpl)
	{
		return 0;
	}

	return ReceiverImpl->GetRollingCodeDrift();
}

void URF433MHzReceiver::EnableLearningMode(float TimeoutSeconds)
{
	if (ReceiverImpl)
	{
		ReceiverImpl->EnableLearningMode(TimeoutSeconds);
	}
}

void URF433MHzReceiver::DisableLearningMode()
{
	if (ReceiverImpl)
	{
		ReceiverImpl->DisableLearningMode();
	}
}

bool URF433MHzReceiver::IsLearningModeActive() const
{
	if (!ReceiverImpl)
	{
		return false;
	}

	return ReceiverImpl->IsLearningModeActive();
}

void URF433MHzReceiver::ProcessButtonEvents(const TArray<FRF433MHzButtonEvent>& Events)
{
	for (const FRF433MHzButtonEvent& Event : Events)
	{
		// Check if learning mode is active - register new button
		if (IsLearningModeActive())
		{
			// Check if this is a new button (not yet learned)
			if (!IsButtonLearned(Event.ButtonCode))
			{
				RegisterLearnedButton(Event.ButtonCode, Event.RollingCode);
				OnCodeLearned.Broadcast(Event.ButtonCode, Event.RollingCode);
				UE_LOG(LogRF433MHz, Log, TEXT("RF433MHzReceiver: Learned new button (Code: %d, RollingCode: %u)"), 
					Event.ButtonCode, Event.RollingCode);
			}
		}

		// Check if this is a new state change
		bool* LastState = LastButtonStates.Find(Event.ButtonCode);
		bool bWasPressed = LastState ? *LastState : false;

		if (Event.bPressed != bWasPressed)
		{
			// State changed - trigger delegates
			if (Event.bPressed)
			{
				OnButtonPressed.Broadcast(Event.ButtonCode);
			}
			else
			{
				OnButtonReleased.Broadcast(Event.ButtonCode);
			}

			OnButtonEvent.Broadcast(Event.ButtonCode, Event.bPressed);

			// Check if button has assigned function - trigger function delegate
			FString* FunctionName = ButtonFunctionMappings.Find(Event.ButtonCode);
			if (FunctionName && !FunctionName->IsEmpty())
			{
				OnButtonFunctionTriggered.Broadcast(Event.ButtonCode, *FunctionName, Event.bPressed);
			}

			// Update last state
			LastButtonStates.Add(Event.ButtonCode, Event.bPressed);
		}
	}
}

int32 URF433MHzReceiver::GetLearnedButtons(TArray<FRF433MHzLearnedButton>& OutLearnedButtons) const
{
	OutLearnedButtons.Empty();
	LearnedButtons.GenerateValueArray(OutLearnedButtons);
	return OutLearnedButtons.Num();
}

int32 URF433MHzReceiver::GetLearnedButtonCount() const
{
	return LearnedButtons.Num();
}

bool URF433MHzReceiver::IsButtonLearned(int32 ButtonCode) const
{
	return LearnedButtons.Contains(ButtonCode);
}

bool URF433MHzReceiver::AssignButtonFunction(int32 ButtonCode, const FString& FunctionName)
{
	if (!IsButtonLearned(ButtonCode))
	{
		UE_LOG(LogRF433MHz, Warning, TEXT("RF433MHzReceiver: Cannot assign function to unlearned button (Code: %d)"), ButtonCode);
		return false;
	}

	if (FunctionName.IsEmpty())
	{
		UE_LOG(LogRF433MHz, Warning, TEXT("RF433MHzReceiver: Function name cannot be empty"));
		return false;
	}

	ButtonFunctionMappings.Add(ButtonCode, FunctionName);

	// Update learned button info
	if (FRF433MHzLearnedButton* LearnedButton = LearnedButtons.Find(ButtonCode))
	{
		LearnedButton->AssignedFunctionName = FunctionName;
		LearnedButton->bIsMapped = true;
	}

	UE_LOG(LogRF433MHz, Log, TEXT("RF433MHzReceiver: Assigned function '%s' to button %d"), *FunctionName, ButtonCode);
	
	// Auto-save if enabled
	AutoSaveIfEnabled();
	
	return true;
}

bool URF433MHzReceiver::UnassignButtonFunction(int32 ButtonCode)
{
	if (!ButtonFunctionMappings.Contains(ButtonCode))
	{
		return false;
	}

	ButtonFunctionMappings.Remove(ButtonCode);

	// Update learned button info
	if (FRF433MHzLearnedButton* LearnedButton = LearnedButtons.Find(ButtonCode))
	{
		LearnedButton->AssignedFunctionName = TEXT("");
		LearnedButton->bIsMapped = false;
	}

	UE_LOG(LogRF433MHz, Log, TEXT("RF433MHzReceiver: Unassigned function from button %d"), ButtonCode);
	
	// Auto-save if enabled
	AutoSaveIfEnabled();
	
	return true;
}

bool URF433MHzReceiver::GetButtonFunction(int32 ButtonCode, FString& OutFunctionName) const
{
	const FString* FunctionName = ButtonFunctionMappings.Find(ButtonCode);
	if (FunctionName && !FunctionName->IsEmpty())
	{
		OutFunctionName = *FunctionName;
		return true;
	}

	OutFunctionName = TEXT("");
	return false;
}

int32 URF433MHzReceiver::GetButtonMappings(TArray<FRF433MHzButtonMapping>& OutMappings) const
{
	OutMappings.Empty();

	for (const auto& Pair : ButtonFunctionMappings)
	{
		FRF433MHzButtonMapping Mapping;
		Mapping.ButtonCode = Pair.Key;
		Mapping.FunctionName = Pair.Value;
		Mapping.bIsActive = true;
		OutMappings.Add(Mapping);
	}

	return OutMappings.Num();
}

void URF433MHzReceiver::ClearAllButtons()
{
	LearnedButtons.Empty();
	ButtonFunctionMappings.Empty();
	LastButtonStates.Empty();
	UE_LOG(LogRF433MHz, Log, TEXT("RF433MHzReceiver: Cleared all learned buttons and mappings"));
	
	// Auto-save if enabled (saves empty state)
	AutoSaveIfEnabled();
}

bool URF433MHzReceiver::RemoveLearnedButton(int32 ButtonCode)
{
	if (!IsButtonLearned(ButtonCode))
	{
		return false;
	}

	LearnedButtons.Remove(ButtonCode);
	ButtonFunctionMappings.Remove(ButtonCode);
	LastButtonStates.Remove(ButtonCode);

	UE_LOG(LogRF433MHz, Log, TEXT("RF433MHzReceiver: Removed learned button %d"), ButtonCode);
	
	// Auto-save if enabled
	AutoSaveIfEnabled();
	
	return true;
}

void URF433MHzReceiver::RegisterLearnedButton(int32 ButtonCode, int32 RollingCode)
{
	FRF433MHzLearnedButton LearnedButton;
	LearnedButton.ButtonCode = ButtonCode;
	LearnedButton.RollingCodeSeed = RollingCode;
	LearnedButton.LearnedTimestamp = FPlatformTime::Seconds();
	LearnedButton.AssignedFunctionName = TEXT("");
	LearnedButton.bIsMapped = false;

	LearnedButtons.Add(ButtonCode, LearnedButton);
	
	// Auto-save if enabled
	AutoSaveIfEnabled();
}

bool URF433MHzReceiver::SaveButtonMappings(const FString& CustomFilePath)
{
	FString FilePath = CustomFilePath.IsEmpty() ? GetDefaultButtonMappingsFilePath() : CustomFilePath;
	
	// Create JSON object
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	
	// Store metadata
	JsonObject->SetStringField(TEXT("LastSaved"), FDateTime::Now().ToString());
	JsonObject->SetNumberField(TEXT("Version"), 1);
	
	// Store learned buttons array
	TArray<TSharedPtr<FJsonValue>> LearnedButtonsArray;
	for (const auto& Pair : LearnedButtons)
	{
		const FRF433MHzLearnedButton& LearnedButton = Pair.Value;
		TSharedPtr<FJsonObject> ButtonObject = MakeShareable(new FJsonObject);
		ButtonObject->SetNumberField(TEXT("ButtonCode"), LearnedButton.ButtonCode);
		ButtonObject->SetNumberField(TEXT("RollingCodeSeed"), (double)LearnedButton.RollingCodeSeed);
		ButtonObject->SetNumberField(TEXT("LearnedTimestamp"), LearnedButton.LearnedTimestamp);
		ButtonObject->SetStringField(TEXT("AssignedFunctionName"), LearnedButton.AssignedFunctionName);
		ButtonObject->SetBoolField(TEXT("bIsMapped"), LearnedButton.bIsMapped);
		
		LearnedButtonsArray.Add(MakeShareable(new FJsonValueObject(ButtonObject)));
	}
	JsonObject->SetArrayField(TEXT("LearnedButtons"), LearnedButtonsArray);
	
	// Store button mappings array
	TArray<TSharedPtr<FJsonValue>> MappingsArray;
	for (const auto& Pair : ButtonFunctionMappings)
	{
		TSharedPtr<FJsonObject> MappingObject = MakeShareable(new FJsonObject);
		MappingObject->SetNumberField(TEXT("ButtonCode"), Pair.Key);
		MappingObject->SetStringField(TEXT("FunctionName"), Pair.Value);
		
		MappingsArray.Add(MakeShareable(new FJsonValueObject(MappingObject)));
	}
	JsonObject->SetArrayField(TEXT("ButtonMappings"), MappingsArray);
	
	// Serialize to string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	
	// Write to file
	bool bSaveSuccess = FFileHelper::SaveStringToFile(OutputString, *FilePath);
	
	if (bSaveSuccess)
	{
		UE_LOG(LogRF433MHz, Log, TEXT("RF433MHzReceiver: Saved %d learned buttons and %d mappings to %s"), 
			LearnedButtons.Num(), ButtonFunctionMappings.Num(), *FilePath);
	}
	else
	{
		UE_LOG(LogRF433MHz, Error, TEXT("RF433MHzReceiver: Failed to save button mappings to %s"), *FilePath);
	}
	
	return bSaveSuccess;
}

bool URF433MHzReceiver::LoadButtonMappings(const FString& CustomFilePath)
{
	FString FilePath = CustomFilePath.IsEmpty() ? GetDefaultButtonMappingsFilePath() : CustomFilePath;
	
	// Check if file exists
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.FileExists(*FilePath))
	{
		UE_LOG(LogRF433MHz, Log, TEXT("RF433MHzReceiver: Button mappings file not found at %s (will create on first save)"), *FilePath);
		return false;
	}
	
	// Read file
	FString FileContents;
	if (!FFileHelper::LoadFileToString(FileContents, *FilePath))
	{
		UE_LOG(LogRF433MHz, Error, TEXT("RF433MHzReceiver: Failed to read button mappings file from %s"), *FilePath);
		return false;
	}
	
	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogRF433MHz, Error, TEXT("RF433MHzReceiver: Failed to parse button mappings JSON from %s"), *FilePath);
		return false;
	}
	
	// Clear existing data
	LearnedButtons.Empty();
	ButtonFunctionMappings.Empty();
	
	// Load learned buttons
	const TArray<TSharedPtr<FJsonValue>>* LearnedButtonsArrayPtr = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("LearnedButtons"), LearnedButtonsArrayPtr) && LearnedButtonsArrayPtr)
	{
		for (const TSharedPtr<FJsonValue>& Value : *LearnedButtonsArrayPtr)
		{
			const TSharedPtr<FJsonObject>* ButtonObjectPtr = nullptr;
			if (Value->TryGetObject(ButtonObjectPtr) && ButtonObjectPtr && ButtonObjectPtr->IsValid())
			{
				const TSharedPtr<FJsonObject>& ButtonObject = *ButtonObjectPtr;
				
				FRF433MHzLearnedButton LearnedButton;
				LearnedButton.ButtonCode = ButtonObject->GetIntegerField(TEXT("ButtonCode"));
				LearnedButton.RollingCodeSeed = (int32)ButtonObject->GetNumberField(TEXT("RollingCodeSeed"));
				LearnedButton.LearnedTimestamp = ButtonObject->GetNumberField(TEXT("LearnedTimestamp"));
				LearnedButton.AssignedFunctionName = ButtonObject->GetStringField(TEXT("AssignedFunctionName"));
				LearnedButton.bIsMapped = ButtonObject->GetBoolField(TEXT("bIsMapped"));
				
				LearnedButtons.Add(LearnedButton.ButtonCode, LearnedButton);
			}
		}
	}
	
	// Load button mappings
	const TArray<TSharedPtr<FJsonValue>>* MappingsArrayPtr = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("ButtonMappings"), MappingsArrayPtr) && MappingsArrayPtr)
	{
		for (const TSharedPtr<FJsonValue>& Value : *MappingsArrayPtr)
		{
			const TSharedPtr<FJsonObject>* MappingObjectPtr = nullptr;
			if (Value->TryGetObject(MappingObjectPtr) && MappingObjectPtr && MappingObjectPtr->IsValid())
			{
				const TSharedPtr<FJsonObject>& MappingObject = *MappingObjectPtr;
				
				int32 ButtonCode = MappingObject->GetIntegerField(TEXT("ButtonCode"));
				FString FunctionName = MappingObject->GetStringField(TEXT("FunctionName"));
				
				ButtonFunctionMappings.Add(ButtonCode, FunctionName);
				
				// Update learned button mapping status
				if (FRF433MHzLearnedButton* LearnedButton = LearnedButtons.Find(ButtonCode))
				{
					LearnedButton->AssignedFunctionName = FunctionName;
					LearnedButton->bIsMapped = true;
				}
			}
		}
	}
	
	FString LastSaved;
	if (JsonObject->TryGetStringField(TEXT("LastSaved"), LastSaved))
	{
		UE_LOG(LogRF433MHz, Log, TEXT("RF433MHzReceiver: Loaded %d learned buttons and %d mappings from %s (saved: %s)"), 
			LearnedButtons.Num(), ButtonFunctionMappings.Num(), *FilePath, *LastSaved);
	}
	else
	{
		UE_LOG(LogRF433MHz, Log, TEXT("RF433MHzReceiver: Loaded %d learned buttons and %d mappings from %s"), 
			LearnedButtons.Num(), ButtonFunctionMappings.Num(), *FilePath);
	}
	
	return true;
}

FString URF433MHzReceiver::GetDefaultButtonMappingsFilePath() const
{
	// Use default path: Saved/Config/HoloCade/RF433MHz_Buttons.json
	FString ConfigDir = FPaths::ProjectSavedDir() / TEXT("Config") / TEXT("HoloCade");
	
	// Create directory if it doesn't exist
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*ConfigDir))
	{
		PlatformFile.CreateDirectoryTree(*ConfigDir);
	}
	
	return ConfigDir / TEXT("RF433MHz_Buttons.json");
}

void URF433MHzReceiver::SetAutoSave(bool bEnable)
{
	bAutoSaveEnabled = bEnable;
	UE_LOG(LogRF433MHz, Log, TEXT("RF433MHzReceiver: Auto-save %s"), bEnable ? TEXT("enabled") : TEXT("disabled"));
}

bool URF433MHzReceiver::IsAutoSaveEnabled() const
{
	return bAutoSaveEnabled;
}

void URF433MHzReceiver::AutoSaveIfEnabled()
{
	if (bAutoSaveEnabled)
	{
		SaveButtonMappings();
	}
}

