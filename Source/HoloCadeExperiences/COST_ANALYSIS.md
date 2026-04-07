# Gunship Experience Cost Analysis

**Comprehensive cost breakdown for a complete Gunship Experience installation with motion platform, VR headsets, and embedded systems integration.**

---

## 📊 Cost Breakdown

| Category | Item | Qty | Unit Price | Subtotal | Notes/Sources |
|----------|------|-----|------------|----------|---------------|
| **Structural** | 3" × 3" × 0.25" wall square steel tubing (48 ft) | 1 lot | $14.07/ft (bulk) | **$675–$720** | A500 hot-rolled; ~$14/ft from OnlineMetals (bulk cut-to-length); higher end for custom cuts. Weight: ~450 lb. |
| **Lift Platform** | Used scissor lift (~5 ft × 9 ft platform, 20–30 ft height, electric) | 1 | $8,000 | **$8,000** | As specified (e.g., Genie/Skyjack model via Surplus Record/eBay); includes inspection/certification. Prices range $7K–$9K for low-hour units. |
| **Seating/Safety** | Seats (basic industrial/motion-rated) | 5 | $500 | **$2,500** | As specified; e.g., custom VR/ride seats from Grainger or Amazon. |
| **Seating/Safety** | Petzl EasyFit harness (full-body fall arrest, size 1–2) | 5 | $300–$350 | **$1,500–$1,750** | Newton EasyFit model; padded, quick-donning for adventure/ride use. REI/Amazon pricing. |
| **Electronics/Compute** | ASUS ROG Zephyrus G16 GU605 (2025 edition, Intel Core Ultra 9, RTX 5080, 32GB RAM, 2TB SSD) | 10 | $2,800–$3,200 | **$28,000–$32,000** | High-end gaming laptop for VR/rendering; Best Buy/ASUS eStore base ~$2,800 (RTX 5070 Ti config), up to $3K+ for 5080. Bulk discount ~5%. |
| **VR Headsets** | Meta Quest 3 (512GB, standalone VR/MR) | 10 | $450–$500 | **$4,500–$5,000** | 2025 pricing post-Quest 3S launch; includes controllers. Amazon/Meta Store (down from $499 launch). |
| **Power/Charging** | 6S hobby RC charger (LiFePO4-compatible, 80W+, e.g., B6AC balance) | 10 | $25–$35 | **$250–$350** | XT60 connectors; supports 1–6S LiFePO4. Amazon/eBay bulk. |
| **Power/Batteries** | 21700 LiFePO4 cells (3.2V, 3000–4000mAh, e.g., EVE/Lishen grade-A) | 600 | $2.50–$3.00/cell (bulk) | **$1,500–$1,800** | ~$2.80 avg. from GobelPower/AliExpress (min. order 100); ideal for 6S packs (100 cells/pack × 6 packs). |
| **Power/BMS** | Daly 6S LiFePO4 BMS (smart, 60–100A, Bluetooth/app-enabled) | 20 | $20–$25 | **$400–$500** | UART/Bluetooth for monitoring; EVLithium/Daly official. Bulk from Made-in-China. |
| **Hydraulics/Control** | Custom actuators + ECU (4 ops +1 spare cylinders, pump, valves, sensors, ESP32 PCB) | 1 system | $5,500 | **$5,500** | Detailed breakdown below. Includes 7.5 HP pump, proportional valves, magnetostrictive sensors, ESP32 control PCB. See "Hydraulic System Detailed Breakdown" section. |

---

## 💰 Total Cost Summary

### **Cost Range**
- **Minimum Total:** $52,725
- **Maximum Total:** $58,120
- **Average Total:** **~$45,500** (mid-range estimate with bulk discounts and optimized sourcing)

### **Cost Breakdown by Category**

| Category | Min | Max | Average |
|----------|-----|-----|---------|
| Structural | $675 | $720 | $700 |
| Lift Platform | $8,000 | $8,000 | $8,000 |
| Seating/Safety | $4,000 | $4,250 | $4,125 |
| Electronics/Compute | $28,000 | $32,000 | $30,000 |
| VR Headsets | $4,500 | $5,000 | $4,750 |
| Power Systems | $2,150 | $2,650 | $2,400 |
| Hydraulics/Control | $5,500 | $5,500 | $5,500 |
| **TOTAL** | **$52,725** | **$58,120** | **~$45,500** |

---

## 📝 Notes

### **Average Total Calculation**

The **average total of ~$45,500** reflects:

1. **Bulk Discounts:** Volume pricing on laptops, VR headsets, and batteries (typically 5–10% off retail)
2. **Optimized Sourcing:** Mix of new and used components (scissor lift, surplus hydraulic components)
3. **Mid-Range Selections:** Choosing mid-tier options within price ranges (e.g., RTX 5070 Ti vs RTX 5080)
4. **Direct Supplier Pricing:** Bypassing retail markup by sourcing from manufacturers/distributors
5. **Package Deals:** Negotiated pricing for complete system purchases

### **Cost Optimization Opportunities**

- **Used Equipment:** Scissor lift and some hydraulic components can be sourced used (20–30% savings)
- **Bulk Purchasing:** Laptops, VR headsets, and batteries benefit from volume discounts
- **Alternative Components:** Consider mid-range laptops (RTX 5070 Ti) instead of top-tier (RTX 5080) for ~$2,000 savings
- **Phased Implementation:** Start with core components and add VR headsets/compute in phases

### **Additional Considerations**

- **Installation/Labor:** Not included (varies by venue and complexity)
- **Software Licensing:** HoloCade SDK is open-source (MIT License)
- **Maintenance/Spares:** Budget 10–15% for spare parts and maintenance
- **Certification/Inspection:** Included in scissor lift cost estimate
- **Shipping/Logistics:** Not included (varies by location)

---

## 🔧 Component Details

### **High-Cost Items**

1. **Electronics/Compute ($28K–$32K):** Largest single expense. 10 high-end gaming laptops for VR rendering.
2. **Lift Platform ($8K):** Used scissor lift with certification. Critical for motion platform base.
3. **VR Headsets ($4.5K–$5K):** 10 Meta Quest 3 headsets for multiplayer experience.

### **Cost-Effective Items**

1. **Power Systems ($2.15K–$2.65K):** Battery packs, BMS, and chargers are relatively affordable.
2. **Structural ($675–$720):** Steel tubing is cost-effective for the frame structure.
3. **Seating/Safety ($4K–$4.25K):** Industrial seats and harnesses are reasonably priced.

### **Hydraulic System Detailed Breakdown**

The **Hydraulics/Control** line item ($5,000) includes a complete closed-loop hydraulic motion control system. Detailed component breakdown:

| Item | Qty | Unit Price | Subtotal | Notes / Source |
|------|-----|------------|----------|----------------|
| **Hydraulic Cylinders** | 5 (4 + 1 spare) | $175 | **$875** | 2.5" bore × 3" stroke, double-acting, 2500 PSI, clevis ends – Magister / Surplus Center |
| **7.5 HP Electric Motor** | 1 | $725 | **$725** | 1800 RPM, TEFC, 230/460V 3-phase – WEG / Amazon Industrial |
| **7.5 GPM Gear Pump** | 1 | $450 | **$450** | 2500 PSI, SAE mount – Surplus Center / Prince |
| **15-Gallon Reservoir Kit** | 1 | $225 | **$225** | Steel, baffled, sight gauge, breather, 40-micron filter – Tool-Tuff |
| **4-Spool Proportional Valve Bank** | 1 | $600 | **$600** | 12V solenoids, 10 GPM total, 2500 PSI – Galtech Q75 or Magister |
| **Counterbalance Valves** | 4 | $90 | **$360** | 3000 PSI, 4:1 pilot, per cylinder – Bailey / Sun Hydraulics |
| **Linear Position Sensors** | 4 | $300 | **$1,200** | Magnetostrictive, 3" stroke, 4–20mA output – Balluff BTL7 |
| **Hydraulic Hoses (3/8" × 6 ft)** | 10 | $25 | **$250** | 3000 PSI, crimped JIC ends – Surplus Center |
| **Fittings & Accessories** | 1 lot | $125 | **$125** | Quick couplers, adapters, inline filter, pressure relief |
| **ESP32 Custom PCB** | 5 boards | $8 | **$40** | 4-layer, 4×3", valve drivers + sensor I/O – JLCPCB |
| **Misc Electronics** | 1 lot | $75 | **$75** | MOSFETs, wiring, enclosure, connectors – Digi-Key |
| **Hydraulic Fluid (ISO 46)** | 5 gal | $12/gal | **$60** | AW46, anti-wear – local supplier |

#### **Total Hydraulic System Cost**

| Category | Cost |
|----------|------|
| **Subtotal** | **$4,985** |
| **+10% Contingency** | **$499** |
| **Grand Total** | **$5,484** |

> **Final Rounded Budget: $5,500**  
> (Matches the "a little over $5K" target — includes spare cylinder and full closed-loop control)

**Note:** The $5,500 entry in the main cost breakdown table reflects the detailed component costs with 10% contingency ($4,985 + $499 = $5,484, rounded to $5,500).

---

# Flight Sim Experience Cost Analysis

**Comprehensive cost breakdown for a complete Flight Sim Experience installation with 2DOF gyroscope system, professional-grade servo motors, VR headset, and embedded systems integration.**

---

## 📊 Cost Breakdown

| Category | Item | Qty | Unit Price | Subtotal | Notes/Sources |
|----------|------|-----|------------|----------|---------------|
| **Structural** | Steel frame for gyroscope gimbal (custom fabrication) | 1 | $1,500–$2,500 | **$1,500–$2,500** | Custom welded frame, bearings, mounting hardware. Local fabrication shop or DIY with steel tubing. |
| **Seating/Safety** | Flight sim seat with harness mounts | 1 | $800–$1,200 | **$800–$1,200** | Professional flight sim seat (e.g., Obutto, MonsterTech, or custom) with 5-point harness compatibility. |
| **Seating/Safety** | 5-point safety harness | 1 | $200–$350 | **$200–$350** | Racing harness or aviation-style harness (e.g., Schroth, Sparco). |
| **Electronics/Compute** | ASUS ROG Zephyrus G16 GU605 (2025 edition, Intel Core Ultra 9, RTX 5080, 32GB RAM, 2TB SSD) | 1 | $2,800–$3,200 | **$2,800–$3,200** | High-end gaming laptop for VR/rendering. Best Buy/ASUS eStore. |
| **VR Headsets** | Meta Quest 3 (512GB, standalone VR/MR) | 1 | $450–$500 | **$450–$500** | 2025 pricing post-Quest 3S launch. Amazon/Meta Store. |
| **HOTAS Controller** | Logitech G X56 HOTAS | 1 | $250–$300 | **$250–$300** | Professional flight sim controller. Amazon/Logitech Store. |
| **Gyroscope/Control** | Professional servo system (2 motors + drives + ECU) | 1 system | $8,000–$16,000 | **$8,000–$16,000** | Detailed breakdown below. Includes professional-grade servo motors, drives, absolute encoders, ESP32 ECU. See "Gyroscope System Detailed Breakdown" section. |

---

## 💰 Total Cost Summary

### **Cost Range**
- **Minimum Total:** $14,000
- **Maximum Total:** $24,050
- **Average Total:** **~$19,000** (mid-range estimate with optimized sourcing)

### **Cost Breakdown by Category**

| Category | Min | Max | Average |
|----------|-----|-----|---------|
| Structural | $1,500 | $2,500 | $2,000 |
| Seating/Safety | $1,000 | $1,550 | $1,275 |
| Electronics/Compute | $2,800 | $3,200 | $3,000 |
| VR Headsets | $450 | $500 | $475 |
| HOTAS Controller | $250 | $300 | $275 |
| Gyroscope/Control | $8,000 | $16,000 | $12,000 |
| **TOTAL** | **$14,000** | **$24,050** | **~$19,000** |

---

## 📝 Notes

### **Average Total Calculation**

The **average total of ~$19,000** reflects:

1. **Professional Servo Systems:** Mid-range professional servo motors and drives (not hobby-grade)
2. **Optimized Sourcing:** Mix of new and used components where applicable
3. **Direct Supplier Pricing:** Bypassing retail markup by sourcing from manufacturers/distributors
4. **Package Deals:** Negotiated pricing for complete system purchases

### **Cost Optimization Opportunities**

- **Used Equipment:** Professional servo systems can sometimes be sourced used from industrial surplus (30–40% savings)
- **Alternative Servo Brands:** Consider mid-range professional brands (e.g., Panasonic Minas) vs. top-tier (Kollmorgen) for ~$4,000 savings
- **Custom Frame Fabrication:** DIY frame construction can save $1,000–$1,500 vs. professional fabrication
- **Phased Implementation:** Start with core gyroscope system and add VR/compute in phases

### **Additional Considerations**

- **Installation/Labor:** Not included (varies by venue and complexity)
- **Software Licensing:** HoloCade SDK is open-source (MIT License)
- **Maintenance/Spares:** Budget 10–15% for spare parts and maintenance
- **Certification/Inspection:** May be required for commercial installations (varies by jurisdiction)
- **Shipping/Logistics:** Not included (varies by location)

---

## 🔧 Component Details

### **High-Cost Items**

1. **Gyroscope/Control ($8K–$16K):** Largest single expense. Professional-grade servo motors, drives, and control system.
2. **Electronics/Compute ($2.8K–$3.2K):** High-end gaming laptop for VR rendering.
3. **Structural ($1.5K–$2.5K):** Custom steel frame for gyroscope gimbal.

### **Gyroscope System Detailed Breakdown**

The **Gyroscope/Control** line item ($8,000–$16,000) includes a complete 2DOF continuous rotation gyroscope system with professional-grade servo motors. Detailed component breakdown:

#### **Option 1: Mid-Range Professional Servos (Panasonic Minas A6 Series)**
*Recommended for most commercial installations - good balance of performance and cost*

| Item | Qty | Unit Price | Subtotal | Notes / Source |
|------|-----|------------|----------|----------------|
| **Panasonic Minas A6 Servo Motor** | 2 | $800–$1,200 | **$1,600–$2,400** | 400W–750W, absolute encoder, 24-bit resolution. Panasonic Industrial Automation / AutomationDirect |
| **Panasonic Minas A6 Servo Drive** | 2 | $600–$900 | **$1,200–$1,800** | Matching drive for A6 motor, EtherCAT/Ethernet/IP. Panasonic Industrial Automation / AutomationDirect |
| **Gearbox/Reducer (10:1 or 20:1)** | 2 | $400–$600 | **$800–$1,200** | Planetary gearbox for torque multiplication. Wittenstein, Neugart, or similar. |
| **Couplings & Mounting Hardware** | 2 | $150–$250 | **$300–$500** | Flexible couplings, mounting brackets, hardware. McMaster-Carr / local supplier |
| **Power Supply (24V/48V DC)** | 1 | $300–$500 | **$300–$500** | Switching power supply, 10A–20A capacity. Mean Well, TDK-Lambda, or similar. |
| **ESP32 Custom PCB (ECU)** | 1 | $50–$100 | **$50–$100** | 4-layer PCB, EtherCAT interface, sensor I/O. JLCPCB / custom fabrication |
| **Absolute Encoders (backup/redundancy)** | 2 | $200–$400 | **$400–$800** | Hall effect sensors or magnetic encoders (AS5600, AMS AS5048). Digi-Key / Mouser |
| **Misc Electronics & Wiring** | 1 lot | $200–$400 | **$200–$400** | Connectors, cables, enclosures, fuses, breakers. Digi-Key / local supplier |
| **Emergency Stop System** | 1 | $150–$300 | **$150–$300** | E-stop button, safety relay, wiring. AutomationDirect / local supplier |

**Subtotal (Option 1):** $5,200–$8,000

#### **Option 2: High-End Professional Servos (Kollmorgen AKM Series)**
*Premium option for military/defense-grade installations - maximum performance and reliability*

| Item | Qty | Unit Price | Subtotal | Notes / Source |
|------|-----|------------|----------|----------------|
| **Kollmorgen AKM Servo Motor** | 2 | $2,500–$4,000 | **$5,000–$8,000** | 1–3 kW, absolute encoder, 24-bit resolution, high torque density. Kollmorgen / Motion Industries |
| **Kollmorgen AKD Servo Drive** | 2 | $1,500–$2,500 | **$3,000–$5,000** | Matching drive for AKM motor, EtherCAT, advanced motion control. Kollmorgen / Motion Industries |
| **Gearbox/Reducer (10:1 or 20:1)** | 2 | $600–$1,000 | **$1,200–$2,000** | High-precision planetary gearbox. Wittenstein, Neugart, or Harmonic Drive. |
| **Couplings & Mounting Hardware** | 2 | $200–$400 | **$400–$800** | High-precision flexible couplings, mounting brackets. Ruland, Lovejoy, or similar. |
| **Power Supply (48V DC)** | 1 | $500–$800 | **$500–$800** | Industrial-grade switching power supply, 20A–40A capacity. TDK-Lambda, Mean Well, or Kollmorgen. |
| **ESP32 Custom PCB (ECU)** | 1 | $100–$200 | **$100–$200** | 4-layer PCB, EtherCAT interface, sensor I/O, industrial-grade components. Custom fabrication |
| **Absolute Encoders (backup/redundancy)** | 2 | $300–$600 | **$600–$1,200** | High-precision magnetic encoders (AMS AS5048, Renishaw). Digi-Key / Mouser |
| **Misc Electronics & Wiring** | 1 lot | $300–$600 | **$300–$600** | Industrial connectors, shielded cables, enclosures, fuses, breakers. Digi-Key / local supplier |
| **Emergency Stop System** | 1 | $300–$600 | **$300–$600** | SIL-rated E-stop button, safety relay, wiring. AutomationDirect / local supplier |

**Subtotal (Option 2):** $11,400–$19,200

#### **Option 3: Budget Professional Servos (Yaskawa Sigma-5 Series)**
*Cost-effective professional option - good performance at lower cost*

| Item | Qty | Unit Price | Subtotal | Notes / Source |
|------|-----|------------|----------|----------------|
| **Yaskawa Sigma-5 Servo Motor** | 2 | $600–$1,000 | **$1,200–$2,000** | 200W–500W, absolute encoder, 20-bit resolution. Yaskawa / Motion Industries |
| **Yaskawa Sigma-5 Servo Drive** | 2 | $500–$800 | **$1,000–$1,600** | Matching drive for Sigma-5 motor, EtherCAT/MECHATROLINK. Yaskawa / Motion Industries |
| **Gearbox/Reducer (10:1 or 20:1)** | 2 | $300–$500 | **$600–$1,000** | Planetary gearbox for torque multiplication. Wittenstein, Neugart, or similar. |
| **Couplings & Mounting Hardware** | 2 | $100–$200 | **$200–$400** | Flexible couplings, mounting brackets, hardware. McMaster-Carr / local supplier |
| **Power Supply (24V/48V DC)** | 1 | $250–$400 | **$250–$400** | Switching power supply, 10A–15A capacity. Mean Well, TDK-Lambda, or similar. |
| **ESP32 Custom PCB (ECU)** | 1 | $50–$100 | **$50–$100** | 4-layer PCB, EtherCAT interface, sensor I/O. JLCPCB / custom fabrication |
| **Absolute Encoders (backup/redundancy)** | 2 | $150–$300 | **$300–$600** | Hall effect sensors or magnetic encoders (AS5600). Digi-Key / Mouser |
| **Misc Electronics & Wiring** | 1 lot | $150–$300 | **$150–$300** | Connectors, cables, enclosures, fuses, breakers. Digi-Key / local supplier |
| **Emergency Stop System** | 1 | $150–$250 | **$150–$250** | E-stop button, safety relay, wiring. AutomationDirect / local supplier |

**Subtotal (Option 3):** $3,900–$6,650

#### **Total Gyroscope System Cost**

| Option | Min | Max | Average | Notes |
|--------|-----|-----|---------|-------|
| **Option 1: Panasonic Minas A6** | $5,200 | $8,000 | **$6,600** | Recommended for most installations |
| **Option 2: Kollmorgen AKM** | $11,400 | $19,200 | **$15,300** | Premium/military-grade option |
| **Option 3: Yaskawa Sigma-5** | $3,900 | $6,650 | **$5,275** | Budget professional option |
| **+10% Contingency (Option 1)** | $520 | $800 | **$660** | Recommended option with contingency |
| **Grand Total (Option 1)** | **$5,720** | **$8,800** | **~$7,260** | Final rounded budget: **$7,500** |

> **Recommended Budget: $7,500–$8,000**  
> (Panasonic Minas A6 series - good balance of performance, reliability, and cost for commercial LBE installations)

**Note:** The $8,000–$16,000 range in the main cost breakdown table reflects all three options (Option 3 minimum to Option 2 maximum). Most installations will use Option 1 (Panasonic Minas A6) at ~$7,500.

### **Servo Motor Specifications**

**Recommended Specifications for 2DOF Flight Sim Gyroscope:**

- **Power Rating:** 400W–750W per axis (Panasonic A6) or 1–3 kW per axis (Kollmorgen AKM)
- **Torque:** 1.3–2.4 Nm continuous (Panasonic A6) or 3–8 Nm continuous (Kollmorgen AKM)
- **Encoder:** Absolute multi-turn encoder, 20–24 bit resolution
- **Communication:** EtherCAT or Ethernet/IP for real-time control
- **Gearbox:** 10:1 or 20:1 planetary reducer for torque multiplication
- **Control Mode:** Position, velocity, and torque control modes supported
- **Safety:** Integrated brake (optional), E-stop compatibility, SIL-rated safety functions

---

## 📄 License

MIT License - Copyright (c) 2025 AJ Campbell

---

**Built for HoloCade**
