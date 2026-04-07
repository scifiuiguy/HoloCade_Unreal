# HoloCade ProLighting

ProLighting provides hardware‑agnostic DMX lighting control for HoloCade using either USB DMX interfaces (stubbed) or Art‑Net over UDP. The design favors a small orchestrator with focused services for transport, discovery, fixtures, and fades.

## Current Architecture

- Orchestrator
  - `UProLightingController` (ActorComponent)
    - Holds configuration and composes services
    - Ticks fades and discovery (via services)
    - Flushes DMX buffers to the active transport
    - Bridges service native events to Blueprint delegates for UMG

- Services (non‑UObject)
  - `FFixtureService`
    - Owns fixture ops and state interactions
    - Dependencies: `FUniverseBuffer`, `FFixtureRegistry`, `FFadeEngine`
    - High‑level APIs by virtual ID: `SetIntensityById`, `SetColorRGBWById`, `SetChannelById`, `StartFadeById`, `AllOffAndNotify`
    - Emits native events: `OnIntensityChanged`, `OnColorChanged` (controller forwards to Blueprint)
  - `FArtNetManager`
    - Consolidates Art‑Net transport and discovery in one class
    - Uses `FArtNetTransport` (send DMX) and internal discovery socket (auto‑poll ArtPoll / parse ArtPollReply)
    - Exposes `OnNodeDiscovered` and `GetDiscoveredArtNetNodes()`
  - `FRDMService`
    - Tracks discovered RDM fixtures (add/update/online/offline/prune)
    - Native events: `OnFixtureDiscovered`, `OnFixtureWentOffline`, `OnFixtureCameOnline`
    - RDM packet IO is stubbed for now

- Utilities
  - `FUniverseBuffer`: per‑universe 512‑byte DMX buffers
  - `FFixtureRegistry`: register/unregister and lookup of `FHoloCadeDMXFixture`
  - `FFadeEngine`: time‑based intensity fades per virtual fixture
  - `IFixtureDriver` + drivers (Dimmable, RGB, RGBW, MovingHead, Custom)

- Transports
  - `FArtNetTransport` (working): UDP socket send of ArtDmx packets
  - `FUSBDMXTransport` (stub): placeholder; not yet sending on serial
  - `IDMXTransport`: interface for transports

## Data Types

- `FHoloCadeDMXFixture`: virtual fixture definition (type, DMX address, universe, channel count, etc.)
- `FHoloCadeArtNetNode`: discovered Art‑Net node metadata
- `FHoloCadeDiscoveredFixture`: discovered RDM fixture metadata

## Event Flow

- UI (UMG) binds to controller Blueprint events:
  - `OnFixtureIntensityChanged(int32 Id, float Intensity)`
  - `OnFixtureColorChanged(int32 Id, float R, float G, float B)`
  - `OnFixtureWentOffline(int32 Id)` / `OnFixtureCameOnline(int32 Id)`
  - `OnFixtureDiscovered(FHoloCadeDiscoveredFixture)`
  - `OnArtNetNodeDiscovered(FHoloCadeArtNetNode)`
- Controller forwards from services’ native events to these Blueprint delegates.

## Usage (C++)

```cpp
// Acquire service and operate via virtual fixture IDs
if (FFixtureService* Svc = Controller->GetFixtureService())
{
    Svc->SetIntensityById(/*VirtualID*/ 1, /*Intensity*/ 0.75f);
    Svc->SetColorRGBWById(1, 1.0f, 0.5f, 0.2f, -1.0f); // white disabled with -1.0f
    Svc->StartFadeById(1, 0.0f, 2.0f); // fade to black in 2s
}
```

## What’s Implemented

- Controller orchestration with services composition
- Art‑Net transport and node discovery (auto‑polling)
- Fixture registry/validation, universe buffers, drivers, and fades
- Event bridging to UMG
- USB DMX transport stub

## Pending / Next Steps

- USB DMX: implement ENTTEC/Open DMX serial protocols (replace stub)
- RDM: build/parse packets, discovery tree, and readback for true bidirectional sync
- sACN (E1.31) transport
- Harden ArtPoll/Reply parsing (port tables, goodinput/output)
- Threading model and synchronization around sockets/buffers
- Persist configuration and discovered mappings
- Unit/integration tests for transports and discovery

## Design Principles

- Keep `UProLightingController` thin; push logic into focused service classes
- Prefer composition over inheritance; use interfaces for transport abstraction
- Emit native events in services; forward to Blueprint in the controller
- Minimize coupling between UI and low‑level DMX/Art‑Net code




