# Network Time Clock

A Wi-Fi-enabled network time clock built using an Arduino UNO R4 WiFi and a 16x2 LCD. It synchronizes time from an NTP server and displays the current New Zealand local time with daylight saving support.

## Features

- Connects to Wi-Fi using user-defined credentials
- Synchronizes time from `time.google.com` (NTP)
- Handles New Zealand daylight saving time (NZST/NZDT)
- Displays local date and 12-hour time (with AM/PM) on a 16x2 LCD
- Outputs synchronization and status messages to Serial

## Hardware

- **Arduino UNO R4 WiFi**
- **16x2 LCD** connected to the following pins:
  - RS: Pin 8  
  - E:  Pin 9  
  - D4: Pin 4  
  - D5: Pin 5  
  - D6: Pin 6  
  - D7: Pin 7  

## Libraries Used

Install via the Arduino Library Manager:

- `WiFiS3`
- `WiFiUdp`
- `NTPClient`
- `LiquidCrystal`
- `TimeLib`
- `Timezone`

## Setup

1. Open the `.ino` file in the Arduino IDE.
2. Update your Wi-Fi credentials:

   ```cpp
   const char ssid[] = "ssid";
   const char pass[] = "password";
   ```

3. Upload the sketch to your Arduino UNO R4 WiFi.
4. Open the Serial Monitor to verify synchronization status.

## Timezone

This project uses the `Timezone` library to automatically apply DST rules for New Zealand.