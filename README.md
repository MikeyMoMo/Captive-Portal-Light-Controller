# Captive-Portal-Light-Controller
This demonstrates an ESP32 controlling a little RF transmitter to turn lights on at dusk and off some time later.

The little transmitter is totally controlled by this program to send the needed code.  The code is in the library RCSwitch.  It can be found in the Manage Libraries part of the Arduino IDE or here on Github.

The needed code to transmit was obtained by a different program.  I don't have the name of that program right now.  If I find it, I will update this.  I think I know who knows.

The software to decode the signal you want to replicate is here: https://randomnerdtutorials.com/rf-433mhz-transmitter-receiver-module-with-arduino/ but there are other options.  There are Arduino libraries that claim to receive and decode signals, too.  Choose the one you like.

This was uploaded to show how to present time in a nice way with "s"s added only when needed and drop out elements of the time that are zero.  The code works if you have the RCSwitch library, the RF Transmitter
and something to receive the code and have determined the code.  It is not expected that it will run without some effort but if you get all the parts together, it will turn your lights on at disk (real or civil)
and turn them off some time later.  The times are calculated for your lat/long and will vary each day, as expected.

![image](https://github.com/MikeyMoMo/Captive-Portal-Light-Controller/assets/15792417/c9dc552f-9849-4929-968b-3576c5b166e0)
