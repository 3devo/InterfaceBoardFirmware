#include "ButtonEncoder.h"
#include "Arduino.h"
#include "Hardware.h"

template <>
volatile int8_t ButtonEncoder<ENC_SW, ENC_A, ENC_B>::encoder_detents = 0;
template <>
volatile uint8_t ButtonEncoder<ENC_SW, ENC_A, ENC_B>::button_presses = 0;

ISR(PCINT0_vect) {
  ButtonEncoder<ENC_SW, ENC_A, ENC_B>::button_isr();
}

ISR(PCINT1_vect) {
  ButtonEncoder<ENC_SW, ENC_A, ENC_B>::encoder_isr();
}
