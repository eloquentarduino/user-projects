#include <EloquentTinyML.h>
#include "LDR.h"
#include "nn.h"

#define NUM_SENSORS 24
#define WINDOW_IN_SAMPLES 100
#define NUMBER_OF_INPUTS (NUM_SENSORS * WINDOW_IN_SAMPLES)
#define NUMBER_OF_OUTPUTS 6
#define TENSOR_ARENA_SIZE 32*1024


LDR ldr;
String command;
float inputs[NUMBER_OF_INPUTS] = {0};
Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> nn;


String idxToLabel(uint8_t idx);


/**
 * 
 */
void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Begin");

  // configure the same as the TfTrackpadTraining sketch
  ldr.setMuxPins(12, 11, 10, 9);
  ldr.setAnalogInput1(A0);
  ldr.setAnalogInput2(A2);
  ldr.setNumberOfInputs1(16);
  ldr.setNumberOfInputs2(8);
  ldr.setFrequency(200);
  ldr.setMaxValue(100);
  ldr.setStartOfTouchThreshold(40);
  ldr.setStartOfTouchTriggers(4);

  // initialize the neural network
  if (!nn.begin(model_data)) {
    Serial.print("NN initialization error: ");
    Serial.println(nn.errorMessage());

    while (true) delay(1000);
  }

  ldr.calibrate();
  Serial.println("Start predicting...");
}


/**
 * 
 */
void loop() {
  // await for start of gesture
  if (!ldr.detectStartOfTouch())
    return;

  Serial.print("Detected ");
  Serial.flush();

  // read window of readings
  for (uint16_t t = 0; t < WINDOW_IN_SAMPLES; t++) {
    ldr.readCalibrated();
    memcpy(inputs + t * NUM_SENSORS, ldr.readings, sizeof(float) * NUM_SENSORS);
  }

  // classify
  Serial.println(idxToLabel(nn.predictClass(inputs)));
  delay(1000);
}


/**
 * Convert class idx to readable name
 */
String idxToLabel(uint8_t idx) {
  switch (idx) {
    case 0: return "swipe right";
    case 1: return "swipe left";
    case 2: return "swipe up";
    case 3: return "swipe down";
    case 4: return "right tap";
    case 5: return "left tap";
    default: return "????";
  }
}
