/*
 *  Author: Claudiu Matei
 */

#ifndef Output_h
#define Output_h

#include <Arduino.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

class Output {
protected:
  int lastValue;

public:
  uint8_t pin;
  uint8_t initialValue = LOW;

  bool isPwm = false;

  void setup() {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, initialValue);
      lastValue = initialValue;
  }

  int getValue() {
    return lastValue;
  }
  
  void setValue(int value) {
    if (isPwm) {
      analogWrite(pin, value);
    } else {
      digitalWrite(pin, value);
    }
    
    lastValue = value;
  }

};


#endif

