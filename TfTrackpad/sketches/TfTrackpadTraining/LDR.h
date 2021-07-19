#pragma once


#include <CD74HC4067.h>


class LDR {
  public:
    float readings[32];

    /**
     * Constructor
     */
    LDR() :
      interval(5),
      maxValue(100),
      thresh(40),
      triggers(4) {
        
      }

    /**
     * Set the MUX control pins
     */
    void setMuxPins(uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3) {
      mux = new CD74HC4067(s0, s1, s2, s3);
    }

    /**
     * Set first analog input pin
     */
    void setAnalogInput1(uint8_t pin) {
      a0 = pin;
    }

    /**
     * Set second analog input pin
     */
    void setAnalogInput2(uint8_t pin) {
      a1 = pin;
    }

    /**
     * Set number of inputs from first MUX
     */
    void setNumberOfInputs1(uint8_t n) {
      n0 = n;
    }

    /**
     * Set number of inputs from second MUX
     */
    void setNumberOfInputs2(uint8_t n) {
      n1 = n;
    }

    /**
     * Set sampling frequency
     */
    void setFrequency(uint16_t freq) {
      interval = 1000 / freq;
    }

    /**
     * Set min value to detect a cell as "triggered"
     */
    void setStartOfTouchThreshold(uint16_t t) {
      thresh = t;
    }

    /**
     * Set number of cells that must "trigger" to detect start of motion
     */
    void setStartOfTouchTriggers(uint8_t t) {
      triggers = t;
    }

    /**
     * Set max value to scale the raw readings
     */
    void setMaxValue(uint16_t value) {
      maxValue = value;
    }

    /**
     * Read sensors
     */
    bool read() {
      if (!mux)
        return false;
        
      for (uint8_t i = 0; i < max(n0, n1); i++) {
        mux->channel(i);

        if (i < n0)
          readings[i] = analogRead(a0);
    
        if (i < n1)
          readings[n0 + i] = analogRead(a1);
      }
    
      delay(interval);

      return true;
    }

    /**
     * Compute "base" value of ambient light
     */
    bool calibrate(uint8_t repeat = 30) {
      if (!mux)
        return false;
        
      for (uint8_t i = 0; i < 32; i++)
        zeros[i] = 0;
        
      // read sensors n times
      for (uint8_t k = 0; k < repeat; k++) {
        read();
        
        for (uint8_t i = 0; i < n0 + n1; i++)
          zeros[i] += readings[i];
      }
    
      // average readings
      for (uint8_t i = 0; i < n0 + n1; i++)
        zeros[i] /= repeat;

      return true;
    }

    /**
     * Get readings relative to "base" value, in the range [0, 1]
     */
    bool readCalibrated() {
      if (!mux)
        return false;
        
      read();

      for (uint8_t i = 0; i < n0 + n1; i++)
        readings[i] = constrain(zeros[i] - readings[i], 0, maxValue) / ((float) maxValue);

      return true;
    }

    /**
     * Simple "start of touch" routine
     * (look for a strong value from a few sensors)
     */
    bool detectStartOfTouch() {
      uint8_t triggers = 0;
      float thresh = (1.0f * this->thresh) / maxValue;

      readCalibrated();
      
      for (int i = 0; i < n0 + n1; i++) {
        if (readings[i] > thresh)
          triggers += 1;
    
        // "strong" trigger
        if (readings[i] > max(0.8, thresh))
          triggers += 1;
      }
    
      return triggers >= this->triggers;
    }

    /**
     * Print readings
     */
    void print() {
      for (uint8_t i = 0; i < n0 + n1; i++) {
        Serial.print(readings[i]);
        Serial.print(i == (n0 + n1 - 1) ? '\n' : ',');
      }
    }


  protected:
    CD74HC4067 *mux;
    uint8_t a0;
    uint8_t a1;
    uint8_t n0;
    uint8_t n1;
    uint16_t interval;
    uint16_t maxValue;
    uint16_t thresh;
    uint8_t triggers;
    float zeros[32];
};
