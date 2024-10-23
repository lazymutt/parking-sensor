// https://docs.sparkfun.com/SparkFun_Qwiic_TMF882X_Arduino_Library/

#include <Wire.h>
#include "SparkFun_TMF882X_Library.h"
#include "Qwiic_LED_Stick.h"
// #include <SparkFun_Alphanumeric_Display.h> // Additional optional debug display

// HT16K33 display;
SparkFun_TMF882X myTMF882X;
LED LEDStick;
static struct tmf882x_msg_meas_results myResults;

const int FIRSTLED = 0;
const int LASTLED = 10;
const byte REDARRAY[10] = { 255, 255, 255, 255, 255, 191, 128, 0, 0, 0 };
const byte GREENARRAY[10] = { 0, 64, 128, 191, 255, 255, 255, 255, 191, 64 };
const byte BLUEARRAY[10] = { 0, 0, 0, 0, 0, 0, 0, 128, 255, 255 };
const float INCH = 24.5;
const float FOOT = 304.8;
const unsigned long STOPLIGHT = 20000;
const byte LOWLEDLEVEL = 5;  // full brightness IS REALLY BRIGHT!!!
const byte HIGHLEDLEVEL = 31;

int averageDistance = 0;
unsigned long averagedConfidence = 0;
int averagedInches = 0;
byte previousStatus = 0;
unsigned long previousTiming = 0;
unsigned long previousHeartBeat = 0;
unsigned long runningDangerLight = 0;

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("");

  Serial.println("In setup");
  Serial.println("==============================");
  Wire.begin();  //Join the I2C bus

  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize the TMF882X device
  if (!myTMF882X.begin()) {
    Serial.println("Error - The TMF882X failed to initialize - is the board connected?");
    while (1)
      ;
  } else {
    Serial.println("TMF882X started.");
    myTMF882X.setSampleDelay(40);
  }

  if (LEDStick.begin() == false) {
    Serial.println("Qwiic LED Stick failed to begin. Please check wiring and try again!");
    while (1)
      ;
  } else {
    Serial.println("Qwiic LED Stick ready!");

    LEDStick.LEDOff();
    LEDStick.setLEDBrightness(LOWLEDLEVEL);
  }

  //   if (display.begin() == false) {
  //     Serial.println("Alphanumeric display did not acknowledge! Freezing.");
  //     while (1)
  //       ;
  //   }
  //   Serial.println("Alphanumeric Display acknowledged.");
}

void loop() {
  if (myTMF882X.startMeasuring(myResults)) {
    // onboard LED heartbeat! Is it still running?
    if (millis() - previousHeartBeat >= 3000) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      previousHeartBeat = millis();
    } else if (millis() - previousHeartBeat <= 0) {
      previousHeartBeat = millis();
    }

    averageDistance = 0;
    averagedConfidence = 0;

    for (int i = 0; i < myResults.num_results; ++i) {
      averageDistance += myResults.results[i].distance_mm;
      averagedConfidence += myResults.results[i].confidence;
    }

    averagedInches = (averageDistance / myResults.num_results) / INCH;
    averagedConfidence /= 9;

    if (averagedConfidence < 60) {  // Confidence too low, turn the lights off
      LEDStick.LEDOff();
      // display.clear(); // debugging display
    } else if (averagedInches >= 80) {  // Too far away, turn the lights off
      runningDangerLight = 0;
      LEDStick.LEDOff();
      // updateDisplayDual(myResults.results[5].distance_mm / INCH, averagedInches);  //For Alphanumeric display, remove when detached.
    } else if (averagedInches <= 30) {  // Inside the danger zone, flash the lights
      dangerLights();
      // updateDisplayDual(myResults.results[5].distance_mm / INCH, averagedInches);  //For Alphanumeric display, remove when detached.
    } else {  // set the lights based on range
      int mappedRange = map(averagedInches, 30, 80, 10, 0);  // Is this working the way we think it is?
      // https://www.arduino.cc/reference/en/language/functions/math/map/

      runningDangerLight = 0;
      setLights(mappedRange);
      // updateDisplayDual(myResults.results[5].distance_mm / INCH, averagedInches);  //For Alphanumeric display, remove when detached.
    }
  }
}

void dangerLights() {
  LEDStick.setLEDBrightness(HIGHLEDLEVEL);

  if (runningDangerLight == 0) {
    // Serial.println("Danger On.");
    // Really ought to check for millis overflow!
    runningDangerLight = millis() + 20000;
  } else if (millis() - runningDangerLight <= 0) {  // If millis() overflows
    runningDangerLight = millis();
  } else if (millis() >= runningDangerLight) {  // Only flash the lights for so long, then shut off.
    // Serial.println("Danger Off.");
    LEDStick.LEDOff();
    return;
  }

  // Flash the lights!!!
  for (byte count = 1; count <= 5; count++) {
    if (count % 2 == 0) {
      for (byte i = FIRSTLED; i <= LASTLED; i++) {
        if (i % 2 == 0) {
          LEDStick.setLEDColor(i, 255, 0, 0);
        } else {
          LEDStick.setLEDColor(i, 0, 0, 0);
        }
      }
    } else {
      for (byte i = FIRSTLED; i <= LASTLED; i++) {
        if (i % 2 == 0) {
          LEDStick.setLEDColor(i, 0, 0, 0);
        } else {
          LEDStick.setLEDColor(i, 255, 0, 0);
        }
      }
    }
    delay(55);
  }

  LEDStick.setLEDBrightness(LOWLEDLEVEL);
  return;
}

void setLights(byte maxLight) {
  // The first LED Stick was defective, so there some remaining logic to handle the non-working LEDs.

  LEDStick.setLEDBrightness(LOWLEDLEVEL); // Set the lights to a reasonable brightness.

  //Sleep the lights
  if (previousStatus == maxLight) {
    if ((millis() - previousTiming) >= STOPLIGHT) {
      LEDStick.LEDOff();
      return;
    }
  } else {
    previousStatus = maxLight;
    previousTiming = millis();
  }

  if (maxLight > 0) {
    for (int i = FIRSTLED; i <= 10; i++) {
      if (i <= maxLight) {
        LEDStick.setLEDColor(i, REDARRAY[maxLight], GREENARRAY[maxLight], BLUEARRAY[maxLight]);
      } else {
        LEDStick.setLEDColor(i, 0, 0, 0);
      }
    }
  }
}

// This function is for the extra debug display I used.

// void updateDisplayDual(short center, short average) {
//   int workingValue, centerA, centerB, averageA, averageB = 0;
//
//   display.clear();
//
//   if (center >= 100) {
//     display.printChar('+', 0);
//     display.printChar('+', 1);
//   } else {
//     workingValue = center / 10;
//     centerA = workingValue;
//     workingValue = center - (centerA * 10);
//     centerB = workingValue;
//     display.printChar('0' + centerA, 0);
//     display.printChar('0' + centerB, 1);
//   }
//
//   if (average >= 100) {
//     display.printChar('+', 2);
//     display.printChar('+', 3);
//   } else {
//     workingValue = average / 10;
//     averageA = workingValue;
//     workingValue = average - (averageA * 10);
//     averageB = workingValue;
//     display.printChar('0' + averageA, 2);
//     display.printChar('0' + averageB, 3);
//   }
//
//   display.updateDisplay();
// }
