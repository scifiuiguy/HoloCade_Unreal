# AIFacemask Experience

The AIFacemask experience template provides a pre-configured solution for LAN multiplayer VR experiences with immersive theater live actors using AI-driven facial animation.

## Architecture

### Network Requirements
- **Dedicated Server Required**: This experience requires a separate local PC running a headless dedicated server
- The same server PC runs the NVIDIA ACE pipeline: Audio → NLU → Emotion → Facial Animation
- NVIDIA ACE streams facial textures and blend shapes to HMDs over the network
- Offloads AI processing from HMDs for optimal performance
- Supports parallelization for multiple live actors

### AI Facial Animation
- Fully automated by NVIDIA ACE - NO manual control, keyframe animation, or rigging
- Live actor wears HMD with AIFace mesh tracked on top of their face (like a mask)
- NVIDIA ACE determines facial expressions based on:
  - Audio track (speech recognition)
  - NLU (natural language understanding)
  - Emotion detection
  - State machine context
- `UAIFacemaskFaceController` receives NVIDIA ACE output and applies it to mesh in real-time

### Live Actor Controls
- Live actors wear wrist-mounted button controls (4 buttons: 2 left, 2 right)
- Buttons control the Experience Loop state machine (NOT facial animation)
- Live actor directs experience flow, AI face handles expressions autonomously

**Button Layout:**
- Left Wrist:  Button 0 (Forward), Button 1 (Backward)
- Right Wrist: Button 2 (Forward), Button 3 (Backward)

## Components

### UAIFacemaskFaceController
Receives and applies NVIDIA ACE facial animation output to a live actor's HMD-mounted mesh.

### UAIFacemaskScriptManager
Manages pre-baked script collections for AI facemask performances. Automatically triggers scripts when narrative states change.

**Inherits from:** `UAIScriptManager` (generic script management from HoloCadeAI module)

### UAIFacemaskImprovManager
Handles real-time improvised responses for AI-facemasked actors. Enables player-to-AI conversations.

**Inherits from:** `UAIImprovManager` (generic LLM + TTS + Audio2Face pipeline from HoloCadeAI module)

### UAIFacemaskASRManager
Converts player voice to text using Automatic Speech Recognition (ASR).

**Inherits from:** `UAIASRManager` (generic ASR functionality from HoloCadeAI module)

### UAIFacemaskLiveActorHUDComponent
Creates and manages the live actor's VR HUD overlay, displaying narrative and improv information to help guide the performance.

## Integration with HoloCadeAI Module

The AIFacemask experience uses the generic `HoloCadeAI` module for low-level AI capabilities:
- LLM providers (Ollama, OpenAI-compatible, NVIDIA NIM)
- ASR providers (NVIDIA Riva, Parakeet, Canary, Whisper)
- TTS providers (NVIDIA Riva, etc.)
- Audio2Face integration
- Container management for Docker-based AI services
- HTTP/gRPC clients for AI service communication

The facemask-specific subclasses extend the generic base classes to add:
- Narrative state machine integration
- Face controller integration
- Experience-specific script structures
- Experience-specific delegates and events

## Usage

Perfect for interactive theater, escape rooms, and narrative-driven LBE experiences requiring professional performers to guide players through story beats.

## Future Enhancements

### Phase 11: Improv-to-Narrative Transition Optimization

**Goal:** Optimize transitions between improv responses and narrative script lines to eliminate latency and provide smooth, context-aware dialogue flow.

#### Key Features

1. **LLM Prompt Context Optimization**
   - Ensure LLM generates appropriately-sized responses (short sentences, avoiding single words or run-on paragraphs)
   - Add prompt context to `UAIFacemaskImprovManager` that instructs LLM:
     - Aim for short, complete sentences (1-2 sentences max)
     - Avoid single-word responses
     - Avoid run-on sentences or multi-sentence paragraphs
     - Keep responses conversational and contextually appropriate

2. **HUD Display Logic**
   - Display both current improv response and narrative target sentence with proper ordering
   - **Display Rules:**
     - **Current improv response**: Always displayed (if active)
     - **Narrative target sentence**: Display based on spoken state:
       - **If target sentence already spoken** → Display ABOVE improv response
       - **If target sentence not yet spoken** → Display BELOW improv response

3. **Narrative State Tracking**
   - Track whether each narrative state's script line has been spoken
   - Add `bHasBeenSpoken` flag to `FAIFacemaskScriptLine` struct
   - Mark script lines as spoken when they complete
   - Allow retreating to already-spoken states (but don't re-speak them)

4. **Transition Logic & Buffering**
   - Eliminate latency by pre-buffering transition sentences
   - **Scenarios:**
     - **Scenario A:** Current state's sentence NOT spoken, improv active
       - LLM immediately starts calculating transition sentence
       - Purpose: Connect improv response to upcoming narrative sentence
     - **Scenario B:** Current state's sentence ALREADY spoken, improv begins
       - LLM immediately starts buffering next state's transition
       - Purpose: If actor advances, transition is ready
     - **Scenario C:** Improv active, actor advances to next state (unspoken sentence)
       - Transition sentence (if buffered) plays immediately, then narrative sentence
       - Purpose: Smooth transition from improv to narrative

5. **Transition Class/Methods**
   - Add transition methods to existing managers:
     - `UAIFacemaskImprovManager::RequestTransitionSentence(FName FromState, FName ToState)`
     - `UAIFacemaskImprovManager::GetBufferedTransition(FName TargetState)`
     - `UAIFacemaskScriptManager::NotifyImprovManagerOfStateChange(FName OldState, FName NewState)`

#### Implementation Order

1. **Narrative State Tracking** (Foundation)
   - Add `bHasBeenSpoken` to script line struct
   - Implement tracking in ScriptManager

2. **LLM Prompt Context** (Foundation)
   - Add prompt context to ImprovManager
   - Test response quality

3. **Transition Logic** (Core Feature)
   - Implement transition buffering in ImprovManager
   - Wire state change notifications from ScriptManager

4. **HUD Integration** (UI)
   - Update HUD to display improv + narrative with correct ordering
   - Wire to ScriptManager and ImprovManager for updates

5. **Integration & Testing** (Polish)
   - Test all transition scenarios
   - Verify latency elimination
   - Polish HUD display

#### Key Classes to Modify

1. **`FAIFacemaskScriptLine`** (HoloCadeAI module)
   - Add `bHasBeenSpoken` flag

2. **`UAIFacemaskScriptManager`** (HoloCadeExperiences module)
   - Track spoken state
   - Notify ImprovManager of state changes
   - Check spoken state before playback

3. **`UAIFacemaskImprovManager`** (HoloCadeExperiences module)
   - Add prompt context
   - Implement transition buffering
   - Handle transition generation

4. **`UAIFacemaskLiveActorHUDComponent`** (HoloCadeExperiences module)
   - Display logic for improv + narrative
   - Ordering based on spoken state

#### Testing Scenarios

1. **Improv → Narrative (unspoken)**: Verify transition sentence plays, then narrative
2. **Improv → Narrative (spoken)**: Verify narrative doesn't re-play, transition is smooth
3. **Narrative → Improv**: Verify HUD shows narrative above if spoken, below if not
4. **State Advance During Improv**: Verify buffered transition plays immediately
5. **State Retreat**: Verify already-spoken states don't re-play
6. **Latency Measurement**: Verify transitions happen with minimal delay (<200ms target)

