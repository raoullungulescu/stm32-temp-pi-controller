# STM32 Temperature PI Controller

Bare-metal STM32 (NUCLEO-C031C6) temperature controller: the difference between an NTC and a BMP180 is driven to zero by a PI controller commanding a servo fan. Software I2C, SSD1306 OLED, UART CSV log. Simulated in Wokwi.

## Overview

Embedded project on the **NUCLEO-C031C6** (STM32C031C6T6), written in C with **no HAL**: all peripherals are programmed through direct register access. The system reads a temperature from an **analog sensor (NTC)** and compares it with the one from an **I2C sensor (BMP180)**. The difference between them is driven toward zero by a **PI controller** that commands a **fan** (servo). System state is shown on **UART** (CSV log) and on an **OLED**, while **buttons**, **warning LEDs** and a **buzzer** handle user input and alarms.

> **Note on I2C:** the I2C bus is implemented in **software (bit-banging)** on PB8/PB9. Using the hardware I2C peripheral through registers did not work in the Wokwi simulation, so software I2C is used for both the OLED and the BMP180. The ACK bit is ignored (simulator-specific).

## How it works

```
NTC (ADC, PA0) ──► T_load ──► simulated plant ──► T_sim ──┐
                                   ▲                       ├─► dT = T_sim − T_bmp ──► PI ──► servo (fan)
                                   └──── u (0..100 %) ◄────┘          ▲
BMP180 (soft I2C) ──► T_bmp (ambient) ────────────────────────────────┘
```

1. **NTC** (PA0, 12-bit ADC, 10 kΩ divider, β = 3950) gives the load temperature `T_load`. It acts as the **disturbance**.
2. **BMP180** (software I2C) gives the ambient temperature `T_bmp` and the pressure.
3. The fan's effect is **simulated** with a first-order plant: `T_sim += (Ts / τ) · (T_load − K·u/100 − T_sim)`, with `τ = 10 s` and `K = 35 °C`.
4. The controlled variable is `dT = T_sim − T_bmp`, with reference **0**.
5. The **PI controller** (`Kp = 20`, `Ki = 2`) computes the command `u` in 0–100 %, with **anti-windup** (conditional integration). The real sampling period `Ts` is measured at every step (target 250 ms).
6. `u` is mapped to a servo pulse: 0–100 % → 500–2500 µs (TIM3 CH1, 50 Hz PWM).

### Result

With a disturbance of ~27 °C above ambient, `dT` peaks at ~3 °C, then the controller brings it to 0 in ~50 s, and the command settles at ~77 % (`35 · u/100 = 27`).

<p align="center"><img width="600" alt="Controller response" src="https://github.com/user-attachments/assets/c6576989-512b-4ae7-b5a2-05c86fff31a9" /></p>

## STM32CubeMX configuration

Pins, timers, ADC and interrupts were configured in **STM32CubeMX** (`SmartController.ioc`), and the project was exported with a **Makefile**. CubeMX generates the pin labels as macros in `main.h` (`BTN_UP_Pin`, `LED_ALARM_GPIO_Port`, ...), which the code uses. Peripheral initialization itself is done with register writes, not HAL.

<table width="100%">
<tr>
<td valign="top" width="50%">

| Peripheral | Configuration |
|:---:|:---|
| **ADC1** | IN0 on PA0 (NTC) |
| **TIM3** | CH1 PWM on PB4, PSC = 47, ARR = 19999 (50 Hz) |
| **EXTI** | PA10, PB3: falling edge, pull-up |
| **USART2** | PA2 / PA3, 115200 baud |
| **GPIO out** | PC7, PB5, PA15 (LEDs), PA9 (buzzer) |
| **GPIO (soft I2C)** | PB8 (SCL), PB9 (SDA), open-drain |
| **SysTick** | 1 ms tick, set in code |

</td>
<td valign="top" width="50%">

<img width="100%" alt="CubeMX pinout" src="https://github.com/user-attachments/assets/8b95c915-74a0-4d79-ae25-b78152facc9f" />

</td>
</tr>
</table>

> PB8/PB9 are labeled `I2C1_SCL/SDA` in the pinout but are driven as plain GPIO by the software I2C.

## Hardware

<img width="900" height="500" alt="wokwi" src="https://github.com/user-attachments/assets/0bd5a81a-47b6-4176-a9c7-5f380b2dcef4" />

| Component | Role | Interface / Pin |
|:---|:---|:---|
| NUCLEO-C031C6 | MCU board (STM32C031C6T6) | - |
| NTC sensor module | Analog temperature, acts as the disturbance | ADC1_IN0, PA0 |
| BMP180 | Ambient temperature and pressure | Software I2C, PB8 / PB9, `0x77` |
| SSD1306 OLED 128×64 | Temperature, pressure, alarm and fault display | Software I2C, PB8 / PB9, `0x3C` |
| Servo | Fan actuator | TIM3_CH1 PWM, PB4 |
| LEDs (T-WARN, P-WARN, ALARM) | Temperature, pressure and alarm indicators | PC7, PB5, PA15 |
| Buzzer | Alarm sound | PA9 |
| Buttons (Temp, Pressure) | Select what the OLED shows (UP / DOWN) | EXTI, PA10 / PB3 |

## Alarms and display

| Condition | Effect |
|:---|:---|
| `T_ntc > 70 °C` | **T-WARN** LED on |
| `P > 1000 hPa` (1 bar) | **P-WARN** LED on |
| both of the above | **ALARM**: LED + buzzer blink, OLED shows `ALARM` |
| `dT > 0.1 °C` | **FAULT** (`AVARIE`): OLED shows `AVARIE`, `Ts` and `dT` |
| fault cleared | OLED shows `Ts` (simulated temperature) |

Display priority is **ALARM > FAULT > T_sim**.

**Buttons** use EXTI interrupts on the falling edge with 50 ms debounce. The ISRs only set a flag; the events are consumed in the main loop. **UP** (PA10) shows temperature, **DOWN** (PB3) shows pressure, and both are ignored during alarm or fault. A **~2 s watchdog (IWDG)** is fed at the end of every loop iteration.

## UART log

USART2, 115200 baud, CSV, no `printf` (integer-to-text routines written by hand):

```
ms,T_ntc,T_bmp,pert,T_sim,dT,u
```

| Column | Meaning |
|:---:|:---|
| `ms` | time since boot [ms] |
| `T_ntc` | NTC temperature (ADC) [°C] |
| `T_bmp` | BMP180 ambient temperature [°C] |
| `pert` | disturbance `T_load − T_bmp` [°C] |
| `T_sim` | simulated temperature after the fan's effect [°C] |
| `dT` | `T_sim − T_bmp`, tends to 0 [°C] |
| `u` | applied command [%] |

`plot.py` (pandas + matplotlib) plots `dT` and `u` versus time from the saved serial output (`data.csv`).

## Software structure

- **main**: main loop, alarm logic, display modes
- **controller**: simulated plant, PI with anti-windup, SysTick, UART log
- **ADC**, **BMP180** (datasheet compensation), **SoftI2C**, **OLED** (SSD1306 + custom 5×7 font), **Buttons**, **LED/Buzzer**, **Servo**, **UART**

```c
while (1)
{
    Alarm_Check();      // readings, alarms, display mode
    Regulator_Run();    // PI controller, runs every TS_MS
    Buttons_Process();  // button events
    IWDG_Feed();
}
```

## Build

```bash
make          # build the .elf
make clean
```

Load the resulting `.elf` into the Wokwi simulation (`diagram.json`), move the NTC slider to create disturbances and watch the serial log.

## Known limitations

- The BMP180 is read with blocking busy-wait loops, which is why the real `Ts` exceeds 250 ms.
- The fan's effect is simulated in software, not measured.
