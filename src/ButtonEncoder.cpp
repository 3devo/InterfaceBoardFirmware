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

#include "ButtonEncoder.h"
#include "Arduino.h"

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
