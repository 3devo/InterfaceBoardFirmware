#pragma once

/*
 * Copyright (C) 2017-2025 3devo (http://www.3devo.eu)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "util/atomic.h"
#include "Arduino.h"
#include "Config.h"

/**
 * This class handles events from a rotary encoder with a combined
 * button, assuming external pullups are present on all pins.
 *
 * The encoder used has both outputs high when idle (at a detent), and
 * switches through a full cycle (both outputs go low and high again)
 * for each detent moved.
 *
 * When moving clock-wise, output A goes low before output B, and when
 * moving counter-clockwise, output B goes low before output A (and they
 * go high again in the same order).
 *
 * The moment you feel the knob "click" into the next detent matches the
 * moment where both outputs are high again. This is the moment where
 * events should be triggered.
 *
 *   Clockwise:            Counterclockwise:
 *   __.   ._____          ____.   ._____
 * A   |___|             A     |___|
 *   ____.   .___          __.   .___
 * B     |___|           B   |___|
 *           ^                     ^
 *           Event here            Event here
 *
 * To accurately catch these events, this class registers interrupt
 * handlers on both pins. It seems sufficient to only trigger on rising
 * edges to catch the above events, but then spurious events will be
 * triggered when the encoder is turned slightly past a detent and back
 * again (the forward movement will be missed, while the movement back
 * will trigger a spurious event).
 *
 * Hence, this class monitors both edges on both pins, and keeps track
 * of the exact value of the encoder (where each edge adds or subtracts
 * one to the value kept). Whenever the value is divisible by 4, the
 * encoder is at a detent.
 *
 * For tracking the encoder value, a lookup table approach is used. This
 * is based on the approach described (among other places) here:
 * https://www.circuitsathome.com/mcu/reading-rotary-encoder-on-arduino
 *
 * This uses a lookup table that maps a combination of the previous pin
 * states and the current pin states to the change in value that this
 * should result in. However, the lookup table used here is slightly
 * modified from the one commonly used.
 *
 * The original table contains 1 to indicate clockwise motion, -1 to
 * indicate counter-clockwise motion, 0 to indicate no motion (pin
 * values unchanged ) and also 0 to indicate an invalid transition (e.g.
 * both pins changed, which should never happen in quadrature encoding).
 *
 * The table used here, uses 2 and -2 for the invalid transitions
 * instead of 0. Even though these should never occur, it is not
 * unthinkable that bouncing or noise will very rarely cause them to
 * happen anyway. With the original table, these transitions did not
 * modify the value, but did modify the "previous pin state" that was
 * stored. Effectively, this could cause the tracked value to get an
 * off-by-two error relative to the actual position. Effectively, these
 * invalid transitions mean the encoder position has changed by 2
 * instead of 1, though there is no way to tell which way the encoder
 * has turned.
 *
 * For this reason, the lookup table contains 2 (and -2, to allow any
 * errors to possibly even out) for these invalid transitions. This will
 * keep the value accurate in some cases, but since we're just making a
 * guess as to the direction, it will by off-by-four in other cases.
 * Even though not perfectly accurate, now at least the assumption that
 * the tracked value is a multiple of four will remain valid, regardless
 * of what values are read.
 *
 * This approach means that the reading is pretty resilient against
 * invalid readings. When there is an invalid reading on a single pin,
 * the value might be temporary off-by-one, but this will be corrected
 * when the reading is correct again. Since normally, only one pin
 * should change at a time, this means that any bouncing of the
 * encoder's contacts should be handled without any problems by this
 * code.
 *
 * When both pins return an invalid reading, the absolute position might
 * get off-by-four errors, but the position within a single cycle should
 * remain correct, meaning detents are still detected correctly.
 */

void assert_interrupt_pin();
void clear_interrupt_pin();

template <uint8_t button_pin, uint8_t pina, uint8_t pinb>
class ButtonEncoder {
  public:
    void setup(void) {
      // These have external pullups
      pinMode(button_pin, INPUT);
      pinMode(pina, INPUT);
      pinMode(pinb, INPUT);

      // Set Pin Change Interrupts Masks that are used for the encoder
      PCMSK0 = _BV(PCINT5);
      PCMSK1 = _BV(PCINT10);

      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      // Be sure that every interrupt has been set, including all the pin change interrupts
      GIMSK |= (_BV(PCIE1) | _BV(PCIE0));

      attachInterrupt(digitalPinToInterrupt(pinb), encoder_isr, CHANGE);

      // Reset any queued events. attachInterrupt does not clear any
      // previously pending interrupts (e.g. due to startup noise, or
      // due to different interrupt settings), so these might trigger
      // right away. Clear away the result of that. See also
      // https://github.com/arduino/Arduino/issues/510
      GIFR |= (_BV(INTF0) | _BV(PCIF1) | _BV(PCIF0));
      }
    }

    int8_t process_encoder(void) {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        int8_t ret = encoder_detents;
        encoder_detents = 0;
        return ret;
      }
      return 0; // Not reached, but keeps compiler happy
    }

    uint8_t process_button() {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        uint8_t ret = button_presses;
        button_presses = 0;
        return ret;
      }
      return 0; // Not reached, but keeps compiler happy
    }

    // Interrupt routine functions
    static void button_isr(void) __attribute__((__always_inline__));
    static void encoder_isr(void) __attribute__((__always_inline__));

    static volatile uint8_t button_presses;
    static volatile int8_t encoder_detents;
};

template <uint8_t button_pin, uint8_t pina, uint8_t pinb>
inline void ButtonEncoder<button_pin, pina, pinb>::button_isr(void) {
  static bool previous_state = true;
  static uint32_t prev_edge_time = 0;

  if (digitalRead(button_pin) == previous_state)
    return;
  
  uint32_t now = micros();
  // Debounce is needed, even with hardware debounce filter some bouncing still
  // happens (probably due to noise when the filter is at an indeterminate voltage).
  if ((now - prev_edge_time) > DEBOUNCE_TIME_US) {
    // Falling edge detection
    if (previous_state) {
      button_presses++;
      assert_interrupt_pin();
    }
  }
  prev_edge_time = now;
  previous_state = !previous_state;
}

template <uint8_t button_pin, uint8_t pina, uint8_t pinb>
inline void ButtonEncoder<button_pin, pina, pinb>::encoder_isr(void) {
  // Initialize to 3, assuming the encoder starts at a detent. This
  // matches the initial value of `value`, so even if it is
  // incorrect, it should resynchronise on the first interrupt.
  static uint8_t prev_reading = 3;
  static int8_t encoder_value = 0;

  // This lookup table maps clockwise rotation to positive changes.
  static const int8_t table[] PROGMEM = { 0, -1, 1, 2, 1, 0, 2, -1, -1, -2, 0, 1, -2, 1, -1, 0 };
  uint8_t reading = digitalRead(pina) << 1 | digitalRead(pinb);
  uint8_t key = prev_reading << 2 | reading;
  encoder_value += pgm_read_byte(&table[key]);
  prev_reading = reading;

  // Generate an event whenever the encoder has moved a full
  // detent (value changed by 4).  
  if (encoder_value >= 4) {
    ++encoder_detents;
    encoder_value -= 4;
    assert_interrupt_pin();
  } else if (encoder_value <= -4) {
    --encoder_detents;
    encoder_value += 4;
    assert_interrupt_pin();
  }
}
