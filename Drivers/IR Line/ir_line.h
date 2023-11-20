// sensor_controller.h

#ifndef IR_LINE_H
#define IR_LINE_H

#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include <stdio.h>
#include "pico/stdlib.h"

extern const uint8_t LEFT_IR_SENSOR;
extern const uint8_t RIGHT_IR_SENSOR;
extern const uint8_t D_LEFT_IR_SENSOR;
extern const uint8_t D_RIGHT_IR_SENSOR;
extern const uint8_t L_VCC;
extern const uint8_t L_GND;
extern const uint8_t R_VCC;
extern const uint8_t R_GND;
extern const uint8_t LEFT_ADC_CHANNEL;
extern const uint8_t RIGHT_ADC_CHANNEL;
extern const uint16_t BLACK_SURFACE_THRESHOLD;

void init_ir();
bool is_left_surface_black();
bool is_right_surface_black();

#endif // IR_LINE_h
