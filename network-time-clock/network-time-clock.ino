/*
  ============================================================
                          Network Time Clock
  ============================================================
  - Connects to Wi-Fi using provided credentials
  - Synchronizes time from an NTP server (time.google.com)
  - Applies New Zealand DST rules for local time display
  - Displays date and 12-hour time with AM/PM on a 16x2 LCD
  - Prints sync details and status messages to Serial

  Hardware:
  - Wi-Fi capable board (e.g., ESP32-S3)
  - 16x2 LCD connected to pins 8,9,4,5,6,7

  Libraries used:
  - WiFiS3.h        : Wi-Fi support for ESP32-S3 boards
  - WiFiUdp.h       : UDP networking support for NTP
  - NTPClient.h     : Network Time Protocol client
  - LiquidCrystal.h : LCD display control
  - TimeLib.h       : Timekeeping functions
  - Timezone.h      : Timezone and DST management

  Author: Sam Evans
  Date:   2025-07-16
*/


#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <LiquidCrystal.h>
#include <TimeLib.h>
#include <Timezone.h>


// ------------------------- User Variables -------------------------

// Wi-Fi credentials
const char ssid[] = "ssid";
const char pass[] = "password";

// Timing constants
const unsigned long ntpUpdateInterval = 600000; // 10 minutes between syncs
//const unsigned long ntpUpdateInterval = 5000; // 5 seconds between syncs (for testing)
const unsigned long displayDelay = 1000;        // 1 second between display updates
const int wifiMaxRetries = 10;


// ------------------ Hardware and Libraries Setup ------------------

// Initialize the 16x2 LCD with pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Defines New Zealand DST rules
TimeChangeRule nzDST = {"NZDT", Last, Sun, Sep, 2, 780};  // UTC+13 during DST
TimeChangeRule nzSTD = {"NZST", First, Sun, Apr, 3, 720}; // UTC+12 standard time
Timezone nzTime(nzDST, nzSTD); // Timezone object to handle NZ time & DST

// Creates a UDP client to send and receive packets for NTP (Network Time Protocol)
WiFiUDP ntpUDP;

// Initialize NTP client with Google time server and UTC offset 0
NTPClient timeClient(ntpUDP, "time.google.com", 0);


// ------------------------- Date Formatting ------------------------

// Short month names, indexed 0–11 for January to December
const char* monthShortNames[] = {
  "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

// Short weekday names, indexed 0–6 (Sunday to Saturday) per TimeLib
const char* weekdayShortNames[] = {
  "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};

// Returns day number suffix
const char* getDaySuffix(int d) {
  if (d >= 11 && d <= 13) return "th";
  switch (d % 10) {
    case 1: return "st"; 
    case 2: return "nd"; 
    case 3: return "rd"; 
    default: return "th";
  }
}


// ----------------------- Serial Monitor Functions -----------------------

// Converts a time_t value into a formatted string for serial output
String formatTimeForSerial(time_t t) {
  char buf[40];  // Buffer to hold the formatted date/time string

  // Format: "Weekday Mon DD HH:MM:SS YYYY", showing full date and 24-hour time
  snprintf(buf, sizeof(buf), "%s %s %02d %02d:%02d:%02d %d",
           weekdayShortNames[weekday(t) - 1],  // Weekday name (Sun, Mon, ...)
           monthShortNames[month(t) - 1],      // Month name (Jan, Feb, ...)
           day(t),                             // Day of the month (01-31)
           hour(t), minute(t), second(t),      // Hour, minute, second (24h)
           year(t));                           // Year (YYYY)

  return String(buf);
}

// Prints NTP synchronization information to Serial
void printSyncInfo() {
  // Get current UTC and local time
  time_t utc = timeClient.getEpochTime();
  time_t local = nzTime.toLocal(utc);
  bool isDST = nzTime.locIsDST(utc);

  // Determine the current timezone rule based on DST status
  TimeChangeRule *tcr = isDST ? &nzDST : &nzSTD;

  // Output synchronization details including time and timezone info
  Serial.print("Sync: UTC=");
  Serial.print(formatTimeForSerial(utc));
  Serial.print(", Local=");
  Serial.print(formatTimeForSerial(local));
  Serial.print(", DST=");
  Serial.print(isDST ? "Yes" : "No");
  Serial.print(", Offset=");
  Serial.print(tcr->offset);
  Serial.print(" min, TZ=");
  Serial.println(tcr->abbrev);
}


// ----------------- Helper Functions --------------------

// Displays the current date and time on the LCD
void displayDateTime() {
  // Get current UTC and convert to New Zealand local time
  time_t utc = timeClient.getEpochTime();
  time_t local = nzTime.toLocal(utc);

  // Extract date and time components from local time
  int w = weekday(local) - 1;
  int m = month(local) - 1;
  int d = day(local);
  int h = hour(local);
  int min = minute(local);
  int s = second(local);

  // Format the date as a string like "Mon 15th Jul"
  char dateStr[17];
  snprintf(dateStr, sizeof(dateStr), "%s %d%s %s", weekdayShortNames[w], d, getDaySuffix(d), monthShortNames[m]);
  
  // Create a blank 16-character line for centering the date
  char dateLine[17] = "                "; 
  int pad = (16 - strlen(dateStr)) / 2; // Calculate left padding for centering the date

  // Copy the date string into the padded line
  memcpy(dateLine + pad, dateStr, strlen(dateStr));

  // Convert 24-hour time to 12-hour format
  int h12 = h % 12; // Convert hour to 12-hour format
  if (h12 == 0) h12 = 12; // Handle midnight and noon as 12
  char timeNumStr[12]; // Buffer for time string
  snprintf(timeNumStr, sizeof(timeNumStr), "%d:%02d:%02d", h12, min, s); // Format time as "H:MM:SS"

  // Determine AM or PM
  const char* statusStr = (h >= 12) ? "PM" : "AM";

  // Prepare a blank line to center the time and status
  char timeLine[17] = "                "; 
  int timeLen = strlen(timeNumStr); // Length of the time string
  int timeStart = (16 - timeLen - 3) / 2; // Calculate start position with space for AM/PM
  int statusPos = timeStart + timeLen + 1; // Position to insert AM/PM

  // Place time and AM/PM into the formatted time line
  memcpy(timeLine + timeStart, timeNumStr, timeLen);
  memcpy(timeLine + statusPos, statusStr, strlen(statusStr));

  // Print the date and time to the LCD
  lcd.setCursor(0, 0);
  lcd.print(dateLine);
  lcd.setCursor(0, 1);
  lcd.print(timeLine);
}


// ---------------------------- Setup ----------------------------

// Initializes serial, LCD, Wi-Fi, and NTP time sync
void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for Serial to be ready

  // Initialize LCD and show Wi-Fi connection attempt
  lcd.begin(16, 2);
  lcd.print("Connecting Wi-Fi");
  lcd.setCursor(0, 1);
  lcd.print(ssid);

  // Counter for Wi-Fi connection retries
  int retries = 0;

  // Attempt to connect to Wi-Fi, retrying up to a maximum number of times
  while (WiFi.begin(ssid, pass) != WL_CONNECTED && retries++ < wifiMaxRetries) {
    Serial.print("Wi-Fi connect attempt ");
    Serial.print(retries);
    Serial.println(" failed. Retrying...");
    delay(3000);
  }

  // If Wi-Fi failed to connect, show error and halt execution
  if (WiFi.status() != WL_CONNECTED) {
    lcd.clear();
    lcd.print("Wi-Fi Failed");
    Serial.println("Wi-Fi connection failed. Stopping.");
    while (true) delay(1000); // Halt execution with periodic pause
  }

  // Wi-Fi successfully connected; update LCD and serial
  Serial.println("Wi-Fi connected");
  lcd.clear();
  lcd.print("Wi-Fi Connected");
  delay(1000); // Pause briefly to show success message
  
  // Start the NTP time client
  timeClient.begin();

  // Attempt initial NTP time sync and log result
  if (timeClient.forceUpdate()) {
    printSyncInfo(); // Print sync details to serial
  } else {
    Serial.println("Initial NTP sync failed"); // Log sync failure
  }
}

// Tracks the last time an NTP sync occurred
unsigned long lastSync = 0;


// ---------------------------- Loop ----------------------------

// Main loop to keep time updated and display current time
void loop() {
  timeClient.update(); // Refreshes internal time from server if needed

  // Check if it's time to manually force an NTP update
  if (millis() - lastSync > ntpUpdateInterval) {
    // Attempt to get the latest time from the NTP server
    if (timeClient.forceUpdate()) {
      // On success, update the sync timestamp and print details
      lastSync = millis();
      printSyncInfo();
    } else {
      // Log failure if time could not be synced
      Serial.println("NTP sync failed");
    }
  }

  // Update the LCD with the current local time
  displayDateTime();
  delay(displayDelay); // Wait before refreshing the display
}
