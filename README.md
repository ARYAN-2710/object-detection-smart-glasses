# Object Detection Smart Glasses

An Arduino-based wearable prototype designed to help a visually impaired user notice nearby obstacles using distance sensing, recorded audio prompts, and a buzzer whose tone changes with distance.

## Components

- Arduino Nano (classic ATmega328P version assumed)
- HC-SR04 ultrasonic distance sensor
- MPU6050 accelerometer and gyroscope module
- DFPlayer Mini MP3 module
- microSD card with audio files and a small speaker
- Piezo buzzer
- BC547 NPN transistor and base resistor for driving the buzzer
- Single-cell 3.7 V Li-ion battery and TP4056 charging module
- Power switch, two zero-PCBs, connecting wires, and a prototype enclosure

## Connections

The table below documents the pin assignments used by `smart_glasses.ino`. Power connections depend on the actual modules and supply arrangement; check the voltage requirements before powering the circuit.

| Module / signal | Arduino Nano connection | Notes |
|---|---|---|
| HC-SR04 TRIG | D9 | Trigger output |
| HC-SR04 ECHO | D10 | Echo input |
| HC-SR04 VCC / GND | Appropriate supply / GND | Use a supply suitable for the HC-SR04 |
| MPU6050 SDA | A4 | I²C data on classic Nano |
| MPU6050 SCL | A5 | I²C clock on classic Nano |
| MPU6050 INT | D2 | Motion interrupt configured by the code |
| MPU6050 VCC / GND | As required by breakout / GND | Module voltage tolerance varies by breakout board |
| Arduino D6 → DFPlayer RX | D6 through 1 kΩ series resistor | SoftwareSerial transmit from Arduino |
| DFPlayer TX → Arduino D7 | D7 | SoftwareSerial receive at Arduino |
| DFPlayer BUSY | D4 | Code treats LOW as audio playing |
| DFPlayer speaker output | SPK+ / SPK− to speaker | Follow DFPlayer speaker-output guidance; do not connect either speaker terminal to ground unless the module documentation explicitly allows it |
| Buzzer driver | D8 through 1 kΩ to BC547 base; emitter to GND; collector to buzzer negative | Confirm the transistor pinout for the exact part/package |
| Buzzer positive | Suitable positive supply rail | Confirm buzzer voltage and current rating |
| Battery / charging | Battery to TP4056 B+ / B−; load to OUT+ / OUT− | TP4056 is a charger, not a regulated 5 V boost converter. A single-cell battery voltage varies with charge. Do not assume the output rail is 5 V. |

The prototype was assembled across two zero-PCBs. The Arduino-side board holds the Nano, DFPlayer Mini, MPU6050, speaker, and BC547 buzzer-driver circuit. The HC-SR04 is mounted at the centre of the glasses. The opposite-side board holds the TP4056, battery, switch, and buzzer. The buzzer-driver output is wired to the buzzer on the opposite side.

## How it works

1. The HC-SR04 measures the distance to an object in front of the glasses.
2. When a valid distance is below 80 cm, the DFPlayer Mini plays the obstacle-warning track (track 2).
3. If the obstacle remains after audio playback, the buzzer is activated.
4. The buzzer frequency is mapped from approximately 300 Hz at 80 cm to 2500 Hz at 4 cm, so closer obstacles produce a higher tone.
5. MPU6050 gyroscope readings are used to detect sufficiently fast head rotation and play track 3. Track 1 is used as the startup sound.

## Software setup

1. Install the Arduino IDE.
2. Select the correct board and port for your Arduino Nano.
3. Install compatible libraries: `DFRobotDFPlayerMini`, `MPU6050` (the library matching the API used in the sketch), and the standard `Wire` and `SoftwareSerial` libraries supplied with Arduino.
4. Copy the audio files to a microSD card in a format and naming layout supported by the DFPlayer Mini library/module. Confirm the track numbering on your specific card/module; the sketch expects startup = track 1, obstacle = track 2, and head-turn = track 3.
5. Connect the board and upload `smart_glasses.ino`.
6. Open Serial Monitor at 9600 baud. Keep the MPU6050 still while its offsets are calibrated during startup.
7. Test the sensor and alerts on a bench before wearing the prototype.

## Known limitations and safety notes

- This is a prototype, not a certified assistive device. It must not be relied on as the user's only means of navigation.
- `pulseIn()` has no explicit timeout in this version and can block while waiting for an echo. Invalid or out-of-range readings need more robust handling.
- A zero distance is treated as “no obstacle,” although it can also indicate a failed or missed sensor reading.
- The function named `isHeadTilted()` checks gyroscope angular-rate readings against a threshold; it does not estimate absolute head tilt/orientation.
- The `mpuInterrupt` flag is set by the interrupt handler but is not used by the main loop in this version.
- Startup audio is stopped after a fixed two-second delay and may be cut off if the track is longer.
- The code uses `delay()` and blocking sensor reads, so it is not fully non-blocking.
- `volume(30)` is near the maximum setting on common DFPlayer Mini libraries; reduce it if audio is too loud or distorted.
- Verify all power rails and module voltage requirements. A TP4056 output follows the Li-ion battery voltage (roughly 3.0–4.2 V), not a regulated 5 V. Do not feed this directly into a Nano 3.3 V output pin. Use a suitable regulated supply/input arrangement for the exact Nano board and modules.
- Check the BC547 pinout and buzzer current against the datasheets for the exact parts used.

## Project status

This repository documents the prototype wiring and code as described by the project notes. The sketch has not been compiled or hardware-tested as part of preparing this repository; confirm library compatibility and verify the circuit before use.
