/***************************************************************************/
void handleUserReq()
/****handleUserReq**********************************************************/
{
  // Although this routine will function without any line ending character,
  //  it requires either of them (both are OK, too) to tell it that the
  //  input is done and to do feedback to the display about the change of
  //  lights on time.  It will work without an line end character but the
  //  display will not be updated.

  if (SerialBT.available()) {
    OnTimeInputSource = OnTimeInputSourceBT;
    memset(cUserReq, 0, sizeof(cUserReq));
    cUserReq[0] = toupper(SerialBT.read());  // Read input character
#if defined DEBUG_PRINTS
    Serial.print("BT input byte: ");
    Serial.write(cUserReq[0]); Serial.print("  ");
#endif
  }
  else if (Serial.available()) {
    OnTimeInputSource = OnTimeInputSourceSerial;
    memset(cUserReq, 0, sizeof(cUserReq));
    cUserReq[0] = toupper(Serial.read());  // Read input character
#if defined DEBUG_PRINTS
    Serial.print("Serial input byte: ");
    Serial.write(cUserReq[0]); Serial.println();
#endif
  }

  if (strlen(cUserReq) == 0) return;  // Really should never be 0 length.

  if (cUserReq[0] == 0x0a || cUserReq[0] == 0x0d) {  // Got either line end?
    if (skipTillEOL) {
#if defined DEBUG_PRINTS
      Serial.println("skipTillEOL = false");
#endif
      skipTillEOL = false;  // No more skipping
      memset(cUserReq, 0, sizeof(cUserReq));  // Clear both work areas.
      memset(cInputCvtBuffer, 0, sizeof(cInputCvtBuffer));
      return;
    }
    if (eolReceived) {  // If I got one already, ignore this one.
      memset(cInputCvtBuffer, 0, sizeof(cInputCvtBuffer));
      memset(cUserReq, 0, sizeof(cUserReq));
      return;  // I got one of these already, since seeing a digit.
    }
    eolReceived = true;
    display.setTextSize(1);
    display.clearDisplay();
    display.setCursor(14, displayLine1);
    display.print("On time updated to");
    display.setTextSize(3);
    display.setCursor(40, displayLine2);
    display.print(lightsOnDurationM);
    display.setCursor(40, displayLine4);
    display.setTextSize(1);
    display.print("minutes.");
    display.display();
#if defined DEBUG_PRINTS
    Serial.printf("EOL found. Lights on for %i secs.\r\n",
                  lightsOnDurationS);
#endif
    if (OnTimeInputSource == OnTimeInputSourceBT)
      SerialBT.printf("Lights will be on for %i minutes.\r\n",
                      lightsOnDurationM);
    else
      Serial.printf("Lights will be on for %i minutes.\r\n",
                    lightsOnDurationM);
    myWait(5000);
    displayTurnOffTime = millis() + displayOnDuration;
    displayOn = true;
    display.setTextSize(1);
    if (preferences.getInt("onMinutes", 180) != lightsOnDurationM) {
      forceRecomputeDusk = true;
      preferences.putInt("onMinutes", lightsOnDurationM);
    }
    // Clear the work buffers.
    memset(cInputCvtBuffer, 0, sizeof(cInputCvtBuffer));
    memset(cUserReq, 0, sizeof(cUserReq));

    return;
  }

  // Not an EOL, so validate the input character and do the right action.
  if (cUserReq[0] == 'Q') {
    eolReceived = false;  // Got a Q, not EOL.
    if (OnTimeInputSource == OnTimeInputSourceBT) {
      SerialBT.printf("Lights Controller Version %s\r\n", pgmVer);
      SerialBT.print("IP Address: "); SerialBT.println(WiFi.localIP());
      SerialBT.printf("Lights are %s\r\n", lightsAreOn ? "on" : "off");
      if (!lightsAreOn) {  // If off, tell lights on time.
        SerialBT.print("Lights on at ");
        SerialBT.printf("%i:%02i\r\n", dloN_Hour, dloN_Min);
      }
      memset(cInputCvtBuffer, 0, sizeof(cInputCvtBuffer));
      memset(cUserReq, 0, sizeof(cUserReq));
    } else {
      Serial.printf("Lights Controller Version %s\r\n", pgmVer);
      Serial.print("IP Address: "); Serial.println(WiFi.localIP());
      Serial.printf("Lights are %s\r\n", lightsAreOn ? "on" : "off");
      if (!lightsAreOn) {  // If off, tell lights on time.
        Serial.print("Lights on at ");
        Serial.printf("%i:%02i\r\n", dloN_Hour, dloN_Min);
      }
      memset(cInputCvtBuffer, 0, sizeof(cInputCvtBuffer));
      memset(cUserReq, 0, sizeof(cUserReq));
    }
    skipTillEOL = true;  // Flush anything else.
    //    Serial.println("skipTillEOL = true");
    return;
  }

  if (skipTillEOL) return;

  if (!isdigit(cUserReq[0])) {
    Serial.println("Input not a number, not Q, input cleared.");
    memset(cInputCvtBuffer, 0, sizeof(cInputCvtBuffer));
    memset(cUserReq, 0, sizeof(cUserReq));
    return;
  }
#if defined DEBUG_PRINTS
  Serial.println("Received a digit.");
#endif
  eolReceived = false;  // Got a number, keep reading till EOL.
  strcat(cInputCvtBuffer, cUserReq);  // Continue building up the number.
  memset(cUserReq, 0, sizeof(cUserReq));
#if defined DEBUG_PRINTS
  Serial.printf("Length of cInputCvtBuffer %i\r\n",
                strlen(cInputCvtBuffer));
  Serial.print("Char input buffer: ");
  Serial.write(cInputCvtBuffer); Serial.println();
#endif
  lightsOnDurationM = atoi(cInputCvtBuffer);  // On time duration in minutes
  if (lightsOnDurationM > 720 || lightsOnDurationM == 0) {
    if (OnTimeInputSource == OnTimeInputSourceBT) {
      SerialBT.printf("Minutes value out of range: %i. Not changed.\r\n",
                      lightsOnDurationM);
      //Reset to last valid value or 180 if not yet set.
      lightsOnDurationM = preferences.getInt("onMinutes", 180);
    } else {
      Serial.printf("Minutes value out of range: %i. Not changed.\r\n",
                    lightsOnDurationM);
      //Reset to last valid value or 180 if not yet set.
      lightsOnDurationM = preferences.getInt("onMinutes", 180);
    }
    memset(cInputCvtBuffer, 0, sizeof(cInputCvtBuffer));
    memset(cUserReq, 0, sizeof(cUserReq));
    //    Serial.println("skipTillEOL = true");
    skipTillEOL = true;  // Yes, it is true. Input error.
    return;
  }
  lightsOnDurationS = lightsOnDurationM * 60;  // On time duration in seconds
#if defined DEBUG_PRINTS
  Serial.printf("Input seconds at exit %i\r\n", lightsOnDurationS);
#endif
}
/***************************************************************************/
void BTcallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
/***************************************************************************/
{
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("BT Client Connected");
    SerialBT.println("I C U\r\nEnter lights on time in minutes or Q.");
  }
  if (event == ESP_SPP_CLOSE_EVT)
    Serial.println("BT Client Disconnected");
}
/***************************************************************************/
int isNumber(char s[])
/***isNumber****************************************************************/
{
  for (int i = 0; s[i] != '\0'; i++)
  {
    if (isdigit(s[i]) == 0) return false;
  }
  return true;
}
/***************************************************************************/
time_t computeCivilDusk()
/******computeCivilDusk*****************************************************/
{
  time_t localEpoch, GMTnowEpoch;
  int CDuskH, CDuskM;

  setenv("TZ", "GMT0", 1);
  tzset(); // Set the TZ environment variable
  getLocalTime(&workTime);  // Update the ts structure.
  // No DST in GMT so the last parm is always 0.
  GMTnowEpoch = getEpochFromDateTime(2022, 1, 1, 1, 1, 1, 0);
#if defined CONFIG_FOR_US
  setenv("TZ", "PST8PDT,M3.2.0,M11.1.0", 1);  // California Riviera
#else
  setenv("TZ", "PHT-8", 1);                   // Philippine Sweat Shop ;-))
#endif
  tzset(); // Set the TZ environment variable
  getLocalTime(&workTime);  // Update the ts structure.
  localEpoch = getEpochFromDateTime(2022, 1, 1, 1, 1, 1, workTime.tm_isdst);
  int CivilTimeOffset = (GMTnowEpoch - localEpoch) / 3600;
  //  Serial.printf("Civil Time offset %i\r\n", CivilTimeOffset);
  // Originally, I had to do the following but it has worked it out, itself.
  // For RP, Hong Kong, eastern China, +8.  For California, -8.
  //  if (GMTnowEpoch < localEpoch) CivilTimeOffset = 0 - CivilTimeOffset;
  //  Serial.printf("Corrected Civil Time offset %i\r\n", CivilTimeOffset);
  //  Serial.printf("TZ offset from GMT to use getting "
  //                "Civil Twilight is %i\r\n", CivilTimeOffset);

  calcSunriseSunset(currentLocalTime.tm_year + 1900,
                    currentLocalTime.tm_mon + 1,
                    currentLocalTime.tm_mday,
                    latitude, longitude, Ctransit, Csunrise, Csunset);
  calcCivilDawnDusk(currentLocalTime.tm_year + 1900,
                    currentLocalTime.tm_mon + 1,
                    currentLocalTime.tm_mday,
                    latitude, longitude, Ctransit, Cdawn, Cdusk);

  // Print the resulting oddly formatted times.
  // All but Cdusk are just for show.  Not used in the program.
  Cdawn    += CivilTimeOffset;
  Serial.printf("Civil dawn: %02i:%02i\r\n", int(Cdawn),
                int((Cdawn - int(Cdawn)) * 60));

  Csunrise += CivilTimeOffset;
  Serial.printf("Sunrise:    %02i:%02i\r\n", int(Csunrise),
                int((Csunrise - int(Csunrise)) * 60));

  Ctransit += CivilTimeOffset;
  Serial.printf("Transit:    %02i:%02i\r\n", int(Ctransit),
                int((Ctransit - int(Ctransit)) * 60));

  Csunset  += CivilTimeOffset;
  Serial.printf("Sunset:     %02i:%02i\r\n", int(Csunset),
                int((Csunset - int(Csunset)) * 60));

  Cdusk    += CivilTimeOffset;
  Serial.printf("Civil dusk: %02i:%02i\r\n", int(Cdusk),
                int((Cdusk - int(Cdusk)) * 60));

  CDuskM = int(round(Cdusk * 60));
  CDuskH = (CDuskM / 60) % 24; CDuskM = CDuskM % 60;
  return (getEpochFromDateTime(currentLocalTime.tm_year + 1900,
                               currentLocalTime.tm_mon + 1,
                               currentLocalTime.tm_mday,
                               CDuskH, CDuskM, 0,
                               currentLocalTime.tm_isdst));
  //  return (CDuskEpoch);
}
/***************************************************************************/
void getDateTimeInfo()
/****getDateTimeInfo********************************************************/
{
  /*
    Member    Type  Meaning Range
    tm_sec    int   seconds after the minute  0-60*
    tm_min    int   minutes after the hour  0-59
    tm_hour   int   hours since midnight  0-23
    tm_mday   int   day of the month  1-31
    tm_mon    int   months since January  0-11
    tm_year   int   years since 1900
    tm_wday   int   days since Sunday 0-6
    tm_yday   int   days since January 1  0-365
    tm_isdst  int   Daylight Saving Time flag
  */

  getLocalTime(&currentLocalTime);  // Update the currentLocalTime structure.
  strftime(FullDate, 100, "%c", &currentLocalTime);
  strftime(JustDate, 100, "%a %b %e, %G", &currentLocalTime);
  //  nowEpoch = getEpochFromDateTime(currentLocalTime.tm_year + 1900,
  //                                  currentLocalTime.tm_mon + 1,
  //                                  currentLocalTime.tm_mday,
  //                                  currentLocalTime.tm_hour,
  //                                  currentLocalTime.tm_min,
  //                                  currentLocalTime.tm_sec,
  //                                  currentLocalTime.tm_isdst);

  time(&nowEpoch);  // Unix epoch for now.

  //  Serial.printf("Calculated (current) epoch %lu\r\n", nowEpoch);
  int tempHour = currentLocalTime.tm_hour;
  if (tempHour > 11)
    myIsPM = true;
  else
    myIsPM = false;

  if (showAMPM)
  {
    if (tempHour == 0) tempHour += 12;
    if (tempHour > 12) tempHour -= 12;
    sprintf(JustTime, "%2i", tempHour);
  } else {
    sprintf(JustTime, "%02d", tempHour);
  }
  strcat(JustTime, ":");
  sprintf(workDate, "%02i", currentLocalTime.tm_min);
  strcat(JustTime, workDate); strcat(JustTime, ":");
  sprintf(workDate, "%02i", currentLocalTime.tm_sec);
  strcat(JustTime, workDate);

  if (showAMPM) {
    if (currentLocalTime.tm_hour > 11) {
      sprintf(workDate, "%s", " PM");
      strcat(JustTime, workDate);
    } else {
      sprintf(workDate, "%s", " AM");
      strcat(JustTime, workDate);
    }
  }
}
/***************************************************************************/
unsigned long getEpochFromDateTime(int year, int month, int day,
                                   int hour, int minute, int second,
                                   int dst)
/*************getEpochFromDateTime******************************************/
{
  // Pass in the human version of year, i.e. 2022 and month where Jan = 1.

  //  Serial.printf("Calcing epoch for %i/%02i/%02i %i:%02i:%02i\r\n",
  //                year,  month,  day, hour,  minute,  second);
  struct tm t;
  time_t t_of_day;

  t.tm_year = year - 1900;  // Year - 1900
  t.tm_mon = month - 1;     // Month, where 0 = Jan, the way TimeLib does it.
  t.tm_mday = day;          // Day of the month
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = second;
  t.tm_isdst = dst;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
  t_of_day = mktime(&t);

  //  printf("seconds since the Epoch: % ld\r\n", (long) t_of_day);
  return (t_of_day);
}
/***************************************************************************/
// The following is not used since it devolved into one statement!  If you
//  want to get the feedback from the print, call this until correct, then
//  just use the first statement inline in your code.  Left in for reference.
//void getDateTimeFromEpoch(time_t myEpoch)
///****getDateTimeFromEpoch*************************************************/
//{
//  workTime = *localtime(&myEpoch);
//  Serial.printf("Time / date from Epoch is: %i/%i/%i%i:%i:%i\r\n",
//                workTime.tm_mon + 1, workTime.tm_mday,
//                workTime.tm_year + 1900,
//                workTime.tm_hour, workTime.tm_min,
//                workTime.tm_sec);
//  return;
//}
/***************************************************************************/
void myWait(unsigned long waitMillis)
/****myWait*****************************************************************/
{
  unsigned long startMillis = millis();
  // wait approx. [waitMillis] ms, then return
  while (millis() < startMillis + waitMillis) yield();
  return;
}
//int getTimeOffsetFromENV(const char TZENV[]) {
//
//  bool nonAlphaFound;
//  int  offsetTime, offsetLen, parseLen, numBegin;
//  int  i, j;
//  char tempChar[10] ;
//
//  nonAlphaFound = false;
//  offsetLen = 0;
//  parseLen = strlen(TZENV);  // How many characters left after " = " sign.
//  Serial.printf("parseLen %i\r\n", parseLen);
//  for (j = 0; j < parseLen; j++) {
//    if (!is_alpha(TZENV[j])) {
//      if (!nonAlphaFound) { // Still false?  First one found, remember it.
//        nonAlphaFound = true;
//        numBegin = j;  // Save the first non-alpha found.
//        Serial.printf("numBegin %i\r\n", numBegin);
//      }
//      offsetLen++;
//      Serial.printf("offsetLen %i\r\n", offsetLen);
//    } else {
//      if (nonAlphaFound) break;  // End of time offset portion.
//    }
//  }
//  memset(tempChar, 0, 10);
//  memcpy(tempChar, TZENV + numBegin, offsetLen);
//  Serial.printf("tempChar %s\r\n", tempChar);
//  offsetTime = atoi(tempChar);
//  Serial.printf("Offset time is %i in %s\r\n", offsetTime, TZENV);
//  return offsetTime;
//}
