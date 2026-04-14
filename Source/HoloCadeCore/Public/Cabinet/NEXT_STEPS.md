# HoloCade Cabinet — next steps (Unreal)

This file tracks **intended SDK work** under HoloCadeCore `Cabinet`. It is not a commitment schedule.

## Diagnostics as a shared SDK surface

Operator / **QC diagnostics** for arcade cabinets should live **in the HoloCade SDK**, not only in individual games. Behavior is expected to be **very similar across cabinets** (switch tests, monitor patterns, sound, lamps/LEDs, version info, etc.); **title-specific** panels cover hardware that varies by game.

**Games** should stay thin: use `UArcadeCabinetBridge` / `UHoloCadeUDPTransport` (or `UHoloCadeUDPTransportSimple`) for I/O, add title-specific panels only where needed, and avoid duplicating a full QC stack per title.

## Future: Diagnostics GUI + boot entry

- **Diagnostics GUI (full interface):** A complete, SDK-owned UI (e.g. **UMG** or Slate-based shell) for **manual QC** of connected hardware—inputs, outputs, LEDs, solenoids, displays, audio, link status, firmware/SDK versions, and other checks appropriate to HoloCade UDP peers.
- **Optional boot / launcher integration:** Support launching this diagnostics experience from a **boot menu** or **service mode** entry (before full game attract, or as a standalone map) so venue ops and factory can run **hardware verification** without loading title-specific gameplay.
