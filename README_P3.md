# PID Motor Speed Controller with Hall-Sensor RPM Feedback

Bare-metal closed-loop motor speed controller on STM32F103C8T6 (Blue Pill). DC motor driven by IRLZ44N MOSFET via 20 kHz PWM. Hall sensor provides RPM feedback for PID control. Runtime gain tuning, CSV telemetry, and live DataStar SSE dashboard. No HAL, no CMSIS, no stdlib, no dynamic allocation.

## Hardware

| Component | Part | Connection |
|-----------|------|------------|
| MCU | STM32F103C8T6 Blue Pill | — |
| MOSFET | IRLZ44N (logic-level N-channel) | PA0 → 100Ω → Gate, Drain → Motor−, Source → GND |
| Motor | DC motor (5V external supply) | Motor+ → 5V, Motor− → MOSFET Drain |
| Flyback diode | 1N4007 | Anode → GND, Cathode → Motor+ |
| Motor cap | 0.1µF ceramic | Across motor terminals |
| Hall sensor | 3144E module | VCC → 5V, GND → GND, S → PB10 |
| EMI filter | 10kΩ + 100nF RC low-pass | Series 10kΩ on PB10, 100nF to GND after resistor |
| USB-UART | CP2102 | PA9 (TX) → RX, PA10 (RX) → TX |

### Wiring Diagram

```
External 5V ──┬── Motor+ ──────── MOSFET Drain
               │    │
               │   1N4007 (cathode)
               │    │
              GND──1N4007 (anode)──── MOSFET Source ── GND
               │
              0.1µF across motor terminals

PA0 ── 100Ω ── MOSFET Gate

Hall sensor module:
  VCC → 5V
  GND → GND
  S   → 10kΩ → PB10
                  │
                100nF → GND

PA9  (USART1 TX) → CP2102 RX
PA10 (USART1 RX) → CP2102 TX
PC13 → Onboard LED
```

### Why External 5V

The motor draws too much current for USB power. A separate 5V supply (phone charger, bench supply, or USB breakout) powers the motor and hall sensor. The Blue Pill runs from USB independently. Motor GND and Blue Pill GND must share a common ground point (star ground to reduce EMI coupling).

## Clock Configuration

- HSE 8 MHz → PLL ×9 → **72 MHz SYSCLK**
- APB1 = 36 MHz (÷2 prescaler)
- APB2 = 72 MHz (no prescaler)
- CSS enabled in RCC_CR — auto-switches to HSI and fires NMI_Handler if the HSE crystal fails
- SysTick: 72 MHz ÷ 1000 = 72,000 cycles/tick → RELOAD = **71,999** (counts from RELOAD to 0, so RELOAD+1 = 72,000 total cycles per tick = 1 ms)

## PWM

- TIM2 Channel 1 on PA0 (alternate function)
- Frequency: 20 kHz (PSC = 71, ARR = 49 → 72 MHz ÷ 72 ÷ 50 = 20 kHz)
- Duty cycle: CCR1 range 0–49 (0% to ~100%)
- PWM Mode 1 (OC1M = 0b110): output high while counter < CCR1

### Why 20 kHz

Above human hearing threshold. Low enough for the IRLZ44N's gate capacitance to fully charge/discharge through the 100Ω gate resistor each cycle. The 100Ω resistor limits inrush current to the gate and damps ringing.

## PID Controller

### Q8.8 Fixed-Point Arithmetic

All PID math uses Q8.8: 8 integer bits, 8 fractional bits. A gain of 2.5 is stored as `0x0280` (2 × 256 + 0.5 × 256 = 640). This avoids floating-point entirely — no `-lm`, no soft-float overhead, deterministic cycle count.

Operations:
- Multiply: `(a * b) >> 8`
- Division: `(a << 8) / b`
- Conversion from integer: `value << 8`

### Control Loop

```
error = setpoint - actual_rpm
P = Kp * error
I = I_prev + Ki * error * dt
D = Kd * (error - error_prev) / dt
output = clamp(P + I + D, 0, 49)
CCR1 = output
```

- Integral windup clamp prevents accumulator runaway
- Output clamped to 0–49 (maps directly to CCR1)
- Anti-windup: integral term frozen when output is saturated
- Stiction compensation: minimum duty cycle kick when transitioning from stopped to running
- Loop runs at SysTick rate (1 kHz), decimated for telemetry output

### Tuning Methodology

Started with P-only (Kp = 10, Ki = 0, Kd = 0). Increased Kp until oscillation appeared, then backed off ~30%. Added Ki for steady-state error elimination — small values (Ki = 1–3) to avoid windup. Kd left at 0 for this motor — the hall sensor resolution is too coarse for meaningful derivative action at low RPM.

## Hall Sensor RPM Measurement

The 3144E hall sensor module outputs a falling edge each time the magnet (attached to the motor fan blade) passes. EXTI10 (PB10, falling edge) captures each edge.

### RPM Calculation

```
period_ms = current_tick - last_edge_tick
rpm = 60000 / period_ms    (one magnet = one edge per revolution)
```

### Debouncing

Motor brush noise causes false edges at certain speeds. Two layers of protection:

1. **Software debounce:** SysTick-based minimum interval between accepted edges. Any edge arriving within the debounce window is discarded.
2. **Hardware RC filter:** 10kΩ series resistor + 100nF capacitor to GND on PB10. Time constant τ = 1 ms, cutoff frequency ~160 Hz. Motor brush noise is typically 10 kHz+ — the filter attenuates it by ~35 dB while passing real hall edges (max ~91 Hz at full speed).

### Why Both

Software debounce alone works at steady speeds but misses rapid noise bursts during speed transitions. The RC filter kills high-frequency noise at the source before it reaches the GPIO input, making the software debounce's job trivial.

## UART Interface

**Baud rate:** 115200, 8N1

### Commands (RX)

| Command | Example | Effect |
|---------|---------|--------|
| `SP <value>` | `SP 500` | Set target RPM to 500 |
| `KP <value>` | `KP 15` | Set Kp = 15 (stored as 15 << 8 = 0x0F00) |
| `KI <value>` | `KI 2` | Set Ki = 2 |
| `KD <value>` | `KD 0` | Set Kd = 0 |

All commands are newline-terminated. Invalid commands print an error message via the ring buffer.

### CSV Telemetry (TX)

Decimated to ~200 lines/sec to avoid saturating UART at 115200 baud.

```
tick,setpoint,actual,error,duty
12345,500,487,13,38
12346,500,492,8,35
12347,500,501,-1,32
```

### Ring Buffer

All UART output goes through a 256-byte ring buffer (power-of-2, mask-based wrap). ISRs write messages into the buffer, main loop drains to USART1→DR. This prevents ISR blocking and string interleaving from concurrent interrupt handlers (SysTick, EXTI10, and main loop all produce output).

## DataStar SSE Dashboard

Live browser dashboard using DataStar v1.0.0-RC.8 with a Python SSE server.

### Architecture

```
Blue Pill → UART → CP2102 → USB → Python serial reader
                                        ↓
                                   SSE server (stdlib HTTPServer + ThreadingMixIn)
                                        ↓
                                   Browser (DataStar HTML)
```

### Features

- Real-time PID chart: setpoint vs actual RPM (scrolling line graph)
- Duty cycle bar indicator
- RPM numeric display
- Gain tuning sliders (Kp, Ki, Kd) wired bidirectionally — slider changes send commands back to the Blue Pill via serial
- Color-coded error field (green when |error| < threshold, red otherwise)

### DataStar RC.8 Syntax Notes

- Colon delimiters: `data-on:click`, `data-bind:value`
- SSE connection via `data-init`
- Morph exclusion via `data-ignore-morph` (not `data-ignore`)

## Build

### Prerequisites

- `arm-none-eabi-gcc`
- `make`
- No external libraries, no HAL, no CMSIS

### Compile and Flash

```bash
make clean
make
# Flash via st-flash or OpenOCD
st-flash write build/main.bin 0x08000000
```

### Project Structure

```
├── Makefile
├── linker.ld          # 64K flash, 20K SRAM, _estack at 0x20005000
├── startup.c          # Vector table, Reset_Handler, zero BSS, copy data
├── main.c             # All application code
└── build/
    ├── main.o
    ├── startup.o
    ├── main.elf
    └── main.bin
```

### Compiler Flags

```
-mcpu=cortex-m3 -mthumb
-fno-exceptions -fno-rtti -nostdlib -ffreestanding
-Wpedantic -Werror -Wall -Wextra
-DDEBUG                # Enables assert (bkpt #0)
```

Release builds define `NDEBUG` — asserts compile to `((void)0)`.

## Design Decisions

**Why no HAL / no CMSIS:** Direct register access gives full visibility into peripheral behavior. When a bug happens (like the motor EMI false-edge problem), there's no abstraction layer hiding the root cause. Every register write is explicit and auditable.

**Why Q8.8 not float:** Deterministic cycle count. No soft-float library linked. No `-lm`. MISRA C Rule 1.4 advisory: use of floating-point is discouraged in safety-related code due to platform-dependent rounding behavior. Q8.8 gives 1/256 resolution (~0.004), more than sufficient for motor PWM duty cycles.

**Why BSRR not ODR:** GPIO set/clear operations use BSRR (atomic, single write) instead of ODR (read-modify-write, vulnerable to races between main loop and ISRs). The only ODR operation remaining is LED toggle on PC13 (`ODR ^=`), which has no BSRR equivalent.

**Why ring buffer:** Without it, a UART print inside an ISR blocks until TX is complete. With concurrent interrupts (SysTick at 1 kHz, EXTI10 on every hall edge), this causes string interleaving and missed deadlines. The ring buffer decouples production (ISR) from consumption (main loop polling).

**Why 100Ω gate resistor:** Limits dI/dt into MOSFET gate capacitance (~1800 pF for IRLZ44N), preventing ringing on the gate drive trace. Without it, parasitic inductance in breadboard wiring can cause gate voltage overshoot above Vgs(max).

**Why 1N4007 flyback diode:** When the MOSFET switches off, the motor's inductance generates a voltage spike (back-EMF). The diode clamps this spike to ~0.7V above the supply rail, protecting the MOSFET from avalanche breakdown.

## Known Limitations

- **RPM resolution at low speed:** With one magnet, one edge per revolution, RPM below ~100 has ±10% measurement granularity due to integer division in the period→RPM conversion
- **5V motor speed range:** Limited max RPM (~1500) due to low supply voltage. Higher voltage motor would demonstrate the PID controller's full dynamic range
- **Oscillation at certain setpoints:** Near the motor's stiction threshold (~15% duty), the PID can oscillate between running and stalled states. Stiction compensation reduces but doesn't eliminate this
- **No persistent gain storage:** Tuned Kp/Ki/Kd values reset on power cycle. Would need flash write (W25Q64 or internal flash) for persistence
