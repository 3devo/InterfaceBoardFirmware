/*
 * Copyright (C) 2017 3devo (http://www.3devo.eu)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "Arduino.h"

static const uint8_t H_Led = PIN_A7;
static const uint8_t H_Sens = PIN_A1;
static const uint8_t H_Sens_ADC_Channel = 1;
static_assert(analogInputToDigitalPin(H_Sens_ADC_Channel) == H_Sens, "Hopper sensor ADC channel mismatch");
static const uint8_t STATUS_PIN = PIN_A0;

static const uint8_t EN_Boost = PIN_A3;
static const uint8_t EN_3V3 = PIN_A2;
static const uint8_t RES_Display = PIN_B0;

static const uint8_t ENC_B = PIN_B1;
static const uint8_t ENC_A = PIN_B2;
static const uint8_t ENC_SW = PIN_A5;
// Debouncing the encoder switch for a certain time, since the Schmitt-trigger on
// `ENC_SW` is not enough to debounce or is not available at all.
static const uint16_t DEBOUNCE_TIME_US = 5000; // us

static const uint8_t SCL = PIN_A4;
static const uint8_t SDA = PIN_A6;

static const bool LED_ON = HIGH;
static const bool LED_OFF = LOW;

static const bool HOPPER_FULL = HIGH;
static const bool HOPPER_EMPTY = LOW;

// Used for SET_ADDRESS, just use 0 to only respond to the
// wildcard type.
const uint8_t INFO_HW_TYPE = 0;

// By default, listen to addresses 8 only
const uint8_t INITIAL_ADDRESS = 0x08;
const uint8_t INITIAL_BITS = 7;

const uint8_t PROTOCOL_VERSION = 0x0000;

#define TWOWIRE_USE_INTERRUPTS
#define USE_I2C

#endif /* CONFIG_H_ */
