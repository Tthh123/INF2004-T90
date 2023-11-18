#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include <stdio.h>
#include "pico/stdlib.h"

const uint8_t LEFT_IR_SENSOR = 26;
const uint8_t RIGHT_IR_SENSOR = 27;
const uint8_t D_LEFT_IR_SENSOR = 20;
const uint8_t D_RIGHT_IR_SENSOR = 21;

const uint8_t L_VCC = 2;     // Left IR sensor's VCC
const uint8_t L_GND = 3;     // Left IR sensor's GND
const uint8_t R_VCC = 4;     // Right IR sensor's VCC
const uint8_t R_GND = 5;     // Right IR sensor's GND

const uint8_t LEFT_ADC_CHANNEL = 0;
const uint8_t RIGHT_ADC_CHANNEL = 1;

const uint16_t BLACK_SURFACE_THRESHOLD = 1000;

void init_ir() {
    adc_init();
    // Initialize IR sensor and power supply pins
    const uint8_t IR_SENSOR_PINS[] = {LEFT_IR_SENSOR, RIGHT_IR_SENSOR};
    const uint8_t IR_SENSOR_VCC[] = {L_VCC, R_VCC};
    const uint8_t IR_SENSOR_GND[] = {L_GND, R_GND};

    for (size_t i = 0; i < sizeof(IR_SENSOR_PINS) / sizeof(IR_SENSOR_PINS[0]); i++) {
        gpio_init(IR_SENSOR_PINS[i]);
        gpio_set_dir(IR_SENSOR_VCC[i], GPIO_OUT);
        gpio_set_dir(IR_SENSOR_GND[i], GPIO_OUT);
        gpio_put(IR_SENSOR_VCC[i], 1);
        gpio_put(IR_SENSOR_GND[i], 0);
    }

    // Configure IR sensor pins
    for (size_t i = 0; i < sizeof(IR_SENSOR_PINS) / sizeof(IR_SENSOR_PINS[0]); i++) {
        gpio_set_dir(IR_SENSOR_PINS[i], GPIO_IN);
        gpio_set_function(IR_SENSOR_PINS[i], GPIO_FUNC_SIO);
        gpio_disable_pulls(IR_SENSOR_PINS[i]);
        gpio_set_input_enabled(IR_SENSOR_PINS[i], false);
    }
}


// void read_lines() {
//     // Initialize ADC
//     adc_init();
    
//     while (true) {
//         // Read analog values
//         adc_select_input(LEFT_ADC_CHANNEL);
//         uint16_t left_adc_result = adc_read();
//         adc_select_input(RIGHT_ADC_CHANNEL);
//         uint16_t right_adc_result = adc_read();

//         // Read digital states
//         bool left_ir_state = gpio_get(D_LEFT_IR_SENSOR);
//         bool right_ir_state = gpio_get(D_RIGHT_IR_SENSOR);
        
//         // Determine surface color based on ADC values
//         const char* left_surface = (left_adc_result > BLACK_SURFACE_THRESHOLD) ? "Black" : "White";
//         const char* right_surface = (right_adc_result > BLACK_SURFACE_THRESHOLD) ? "Black" : "White";

//         const char* format_string_left = "Left ADC Result: %d - State: %d - Surface: %s\n";
//         const char* format_string_right = "Right ADC Result: %d - State: %d - Surface: %s\n";
//         // Print results
//         printf(format_string_left, left_adc_result, left_ir_state, left_surface);
//         printf(format_string_right, right_adc_result, right_ir_state, right_surface);

//         // Sample every 1s
//         sleep_ms(1000);
//     }
// }

bool is_left_surface_black() {

    // Read analog values
    adc_select_input(LEFT_ADC_CHANNEL);
    uint16_t left_adc_result = adc_read();

    // Determine surface color based on ADC values
    bool left_surface_black = (left_adc_result > BLACK_SURFACE_THRESHOLD); // return 1 if black, else return 0 if white

    return left_surface_black;
}

bool is_right_surface_black() {

    // Read analog values
    adc_select_input(RIGHT_ADC_CHANNEL);
    uint16_t right_adc_result = adc_read();

    // Determine surface color based on ADC values
    bool right_surface_black = (right_adc_result > BLACK_SURFACE_THRESHOLD); 

    return right_surface_black; // return 1 if black, else return 0 if white
} 

int main(void) {
    // Initialize stdio
    stdio_init_all();

    // Initialize GPIO
    init_ir();

    // Start reading sensor values
    // read_lines();

    return 0;
}