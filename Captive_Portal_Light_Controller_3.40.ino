/*
   Operation:
   A short press (click) will toggle the lights between on and off.
   Grounding pin 33 will keep the display on until the pin is pulled, then
    it will go out in 30 seconds afterwards, as expected.

   Entering a number between 1 and 7
*/

//#define DEBUG_PRINTS
#define DISPLAY_ON_TIME 30

// Turn off display after xx seconds.
unsigned long displayOnDuration = DISPLAY_ON_TIME * 1000;

String pgmVer = "v3.40";  // Add in the WiFi scan and delay.
#define LED_BUILTIN 2
//#define CONFIG_FOR_US  // Uncomment for Joe's time and location
#define DO_LED            // Turn on builtin LED when the lights are on

#include "Button2.h"
Button2 button;
int pinForceSend = 19;  // Change pin number here to an available pin.
#define displayOnSolid 33  // If grounded, the display will stay on.

//#include "TimeLib.h"
#include "esp_sntp.h"
// Always has the current, correct date/time.
struct tm currentLocalTime;
// Work struct for finding timezone offset from GMT.
struct tm workTime;
struct tm * timeinfo;
time_t workTime_t;

// Civil Dusk Hour and Minute for human consumption.
int connAttempts;
int prev_sec = 100;  // One second gate in loop().
int rebootTime;

#include <SolarCalculator.h>
// The following are in the format where 1.5 = 1:30.
double Csunrise;  // Sunrise, in hours (UTC)
double Ctransit;  // Solar noon, in hours (UTC)
double Csunset;   // Sunset, in hours (UTC)
double Cdawn;     // Civil dawn, in hours (UTC)
double Cdusk;     // Civil dusk, in hours (UTC)

#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();

// OLED Stuff here
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// See Serial.print line on serial monitor for pin numbers.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
// End of OLED Stuff

bool displayOn = true;
unsigned long displayTurnOffTime;
unsigned long adjustForLateOn;
// If true, we are in the WaitForOff routine, lights on.
bool lightsAreOn = false;
bool forceRecomputeDusk = false;

// Display line offsets for nice spacing.
const int displayLine1 = 0;
const int displayLine2 = 16;
const int displayLine3 = 29;
const int displayLine4 = 42;
const int displayLine5 = 56;

int onDurationM;    // How many minutes to stay on.

// 3600 seconds = 1 hour.
// To change the On time for testing, make your adjustments here.
// In seconds. Positive makes it later. Negative makes it earlier.
const int onTimeAdj = 0;  // No adjustment this time.
//const int onTimeAdj = -50 * 60;  // Backup  "on" time by 50 minutes.
//const int onTimeAdj = +50 * 60;  // Advance "on" time by 50 minutes.
// To change the On time for testing, make your adjustments here.

// On time minutes
int lightsOnDurationM;

// On time seconds
int lightsOnDurationS;
// On time milliseconds
//unsigned long lightsOnDurationMillis;

#ifdef CONFIG_FOR_US
double latitude      =   38.076;
double longitude     = -122.18151;
#else  // Mike's config parms
double latitude      =   18.19;
double longitude     =  120.55;
#endif

char JustTime[100];   // Just the time part of the date-time string.
char JustDate[100];   // Just the date part of the date-time string.
char FullDate[100];   // Date and time
char workDate[100];       // String builder work field
char timeRemaining[100];  // Used while waiting for lights off
bool myIsPM;
// If you like AM and PM. False gives 24 hour clock format.
const boolean showAMPM = false;

time_t civilDuskEpoch;
time_t lightsOffSecond;  // The second the lights should go off.
time_t nowEpoch;

// Sun info from calculate routine.
//  "dl" means delayed lights then on or off.
int dloN_Mon, dloN_DOM, dloN_Year, dloN_Hour, dloN_Min, dloN_Sec;
//int dloFF_Hour, dloFF_Min, dloFF_Sec;
//int dloFF_Mon, dloFF_DOM, dloFF_Year,
/**************************************************************************
  AutoConnect_ESP32_minimal.ino
  For ESP8266 / ESP32 boards
  Built by Khoi Hoang https://github.com/khoih-prog/ESP_WiFiManager
  Licensed under MIT license
 *************************************************************************/
#if !( defined(ESP32) )
#error This code is intended to run on ESP32 platform only.
#endif
//https://github.com/khoih-prog/ESP_WiFiManager
//#include <ESP_WiFiManager.h>
//https://github.com/khoih-prog/ESP_WiFiManager
//#include <ESP_WiFiManager-Impl.h>

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
//WiFiManager, Local intialization. Once its business is done,
// there is no need to keep it around
WiFiManager wm;

#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
char cUserReq[2];
char cInputCvtBuffer[100];
bool eolReceived = true;  // I just received an 0x0a or 0x0d, not a number.
bool skipTillEOL = false;

#include "Preferences.h"  // To save/retrieve the lights on duration.
Preferences preferences;

bool    OnTimeInputSource;  // Input from Serial or BT?
#define OnTimeInputSourceBT true
#define OnTimeInputSourceSerial false

#define I2C_SDA 32
#define I2C_SCL 33

/***************************************************************************/
void setup()
/****setup******************************************************************/
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(displayOnSolid, INPUT_PULLUP); // Force display on for testing.

  Serial.begin(115200); delay(2000);
  Serial.print(F("\r\nStarting Captive Portal Light Controller on "));
  Serial.println(ARDUINO_BOARD);
  Serial.print("Running from "); Serial.println(__FILE__);
  Serial.printf("Use SDA pin %i, SCL pin %i for the display.\r\n", SDA, SCL);
  // OLED Address 0x78, 0x3C, 0x3D for 128x64
  // check i2cdect.ino for the correct address when screen is connected.

  //In landscape orientation:
  //Font size 1, 21 characters across (approximately)
  //Font size 2, 11 characters across
  //Font size 3,  7 characters across

  //  Wire.begin(I2C_SDA, I2C_SCL);  // If you want to change from 21 and 22 for some reason.

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }

  button.begin(pinForceSend);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(22, displayLine1);
  display.print("Joe's Magical,");
  display.setCursor(22, displayLine2);
  display.print("Mystical light");
  display.setCursor(14, displayLine3);
  display.printf("controller %s", pgmVer);
  // Let my user admire the beautiful display.
  display.display(); myWait(3000);
#ifdef DO_LED
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
#else
  delay(2000);
#endif

  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  // WiFi.scanNetworks will return the number of networks found.
  Serial.println("Begin SSID scan.");
  display.clearDisplay();
  display.setCursor(0, displayLine1); display.print("SSID Scan...");
  display.display();
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  while (n == 0) {
    Serial.println("There are no WiFi Networks detected.  Waiting 1 minute to try again.");
    display.clearDisplay();
    display.setCursor(0, displayLine1); display.print("No SSIDs seen.");
    display.setCursor(0, displayLine2); display.print("Waiting 1 minute");
    display.setCursor(0, displayLine3); display.print("to scan again.");
    display.display();
    delay(60000);
    n = WiFi.scanNetworks();
  }
  Serial.println("At least 1 SSID found.  Trying to connect.");

  display.clearDisplay();
  display.setCursor(0, displayLine1); display.print("Connect to");
  display.setCursor(0, displayLine2); display.print("LightController");
  display.setCursor(0, displayLine3); display.print("to complete config.");
  display.setCursor(0, displayLine4); display.print("Else, wait for");
  display.setCursor(0, displayLine5); display.print("AutoConnect.");
  display.display();
  Serial.println("If no connection, connect WiFi to 192.168.4.1/LightController/lightitup");

  // wm.resetSettings(); // wipe settings -- emergency use only.
  Serial.print("Connecting to WiFi...");
  wm.setTimeout(60);         // 60 second connect timeout then reboot.
  wm.disconnect();
  wm.setConnectTimeout(10);  // 10 seconds (I think) timeout.
  wm.setConnectRetries(3);   //  3 connection attempts before failure (?)
  wm.setDebugOutput(false);  // Turn on/off debug messages during connect.
  // password protected ap
  bool res = wm.autoConnect("LightController", "lightitup");

  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi
    Serial.println("WiFi connected");
    Serial.print("SSID: ");       Serial.println(WiFi.SSID());
    Serial.print("IP address: "); Serial.println(WiFi.localIP());
  }

  SerialBT.register_callback(BTcallback);

  if (!SerialBT.begin("LightController"))  //Bluetooth device name
    Serial.println("An error occurred initializing Bluetooth");
  else
    Serial.println("Bluetooth initialized");

  display.clearDisplay();
  display.setCursor(0, displayLine1);
  display.print("Fetching Time...");
  display.display();

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
#if defined CONFIG_FOR_US
  setenv("TZ", "PST8PDT,M3.2.0,M11.1.0", 1);  // California Riviera
#else
  setenv("TZ", "PHT-8", 1);                   // Philippine Sweat Shop ;-))
#endif
  tzset(); // Set the TZ environment variable
  // A little over a day. Try to avoid busy times at NTP.org.
  //  setSyncInterval(86413000);
  Serial.println("Fetch Initial Time Set... ");

//  time(&workTime_t);
//  timeinfo = localtime(&workTime_t); delay(2000);
//  sntp_set_sync_interval(86413000);  // 1 day in ms.
//  int iYear = 0;
//  while (iYear < 2023) {
//    time(&workTime_t);
//    timeinfo = localtime(&workTime_t);
//    iYear = timeinfo->tm_year + 1900;
//    Serial.printf("Waiting for valid time. Year returned is %i\r\n", iYear);
//    delay(2000);
//  }

  // This kickstarts everything, apparently.
  while (!getLocalTime(&currentLocalTime)) {
    display.clearDisplay();
    display.setCursor(0, displayLine1);
    display.print("Fetching Time...");
    display.setCursor(0, displayLine2);
    display.print("try #" + String(connAttempts + 1)); display.display();
    // delay(500);
    Serial.printf("Try #% 2i\r\n", connAttempts + 1);
    if (++connAttempts > 29 ) {
      Serial.println("\r\nTime fetch failed.  Rebooting in 2 seconds.");
      display.clearDisplay();
      display.setCursor(0, displayLine3);
      display.print("Time Fetch Failed");
      display.setCursor(0, displayLine4);
      display.print("Rebooting in 2 secs.");
      display.display();
      delay(2000);
      ESP.restart();
    }
  }
  Serial.println("Completed.");
  /*
    433 MHz transmitter infomation and example for different
    sending methods at: https://github.com/sui77/rc-switch/
  */
  //Pin #14 ON box with push button. #13 on the other.
  mySwitch.enableTransmit(14);
  mySwitch.setProtocol(1);
  mySwitch.setPulseLength(390);
  // Optional set number of transmission repetitions. Must be from 2 to 8.
  mySwitch.setRepeatTransmit(5);

  getDateTimeInfo();  // Calc today in human readable
  display.clearDisplay();
  display.setCursor(0, displayLine1);
  display.print("IP: "); display.print(WiFi.localIP());
  display.setCursor(0, displayLine2);
  display.print("It's " + String(JustDate));
  display.setCursor(0, displayLine3);
  display.print("Time: " + String(JustTime));
  display.display();

  //  Serial.print("IP: "); Serial.println(WiFi.localIP());
  delay(5000);    //display IP address, Date & Time for 5 seconds.

  Serial.println("Setup done, beginning operation.");
  Serial.printf("\r\nTime now: %s\r\n", FullDate);

  preferences.begin("LightController", false);
  lightsOnDurationM = preferences.getInt("onMinutes", 180);
  // Testing
  //  lightsOnDurationM = 180;
  Serial.printf("Lights on for %i minutes.\r\n", lightsOnDurationM);
  lightsOnDurationS = lightsOnDurationM * 60; // Now we have seconds.
  // Get the time that the lights should be turned on.
  civilDuskEpoch = computeCivilDusk();  // This is epoch in seconds
  // Plus seconds makes it later, negative for earlier.
  civilDuskEpoch += onTimeAdj;  // Add/subtract test time, if any.
  // Now get the time the lights should go out.
  lightsOffSecond = civilDuskEpoch + lightsOnDurationS;

  /*
    int onDurationH = 3;    // How many full hours for lights to stay on.
    int onDurationM = 0;    // How many additional minutes to stay on.

    // 3600 seconds = 1 hour.
    // To change the On time for testing, make your adjustments here.
    // In seconds. Positive makes it later. Negative makes it earlier.
    const int onTimeAdj = 0;
    // To change the On time for testing, make your adjustments here.

    // On time minutes
    int lightsOnDurationM = (onDurationH * 60) + onDurationM;
    // On time seconds
    int lightsOnDurationS = lightsOnDurationM * 60;
    // On time milliseconds
    unsigned long lightsOnDurationMillis = lightsOnDurationS * 1000;

  */
  displayTurnOffTime = millis() + displayOnDuration;  // Display off time.

  memset(cInputCvtBuffer, 0, sizeof(cInputCvtBuffer));
  Serial.println("Setup finished, to work we go.");
}
/***************************************************************************/
void loop()
/****loop*******************************************************************/
{
  if (digitalRead(displayOnSolid) == 0) {
    displayTurnOffTime = millis() + displayOnDuration;
    displayOn = true;
  }

  if (displayOn) {
    if (millis() > displayTurnOffTime) {
      display.clearDisplay();
      display.display();
      displayOn = false;
    }
  }
  button.loop();
  if (button.wasPressed()) {
    switch (button.read()) {
      case single_click:
        Serial.println("Single push");
        Serial.println("Manually sending code now.");
        mySwitch.send("101010101111111111101000");
        displayTurnOffTime = millis() + displayOnDuration;
        displayOn = true;
        display.clearDisplay();
        display.setCursor( 0, displayLine1);

        //             123456789112345678921
        display.print("** Button Pressed **");
        display.setCursor( 5, displayLine2);

        //             123456789112345678921
        display.print("Status of lights");
        display.setCursor(20, displayLine3);

        //             123456789112345678921
        display.print("unknown to me now.");
        display.setCursor(20, displayLine5);

        //             123456789112345678921
        display.print("Please verify.");
        display.display(); delay(5000);  // was 10000
        break;
      case long_click:
        Serial.println("Button Long press");
        displayTurnOffTime = millis() + displayOnDuration;
        displayOn = true;
        break;
        // case double_click: Serial.println("double"); break;
        // case triple_click: Serial.println("triple"); break;
    }
  }

  handleUserReq();  // Input of different on-time duration. In minutes.

  // Calc today as human readable
  //  Also, fill in currentLocalTime struct and calc "now" epoch.
  getDateTimeInfo();

  if (prev_sec == currentLocalTime.tm_sec) return;  // One second gate.
  prev_sec = currentLocalTime.tm_sec;               // Reset the gate for next time.

#if defined DEBUG_PRINTS
  Serial.printf("Status %02i:%02i %i\r\n",
                currentLocalTime.tm_min,
                currentLocalTime.tm_sec,
                forceRecomputeDusk);
#endif

  // Once per 6 hours is plenty for this.
  if (currentLocalTime.tm_sec == 0 && currentLocalTime.tm_min == 0)
    Serial.printf("\r\nTime now: %s\r\n", FullDate);  // Print time on the hour.
  if ((currentLocalTime.tm_sec == 0 && currentLocalTime.tm_min == 0 &&
       currentLocalTime.tm_hour % 6 == 0) || forceRecomputeDusk) {
    forceRecomputeDusk = false;
    // The magic is here to get "on" time epoch,
    civilDuskEpoch = computeCivilDusk();

    // This is for changing the time for testing, only.
    // Set the value, far above.
    // Plus seconds for later, negative for earlier.
    civilDuskEpoch += onTimeAdj;
    lightsOffSecond = civilDuskEpoch + lightsOnDurationS;
  }
#if defined DEBUG_PRINTS
  Serial.printf("Civil dusk: %lu. Lights on until %lu\r\n",
                civilDuskEpoch, lightsOffSecond);
#endif
  // workTime is a time structure.  See details in these two places:
  // http://www.cplusplus.com/reference/ctime/tm/
  // http://www.cplusplus.com/reference/ctime/strftime/
  // Plug civilDuskEpoch seconds into the workTime
  //  struct to get human readable.
  workTime = *localtime(&civilDuskEpoch);
  // month (Jan = 0 -- adjust to human counting)
  dloN_Mon = workTime.tm_mon + 1;
  dloN_DOM = workTime.tm_mday;          // day of month
  dloN_Year = workTime.tm_year + 1900;  // year - 1900 to make TimeLib work.
  dloN_Hour = workTime.tm_hour;         // hour
  dloN_Min = workTime.tm_min;           // minute
  dloN_Sec = workTime.tm_sec;           // second

  if (displayOn) {
    display.clearDisplay();
    // Assumed: Lights off.
    display.setCursor(0, displayLine1); display.print("Lights are Off");
    display.setCursor(0, displayLine2); display.print(JustDate);
    display.setCursor(0, displayLine3); display.print(JustTime);
  }
  // Restart each Sunday at midnight.
  rebootTime = currentLocalTime.tm_hour + currentLocalTime.tm_min +
               currentLocalTime.tm_sec + currentLocalTime.tm_wday;
#if defined DEBUG_PRINTS
  Serial.printf("H%i M%i S%i D%i\r\n",
                currentLocalTime.tm_hour, currentLocalTime.tm_min,
                currentLocalTime.tm_sec, currentLocalTime.tm_wday);
  Serial.printf("rebootTime indicator %i\r\n", rebootTime);
  This varies from 0 to 147.  Only 1 "0" and "147",
  but multiples of the others.
#endif
  if (rebootTime == 0) ESP.restart();

  if (displayOn) {
    display.setCursor(0, displayLine4); display.print("Lights On:");
    // Show Delayed Lights on time (Civil dusk).
    display.setCursor(0, displayLine5);
    display.printf("%i:%02i:%02i - ", dloN_Hour, dloN_Min, dloN_Sec);
    workTime = *localtime(&lightsOffSecond);
    display.printf("%i:%02i:%02i",
                   workTime.tm_hour, workTime.tm_min, workTime.tm_sec);
    display.display();
  }

  //  Serial.printf("Loop lightsOffSecond %lu\r\n", lightsOffSecond);
  //  workTime = *localtime(&lightsOffSecond);
  // month (Jan = 0 -- adjust to human counting)
  //  dloFF_Mon = workTime.tm_mon + 1;
  //  dloFF_DOM = workTime.tm_mday;          // day of month
  //  dloFF_Year = workTime.tm_year + 1900;  // year - 1900. No idea why!
  //  dloFF_Hour = workTime.tm_hour;         // hour
  //  dloFF_Min = workTime.tm_min;           // minute
  //  dloFF_Sec = workTime.tm_sec;           // second

  // Show Delayed Lights off time.
  //  if (displayOn) {
  //    display.printf("%i:%02i:%02i", dloFF_Hour, dloFF_Min, dloFF_Sec);
  //    display.printf("%i:%02i:%02i",
  //                   workTime.tm_hour, workTime.tm_min, workTime.tm_sec);
  //    display.display();
  //  }

  //  Serial.printf("nowEpoch %lu, civilDuskEpoch %lu, "
  //                "lightsOffSecond %lu\r\n",
  //                nowEpoch, civilDuskEpoch, lightsOffSecond);

  // Compare current, local time epoch with Civil Dusk time.
  // See if the turn on time window has arrived.
  if (nowEpoch >= civilDuskEpoch && nowEpoch < lightsOffSecond) {
    adjustForLateOn = nowEpoch - civilDuskEpoch;
    //    Serial.println("We are in the lights on time window.");
    if (adjustForLateOn < 1)
      Serial.println("Lights coming on on time");
    else
      Serial.printf("Lights coming on %i seconds late.\r\n",
                    adjustForLateOn);
    Serial.println(FullDate);
    Serial.println("Issuing the switch command. "
                   "(On and Off are the same code.)");
    mySwitch.send("101010101111111111101000");  // This one turns lights on.
    lightsAreOn = true;
    WaitForOff();
    lightsAreOn = false;
    getDateTimeInfo();  // Calc time now into human readable.
    Serial.print(FullDate);
    Serial.println(" - Issuing the off command.");
    mySwitch.send("101010101111111111101000");  // This one turns them off.
  }
}
/***************************************************************************/
void WaitForOff()
/****WaitForOff*************************************************************/
{
  int remainingSeconds;
  char buf[20];
  int  hrRemaining, minRemaining, secRemaining;
  int prev_min = 100;
  int prev_sec = 100;

#ifdef DO_LED
  digitalWrite(LED_BUILTIN, HIGH);
#endif

  getDateTimeInfo();
  while (lightsOffSecond > nowEpoch) {
    handleUserReq();  // Input of different on-time duration. In minutes.
    getDateTimeInfo();
    button.loop();
    if (button.wasPressed()) {
      switch (button.read()) {
        case long_click:
          Serial.println("Button Long press");
          displayTurnOffTime = millis() + displayOnDuration;
          displayOn = true;
          break;
          // case double_click: Serial.println("double"); break;
          // case triple_click: Serial.println("triple"); break;
      }
    }
    if (displayOn) {
      if (millis() > displayTurnOffTime) {
        display.clearDisplay();
        display.display();
        displayOn = false;
      }
    }
    if (digitalRead(displayOnSolid) == 0) {
      displayTurnOffTime = millis() + displayOnDuration;
      displayOn = true;
    }
    // Update once per second, only.
    if (prev_sec != currentLocalTime.tm_sec) {
      prev_sec = currentLocalTime.tm_sec;
      getDateTimeInfo();

      // Number of on seconds remaining.
      lightsOffSecond = civilDuskEpoch + lightsOnDurationS;
      remainingSeconds = lightsOffSecond - nowEpoch;

      secRemaining = int(remainingSeconds % 60);
      minRemaining = int((remainingSeconds / 60) % 60);
      hrRemaining  = int((remainingSeconds / 3600) % 24);

      // Cheap char array clear.
      timeRemaining[0] = '\0';  // Build time remaining string here.
      //Get hour(s) remaining
      if (hrRemaining > 0) {
        sprintf(buf, "%i", hrRemaining);
        strcat(timeRemaining, buf);
        strcat(timeRemaining, " hour");
        if (hrRemaining != 1) strcat(timeRemaining, "s");
        if (minRemaining > 0 || (secRemaining > 0 && hrRemaining == 0))
          strcat(timeRemaining, ", ");
      }
      //Get minute(s) remaining
      if (minRemaining > 0) {
        sprintf(buf, "%i", minRemaining);
        strcat(timeRemaining, buf);
        strcat(timeRemaining, " min");
        if (minRemaining != 1) strcat(timeRemaining, "s");
      }
      //Get second(s) remaining
      if (hrRemaining == 0 && secRemaining > 0) {
        if (minRemaining > 0) strcat(timeRemaining, ", ");
        sprintf(buf, "%i", secRemaining);
        strcat(timeRemaining, buf);
        strcat(timeRemaining, " sec");
        if (secRemaining != 1) strcat(timeRemaining, "s");
      }
      if (displayOn) {
        display.clearDisplay();
        display.setTextSize(1);

        display.setTextColor(WHITE);
        display.setCursor(0, displayLine1);
        display.print("Lights are On");
        display.setCursor(0, displayLine2);
        display.print(JustDate);
        display.setCursor(0, displayLine3);
        display.print(JustTime);
        display.setCursor(0, displayLine4);
        display.print("Lights Off in");
        display.setCursor(0, displayLine5);
        display.print(timeRemaining);
        display.display();
      }
      if (prev_min != currentLocalTime.tm_min) {  // Once per minute, only.
        prev_min = currentLocalTime.tm_min;
        Serial.print("Lights are On. Now ");
        Serial.println(FullDate);
        Serial.print("Lights Off in ");
        Serial.print(timeRemaining);
        //        Serial.printf("On wait lightsOffSecond %lu\r\n",
        //                       lightsOffSecond);
        workTime = *localtime(&lightsOffSecond);
        Serial.printf(" at: %02i:%02i:%02i\r\n--------\r\n",
                      workTime.tm_hour, workTime.tm_min, workTime.tm_sec);
      }
      myWait(50);
    }
  }
#ifdef DO_LED
  digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the pin LOW
#endif
}
