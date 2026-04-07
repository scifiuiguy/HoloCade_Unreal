# KiCAD Research & Workflow Guide

This document consolidates all KiCAD-related research, setup notes, troubleshooting tips, and design decisions for the HoloCade Universal Shield. Use it as a single reference when working on schematics, footprints, or PCB layout.

---

## 1. Project Overview

- **Project root:** `HoloCade_Unreal/Plugins/HoloCade/FirmwareExamples/PCBs/HoloCade_Universal_Shield/`
- **Primary files:** `HoloCade_Universal_Shield.kicad_pro`, `.kicad_sch`, `.kicad_pcb`
- **Recommended KiCAD:** Version 8.0 or later (installs modern libraries & UI)
- **Folder structure:**
  ```
  Universal_Shield/
    ├─ HoloCade_Universal_Shield.kicad_pro
    ├─ HoloCade_Universal_Shield.kicad_sch
    ├─ HoloCade_Universal_Shield.kicad_pcb
    ├─ README.md                      # design spec + BOM
    └─ Research/                      # this file + CAN_Research
  ```
- **Goal:** Build a universal shield that hosts ESP32-S3/STM32/Arduino parent ECUs and exposes 8 Ethernet-driven aux ports for child ECUs (Pi, Jetson, etc.).

---

## 2. EasyEDA → KiCAD Quick Start

| Topic                | EasyEDA                              | KiCAD                                             |
|----------------------|--------------------------------------|---------------------------------------------------|
| UI                   | Web-based single window              | Desktop app with separate schematic / PCB windows |
| Workflow             | Schematic + PCB in one scene         | Project Manager launches Eeschema / Pcbnew        |
| Libraries            | Cloud search                         | Local + managed libs (+ optional downloads)       |
| Component placement  | Drag from left panel                 | `Add Symbol` (`A`) → Library Browser              |
| Autorouting          | Built-in                             | External plugins; manual routing preferred        |

**First launch checklist**
1. Install KiCAD 8 with official symbol/footprint packs.
2. `File → Open Project` → select `HoloCade_Universal_Shield.kicad_pro`.
3. Confirm libraries under `Preferences → Manage Symbol/Footprint Libraries`.
4. Keep Project Manager open; double-click `.kicad_sch` for schematic work.

---

## 3. Recommended Schematic Workflow

1. **Phase 1 – Power Section**
   - Place barrel jack (J1), LM2576 buck converter, optional 3.3 V LDO.
   - Add decoupling caps near regulators and connectors.
   - Use net labels `+12V`, `+5V`, `+3V3`, `GND`.

2. **Phase 2 – MCU & Ethernet Core**
   - ESP32-S3 44-pin stacking header (socket + through-hole footprints).
   - LAN8720A PHY + Adam Tech MTJ-883X1 main RJ45.
   - Label management nets `MDC`, `MDIO`, `RESET_N`.

3. **Phase 3 – Aux Ports & Special IO**
   - 8× RJ45 aux connectors (pins 1‑3/6 Ethernet, pin 4 +5 V, pin 5 GND, pin 7 ADC, pin 8 PWM).
   - CAN connector (3-pin) and ADM3057E placement.
   - E-stop connector with RC filter (10 kΩ pull-up + 100 nF to GND).

4. **Phase 4 – Indicators & Test Points**
   - Power LED (red), Ethernet Link LED (green), Ethernet Speed LED (green) with 270 Ω resistors.
   - Add test pads for key rails (12 V, 5 V, 3.3 V, CAN).

5. **Verification**
   - `Tools → Annotate Schematic`.
   - `Tools → Electrical Rules Check (ERC)` after each section.
   - Assign footprints before generating netlist.

---

## 4. Component & Library Strategy

### 4.1 Use Generic Placeholders First

| Target Part                      | Temporary Symbol                                | Notes                                           |
|----------------------------------|-------------------------------------------------|-------------------------------------------------|
| Kycon KLDVHCX-0202-A-LT (J1)     | `Connector:Conn_01x02_Male` or generic DC jack  | swap later; focus on nets                       |
| Adam Tech MTJ-883X1 (RJ45)       | `Connector:RJ45`                                | assign custom footprint before layout           |
| Molex 0444320402 (aux headers)   | `Connector_PinHeader_2.54mm:PinHeader_1x04`     | actual pitch 3 mm → adjust footprint later      |
| LAN8720A PHY                     | Generic 32-pin QFN                              | create custom symbol for correct pin names      |
| LM2576 buck                      | Generic TO-220 regulator                        | add custom symbol/footprint for final revision  |

**Why:** Connectivity matters more than perfect graphics; annotate, run ERC, then replace generics with exact parts before layout.

### 4.2 Creating Custom Symbols

1. Open **Symbol Editor** → `File → New Symbol`.
2. Choose/create project library (e.g., `HoloCade_Custom`).
3. Add pins per datasheet (name, number, electrical type).
4. Fill metadata (Reference, Value, Footprint link, Datasheet URL).
5. Save and re-open schematic; components appear in `Add Symbol`.

### 4.3 Footprint Creation / Import

- Use **Footprint Editor** with datasheet dimensions (pad pitch, drill size, courtyard).
- For connectors (RJ45, barrel jack), include mechanical outlines and mounting tabs.
- You can import vendor footprints from SnapEDA / Ultra Librarian; add via `Manage Footprint Libraries`.
- Keep project-specific libraries inside `Research/` or `KiCADv6/` for portability.

---

## 5. JST & Connector Guidance

### 5.1 Pitch Reality

- **JST XH:** 2.50 mm pitch (0.098 in) — NOT 2.54 mm.
- **JST EH:** Also 2.50 mm pitch and **already in KiCAD default libraries**.
- **Standard pin headers:** 2.54 mm (0.1 in) — acceptable fallback when JST libs missing.

### 5.2 Recommended Approach

1. **If JST libraries missing:** temporarily use `PinHeader_1x03_P2.54mm` for the CAN connector. Works immediately.
2. **Install JST libraries:** `Preferences → Manage Libraries → Add` `Connector_JST` (symbols + footprints) or download from SnapEDA (e.g., B3B-XH-A).
3. **Use JST EH for CAN connector:** available by default, 2.5 mm pitch, reliable for wire-to-board.

### 5.3 Physical Compatibility

- JST EH and XH have same pitch but different keying; do not mix housings.
- For new builds, standardize on EH or XH and keep BOM consistent.
- For exact 2.54 mm spacing, stick to pin headers or Molex KK series.

---

## 6. Library Troubleshooting

| Symptom                            | Likely Cause                          | Fix                                                                 |
|------------------------------------|---------------------------------------|---------------------------------------------------------------------|
| Searching “JST” returns nothing    | JST library not installed             | Add `Connector_JST` library or download from SnapEDA                |
| Standard symbols missing           | Libraries disabled                    | `Preferences → Manage Libraries` → ensure “Enabled” is checked      |
| ERC reports “power pin not driven” | Missing power flag                    | Place `PWR_FLAG` symbol on rails driven by regulators               |
| ERC “unconnected pin” warnings     | Intentional NC pins not marked        | Use the “No Connect” (X) symbol on unused pins                      |
| Footprint mismatch                 | Using placeholder symbol              | Assign correct footprint before generating netlist                  |
| KiCAD can’t find downloaded libs   | Path not added                        | Add custom library folder in Manage Libraries (Symbol + Footprint)  |

**Pro tip:** If all else fails, drop in standard pin headers so schematic work can continue, then swap to custom connectors later.

---

## 7. Power Architecture Research (Summary)

### 7.1 Options Evaluated

1. **Shield regulates everything** — simple but redundant, more heat/cost.
2. **MCU provides 3.3 V (Selected)** — shield only has buck → 5 V; adapters supply 3.3 V logic.
3. **Hybrid** — shield powers itself, MCU powers itself; more parts for little gain.
4. **Configurable (jumpers)** — flexible but complicated, easier to misconfigure.

### 7.2 Selected Approach (Option 2)

```
12V/24V → LM2576 buck (60 V max input) → 5 V bus
    ├─→ MCU VIN via adapter
    │     └─→ MCU onboard LDO → 3.3 V → shield logic (LAN8720A, LEDs, ADM3057E logic)
    └─→ Aux port pin 4 (+5 V, ≥5 A total across 8 ports)
```

- **High-current MCUs (ESP32-S3, STM32F407):** adapter is just pin mapping; MCU 3.3 V output feeds shield.
- **Low-current MCUs (Arduino):** adapter includes small AMS1117-3.3 regulator powered from shield 5 V.
- **Child ECUs (Pi/Jetson):** always treated as Ethernet devices with their own power.

**Benefits:** lower part count, less heat, same shield for all MCUs, adapters handle edge cases.

---

## 8. Schematic Generation Notes

- Procedural generation already inserted net labels, section boundaries, and placeholders, but **symbols still need to be placed** in KiCAD.
- Use `README` + this guide for net naming:
  - Power: `+12V`, `+5V`, `+3V3`, `GND`
  - Ethernet: `MDC`, `MDIO`, `TXP`, `TXN`, `RXP`, `RXN`
  - Aux I/O: `AUX1_ADC` … `AUX8_ADC`, `AUX1_PWM` … `AUX8_PWM`
  - Control: `E_STOP`, `CANH`, `CANL`, `CAN_GND`
- Placement strategy:
  - **Top-left:** Power input + LM2576 + headers
  - **Top-right:** LAN8720A + main RJ45
  - **Center:** ESP32-S3 header and adapter interface
  - **Perimeter:** Aux RJ45s; keep Ethernet pairs short and consistent
  - **Near MCU:** ADM3057E CAN transceiver + JST/CAN connector, E-stop RC filter

---

## 9. PCB Layout Guidance (High Level)

- **Power traces:** use ≥2 mm width for 5 V aux rail (≥5 A total).
- **Ethernet:** treat TX/RX as differential pairs; match lengths from PHY to each RJ45.
- **CAN:** route CANH/CANL as short differential pair; CAN_GND trace always present (even if harness omits wire).
- **Grounding:** keep CAN isolated ground tied only at ADM3057E; join to main ground near PSU if required.
- **Potting:** connectors are vertical with 16.38 mm shells; maintain keep-outs for potting compound.
- **Testability:** add test pads for VIN, +5 V, +3.3 V, CAN signals, and key aux nets.

---

## 10. Reference Checklist

1. Install KiCAD + libraries (symbol, footprint, 3D).
2. Open project and schematic; verify libraries enabled.
3. Place generic symbols section by section; wire per spec.
4. Run ERC, annotate, fix warnings as you go.
5. Create/import custom symbols & footprints before finalizing.
6. Assign footprints → generate netlist → move to PCB editor.
7. Place components following placement guidance; route high-current first.
8. DRC + design review + export fabrication files.

---

## 11. Useful Shortcuts

| Action         | Shortcut | Notes                           |
|----------------|----------|---------------------------------|
| Add Symbol     | `A`      | Opens library browser           |
| Add Wire       | `W`      | Draw net; `Esc` to finish       |
| Add Net Label  | `L`      | Name nets instead of long wires |
| Move           | `M`      | Grab symbol/wire segment        |
| Rotate         | `R`      | Rotates selected part           |
| Delete         | `Del`    | Removes selection               |
| Properties     | `E`      | Edit component/net properties   |
| Zoom Fit       | `Ctrl+F` | Auto-fit schematic              |
| Annotate       | `Tools → Annotate` | Assign reference IDs |
| ERC            | `Tools → Electrical Rules Check` | Validate |

---

## 12. Additional Resources

- KiCAD Docs: https://docs.kicad.org/
- KiCAD Forum: https://forum.kicad.info/
- Official libraries: https://github.com/kicad/
- SnapEDA components: https://www.snapeda.com/
- Ultra Librarian: https://www.ultralibrarian.com/

**Need a connector footprint fast?** Search SnapEDA/Ultra Librarian for manufacturer part numbers (e.g., “B3B-XH-A”, “MTJ-883X1”), download KiCAD symbol/footprint, add via `Manage Libraries`.

---

## 13. Final Notes

- Focus on completing schematic connectivity even if symbols are placeholders.
- Keep README and KiCAD files in sync; README now holds canonical BOM/spec details.
- All exploratory docs were moved here or to `CAN_Research.md` to avoid duplication.
- If a library hiccup blocks progress, drop in a generic pin header, document the placeholder, and keep moving.

Happy routing! 🚀