# AR Translation Glasses

An embedded speech translation system based on ESP32-S3 that captures audio, sends it to a Python-based processing server and displays translated output on an OLED display.

## Overview

The project explores a compact wearable translation system using an ESP32-S3 as the embedded controller.

Audio is captured using an I2S microphone and transferred through Wi-Fi to a Python server for speech processing and translation. The processed output is then sent back to the ESP32-S3 and displayed on an OLED.

## System Architecture

```text
I2S Microphone
      |
      v
   ESP32-S3
      |
      | Wi-Fi
      v
 Python Server
      |
      +--> Speech-to-Text
      |
      +--> Translation
      |
      v
   ESP32-S3
      |
      v
  OLED Display
```

## Hardware

* ESP32-S3 WROOM-1
* INMP441 I2S microphone
* 0.91-inch 128×32 OLED display
* Push button
* Wi-Fi connection

## Software and Technologies

* C/C++
* Arduino framework
* Python
* I2S audio interface
* Wi-Fi communication
* Speech processing
* Machine translation
* OLED display communication

## Pin Configuration

### INMP441

| Signal | ESP32-S3 GPIO |
| ------ | ------------: |
| WS     |       GPIO 15 |
| SD     |       GPIO 16 |
| SCK    |       GPIO 14 |

### OLED

| Signal | ESP32-S3 GPIO |
| ------ | ------------: |
| SDA    |        GPIO 8 |
| SCL    |        GPIO 9 |

### Push Button

| Signal | ESP32-S3 GPIO |
| ------ | ------------: |
| Button |        GPIO 2 |

## Working Principle

### 1. Audio Capture

The INMP441 microphone captures speech and provides digital audio data through the I2S interface.

### 2. ESP32-S3 Processing

The ESP32-S3 collects the audio data and communicates with the Python server over Wi-Fi.

### 3. Speech Processing

The Python server processes the received audio and performs speech-to-text conversion.

### 4. Translation

The recognized text is processed by the translation system to generate translated text.

### 5. OLED Output

The translated result is sent back to the ESP32-S3 and displayed on the OLED.


## Setup

### ESP32

1. Open the ESP32 source file using Arduino IDE.
2. Select the appropriate ESP32-S3 board.
3. Configure the Wi-Fi credentials locally.
4. Configure the local Python server address.
5. Upload the firmware to the ESP32-S3.

### Python Server

Install the required Python packages:

```bash
pip install -r requirements.txt
```

Run the server using:

```bash
python server.py
```

The ESP32-S3 and computer running the Python server should be connected to the same network.
