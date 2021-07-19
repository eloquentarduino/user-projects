#include "LDR.h"

#define NUM_SENSORS 24
#define WINDOW_IN_SAMPLES 100
#define NUMBER_OF_INPUTS (NUM_SENSORS * WINDOW_IN_SAMPLES)


LDR ldr;
String command;
float inputs[NUMBER_OF_INPUTS] = {0};


/**
 * 
 */
void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Begin");

  ldr.setMuxPins(12, 11, 10, 9);
  ldr.setAnalogInput1(A0);
  ldr.setAnalogInput2(A2);
  ldr.setNumberOfInputs1(16);
  ldr.setNumberOfInputs2(8);
  ldr.setFrequency(200);
  ldr.setMaxValue(100);
  ldr.setStartOfTouchThreshold(40);
  ldr.setStartOfTouchTriggers(6);

  Serial.println("Initialization done");
  Serial.println("* Send 'calibrate' to calibrate the sensors");
  Serial.println("* Send 'raw' to read the raw values");
  Serial.println("* Send 'read' to read the calibrated values");
  Serial.println("* Send 'record' to start recording samples of a gesture");
  Serial.println("* Send anything else to stop");
}


/**
 * 
 */
void loop() {
  if (Serial.available())
    command = Serial.readStringUntil('\n');


  if (command == "calibrate") {
    command = "";
    Serial.println(ldr.calibrate() ? "Calibration done" : "Calibration error - you must configure the MUX first");
  }

  else if (command == "raw") {
    ldr.read();
    ldr.print();
  }

  else if (command == "read") {
    ldr.readCalibrated();
    ldr.print();
  }

  else if (command == "record") {
    while (!ldr.detectStartOfTouch())
      ; // await for start of gesture

    // read WINDOW_IN_SAMPLES times the LDR
    for (uint16_t t = 0; t < WINDOW_IN_SAMPLES; t++) {
      ldr.readCalibrated();
      memcpy(inputs + t * NUM_SENSORS, ldr.readings, sizeof(float) * NUM_SENSORS);
    }

    // print the readings
    for (uint16_t i = 0; i < NUMBER_OF_INPUTS; i++) {
      Serial.print(inputs[i]);
      Serial.print(i == NUMBER_OF_INPUTS - 1 ? '\n' : ',');
    }

    delay(1000);
  }

  else {
    command = "";
  }

}
