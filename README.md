# Parking Slot Management System Using LPC1768

A microcontroller-based parking slot management system developed using the **ARM Cortex-M3 LPC1768**. The system simulates an eight-slot parking facility using onboard LEDs, an LCD, and a 4×4 matrix keypad.

It allows a parking control room operator to manage slot availability, occupancy, and reservations through an interactive menu-driven interface.

## Features

- **8-Slot Simulation:** Uses eight onboard LEDs to represent individual parking slots and their occupancy status.
- **Slot Activation Management:** Enable or disable individual parking slots.
- **Occupancy Management:** Assign parking slots as vacant, occupied, or reserved.
- **Live Slot Statistics:** Displays vacant and occupied slot counts relative to the total number of active slots.
- **Interactive LCD Interface:** Provides four operational modes through a 16×2 LCD.
- **Configuration Management:** Supports saving modified configurations through the keypad.
- **Page Navigation:** Maintains a navigation history of up to five previously visited pages.
- **Reservation Indication:** Uses Timer0 to generate a periodic blinking output when at least one saved parking slot is reserved.

## Operating Modes

The system provides four operating modes, accessible through the first row of the matrix keypad.

### Mode A – Slot Activation / Deactivation

Allows the operator to enable or disable individual parking slots.

| State | Description |
|---|---|
| A | Active |
| I | Inactive |

- All slots are active by default.
- Individual slots can be toggled between active and inactive states using the keypad.
- Inactive slots cannot have their occupancy status modified.

### Mode B – Parking Availability Statistics

Displays the number of vacant and occupied slots relative to the total number of active parking slots.

```text
Vacant   : <count>/<active slots>
Occupied : <count>/<active slots>
```

- Read-only display mode.
- Supports vertical scrolling using the Up and Down keys.
- Slot modification keys are disabled in this mode.

### Mode C – Occupancy Status Management

Allows the operator to modify the occupancy status of active parking slots.

| State | Description |
|---|---|
| V | Vacant |
| O | Occupied |
| R | Reserved |
| X | Inactive |

- Active slots cycle through Vacant → Occupied → Reserved.
- Inactive slots are represented by X.
- All active slots are initially vacant.
- Occupancy changes are reflected across the other display modes.

### Mode D – Combined Slot Summary

Provides a consolidated view of slot activation and occupancy states.

```text
1 2 3 4 5 6 7 8
A I A I A A I A
V X O X R V X O
```

- First row displays the activation status.
- Second row displays the corresponding occupancy status.
- Read-only mode.

## Keyboard Mapping

The system uses a 4×4 matrix keypad for navigation and configuration.

| Key | Function |
|---|---|
| A | Mode A – Slot Activation |
| B | Mode B – Availability Statistics |
| C | Mode C – Occupancy Management |
| D | Mode D – Combined Summary |
| 1–8 | Modify the corresponding parking slot |
| # | Save configuration changes |
| ↑ | Scroll up in Mode B |
| ↓ | Scroll down in Mode B |
| * | Navigate to the previous page |

**Context-sensitive key behaviour:**
- Keys 1–8 are enabled only in Modes A and C.
- The Up and Down keys are enabled only in Mode B.
- The Save key is enabled when modifications have been made in Modes A or C.
- The previous-page key maintains a navigation history of up to five pages.

## Hardware and Peripheral Connections

The project uses the onboard peripherals of the LPC1768 evaluation board.

| Peripheral | LPC1768 Pins | Function |
|---|---|---|
| LEDs 1–8 | P0.4–P0.11 | Parking slot indication |
| Matrix Keypad | P1.23–P1.26, P2.10–P2.13 | User input and navigation |
| LCD Data (D4–D7) | P0.23–P0.26 | 4-bit LCD data interface |
| LCD RS | P0.27 | Register selection |
| LCD EN | P0.28 | LCD enable |
| Reservation Blink Output | P1.28 (MAT0.0) | Timer0-based periodic output |

### Reservation Indication

Timer0 is configured to generate a periodic output on P1.28 (MAT0.0). The output toggles every one second while at least one **saved** parking slot is in the Reserved state.

## Screenshots

The following screenshots demonstrate the system's LCD interface, parking slot indication, and different operating modes.

<!-- Replace the filenames below with your actual screenshot filenames. -->

### Mode A – Slot Activation
![Mode A - Slot Activation](images/mode-a.png)

### Mode B – Parking Availability Statistics
![Mode B - Parking Statistics](images/mode-b.png)

### Mode C – Occupancy Status
![Mode C - Occupancy Management](images/mode-c.png)

### Mode D – Combined Slot Summary
![Mode D - Combined Summary](images/mode-d.png)

### Hardware / System Output
![Parking Slot Management System Output](images/system-output.png)

## Technologies and Concepts

- **Microcontroller:** LPC1768 (ARM Cortex-M3)
- **Programming Language:** Embedded C
- **Display:** 16×2 LCD (4-bit interface)
- **Input:** 4×4 Matrix Keypad
- **Output:** GPIO-controlled LEDs
- **Timer:** Timer0

### Concepts Implemented

- GPIO configuration and register-level programming
- Bitwise operations and register masking
- Matrix keypad scanning and input handling
- LCD interfacing and display management
- Timer configuration and periodic output generation
- State management and menu-driven navigation
- Configuration modification and save handling

## Project Objective

To develop an interactive parking slot management simulation using the LPC1768 microcontroller, demonstrating peripheral interfacing, embedded C programming, state management, and real-time hardware control.
