# Superhero Flight Experience - NOOP Sections Summary

This document lists all NOOP (Not Operational) sections in the Superhero Flight Experience that are marked for future implementation.

**Last Updated:** 2025-01-XX

---

## 1. FlightHandsController (`FlightHandsController.cpp`)

### 1.1 Multiplayer Replication
- **Location:** Line 97
- **Status:** NOOP
- **Description:** Gesture state replication to server for multiplayer support
- **Implementation Required:**
  - Replicate `FSuperheroFlightGestureState` struct to server
  - Use Unreal Replication system (Replicated properties or RPCs)
  - Handle network authority checks (only local player replicates)
  - **Note:** Currently marked as NOOP for initial pass, but multiplayer support is planned

### 1.2 Virtual Altitude Raycast - Landable Surface Detection
- **Location:** Lines 421-434
- **Status:** Partial Implementation (basic raycast works, landable surface filtering is NOOP)
- **Description:** Raycast to detect landable surfaces for virtual altitude calculation
- **Current Implementation:**
  - Basic raycast using `LineTraceSingleByChannel` with `ECC_WorldStatic`
  - Returns hit point if collision detected
- **Missing Implementation:**
  - Filter by collision tags/channels to identify "landable" surfaces
  - Distinguish between landable surfaces (ground, platforms) and non-landable surfaces (walls, ceilings)
  - Use collision tags or custom collision channel for landable surfaces
- **Implementation Required:**
  ```cpp
  // Check if hit actor/component has "Landable" tag
  if (HitResult.GetActor()->ActorHasTag(TEXT("Landable")))
  {
      OutHitPoint = HitResult.ImpactPoint;
      return true;
  }
  ```

---

## 2. SuperheroFlightExperience (`SuperheroFlightExperience.cpp`)

### 2.1 RF433MHz Receiver Configuration
- **Location:** Lines 74-79
- **Status:** NOOP
- **Description:** Initialize and configure 433MHz RF receiver for height calibration clicker
- **Missing Implementation:**
  - Configure USB device path for RF receiver dongle
  - Configure security settings (rolling code validation, replay attack prevention)
  - Subscribe to button function events (`OnButtonFunctionTriggered` delegate)
  - Map button functions to calibration commands (HeightUp, HeightDown)
- **Implementation Required:**
  ```cpp
  // Configure RF433MHzReceiver
  FRF433MHzReceiverConfig Config;
  Config.USBDevicePath = TEXT("COM3");  // Or auto-detect
  Config.bEnableRollingCode = true;
  Config.bEnableReplayAttackPrevention = true;
  RF433MHzReceiver->InitializeReceiver(Config);
  
  // Subscribe to button events
  RF433MHzReceiver->OnButtonFunctionTriggered.AddDynamic(this, &ASuperheroFlightExperience::HandleCalibrationButton);
  ```

### 2.2 Calibration Mode Disable
- **Location:** Line 133
- **Status:** NOOP
- **Description:** Disable calibration mode when timeout is reached
- **Missing Implementation:**
  - Set calibration mode flag to false
  - Disable RF433MHzReceiver button processing
  - Log calibration mode disabled event
- **Implementation Required:**
  ```cpp
  bCalibrationModeEnabled = false;
  RF433MHzReceiver->SetButtonProcessingEnabled(false);
  UE_LOG(LogSuperheroFlight, Warning, TEXT("Calibration mode disabled due to timeout"));
  ```

---

## 3. SuperheroFlightECUController (`SuperheroFlightECUController.cpp`)

### 3.1 UDP Data Processing
- **Location:** Lines 31, 192-196
- **Status:** NOOP
- **Description:** Process incoming UDP data from ECU and parse binary HoloCade protocol packets
- **Missing Implementation:**
  - Register callback with `HoloCadeUDPTransport` to receive data
  - Parse binary HoloCade protocol packets
  - Extract Channel 310 (dual-winch state) struct
  - Extract Channel 311 (system telemetry) struct
  - Update `LastWinchState` and `LastTelemetry` with parsed data
  - Update timestamp variables (`LastWinchStateTime`, `LastTelemetryTime`)
- **Implementation Required:**
  ```cpp
  // In InitializeECU():
  UDPTransport->OnDataReceived.AddDynamic(this, &USuperheroFlightECUController::ProcessReceivedData);
  
  // In ProcessReceivedData():
  // Parse binary HoloCade protocol
  // Extract Channel 310: FSuperheroFlightDualWinchState
  // Extract Channel 311: FSuperheroFlightTelemetry
  // Update LastWinchState, LastTelemetry, timestamps
  ```

### 3.2 GetDualWinchState - Channel 310 Parsing
- **Location:** Lines 178-183
- **Status:** NOOP (returns cached data, but parsing is not implemented)
- **Description:** Parse Channel 310 struct from UDP to get dual-winch state
- **Current Implementation:**
  - Returns cached `LastWinchState`
  - Checks if data is fresh (within timeout)
- **Missing Implementation:**
  - Actual parsing of Channel 310 binary data in `ProcessReceivedData()`
  - See section 3.1 above

### 3.3 GetSystemTelemetry - Channel 311 Parsing
- **Location:** Lines 185-190
- **Status:** NOOP (returns cached data, but parsing is not implemented)
- **Description:** Parse Channel 311 struct from UDP to get system telemetry
- **Current Implementation:**
  - Returns cached `LastTelemetry`
  - Checks if data is fresh (within timeout)
- **Missing Implementation:**
  - Actual parsing of Channel 311 binary data in `ProcessReceivedData()`
  - See section 3.1 above

---

## 4. GestureDebugger (`GestureDebugger.cpp`)

### 4.1 Get Hand Positions from FlightHandsController
- **Location:** Lines 74-78, 101-103, 123, 142
- **Status:** NOOP
- **Description:** Get actual HMD and hand positions from FlightHandsController for debug visualization
- **Current Implementation:**
  - Uses placeholder `FVector::ZeroVector` values
- **Missing Implementation:**
  - Call `FlightHandsController->GetHMDPosition()`
  - Call `FlightHandsController->GetLeftHandPosition()`
  - Call `FlightHandsController->GetRightHandPosition()`
- **Implementation Required:**
  ```cpp
  FVector HMDPos = FlightHandsController->GetHMDPosition();
  FVector LeftHandPos = FlightHandsController->GetLeftHandPosition();
  FVector RightHandPos = FlightHandsController->GetRightHandPosition();
  ```

### 4.2 Draw Angle Threshold Arcs
- **Location:** Lines 125-128
- **Status:** NOOP
- **Description:** Draw visual arcs showing transition zones for angle thresholds
- **Missing Implementation:**
  - Draw debug arcs showing up-to-forward angle threshold
  - Draw debug arcs showing forward-to-down angle threshold
  - Visual representation of gesture angle zones
- **Implementation Required:**
  ```cpp
  // Draw arc from HMD position showing angle thresholds
  // Use DrawDebugArc or DrawDebugLine to visualize transition zones
  ```

### 4.3 Draw HUD Text Overlay
- **Location:** Lines 155-164
- **Status:** NOOP
- **Description:** Draw HUD text overlay showing flight mode, arm extension, virtual altitude, gesture angle, and fist states
- **Missing Implementation:**
  - Create UMG HUD widget or use Canvas API
  - Display current flight mode
  - Display arm extension percentage (flight speed throttle)
  - Display virtual altitude
  - Display gesture angle
  - Display fist states (left/right/both)
- **Implementation Required:**
  ```cpp
  // Option 1: Use UMG HUD widget
  // Option 2: Use Canvas API (DrawText on UCanvas)
  // Display all gesture state information in HMD viewport
  ```

---

## Summary by Priority

### High Priority (Core Functionality)
1. **ECU UDP Data Processing** (Section 3.1) - Required for receiving winch state and telemetry
2. **Channel 310/311 Parsing** (Sections 3.2, 3.3) - Required for winch state feedback
3. **RF433MHz Receiver Configuration** (Section 2.1) - Required for height calibration

### Medium Priority (User Experience)
4. **GestureDebugger Hand Positions** (Section 4.1) - Required for debug visualization
5. **Virtual Altitude Landable Surface Filtering** (Section 1.2) - Improves landing detection accuracy
6. **GestureDebugger HUD Text** (Section 4.3) - Helpful for Ops Tech calibration

### Low Priority (Nice to Have)
7. **GestureDebugger Angle Threshold Arcs** (Section 4.2) - Visual debugging aid
8. **Calibration Mode Disable** (Section 2.2) - Safety feature (timeout handling)
9. **Multiplayer Replication** (Section 1.1) - Future feature (marked as NOOP for initial pass)

---

## Notes

- **Multiplayer Replication** is intentionally marked as NOOP for the initial pass, as noted in the roadmap. This will be implemented as part of the "VR Player Transport" task.
- **ECU UDP Data Processing** is critical - without it, the experience cannot receive winch state feedback or telemetry from the hardware.
- **RF433MHz Receiver** is required for the height calibration workflow, but the experience can function without it (manual calibration via console commands).

