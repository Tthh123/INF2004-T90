/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Output PWM signals on pins 0 and 1
// 23 for right
// 45 for left

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include <stdio.h>

#define RIGHT_PWM      0
#define LEFT_PWM       1
#define RIGHT_BACKWARD 2
#define RIGHT_FORWARD  3
#define LEFT_BACKWARD  4
#define LEFT_FORWARD   5
#define TRIG_PIN 6
#define ECHO_PIN 7

volatile uint32_t start_time = 0;
volatile uint32_t end_time = 0;
volatile bool new_measurement_available = false;

const uint32_t wrap_speed = 64515;
const float left_adjustments = 0.965;
const float turn_value = 0.0;

void echo_isr() {
    if (gpio_get(ECHO_PIN)) { // Rising edge detected
        start_time = time_us_32();
    } else { // Falling edge detected
        end_time = time_us_32();
        new_measurement_available = true;
    }
}

void timer_callback() {
    // Set TRIG pin low
    gpio_put(TRIG_PIN, 0);
    
    // Disable the timer interrupt (single shot)
    hw_clear_bits(&timer_hw->intr, 1u << 0);
}

void hcsr04_init() {
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN, 0);

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);

    // Setup interrupt on ECHO pin for both rising and falling edges
    gpio_set_irq_enabled_with_callback(ECHO_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &echo_isr);
}

void hcsr04_trigger_measurement() {
    // Start the timer interrupt for 10 microseconds
    uint alarm_num = 0;  // Using timer 0
    hw_set_bits(&timer_hw->inte, 1u << alarm_num);
    timer_hw->alarm[alarm_num] = timer_hw->timerawl + 10;  // Set for 10 microseconds from now
    
    irq_set_exclusive_handler(TIMER_IRQ_0, timer_callback);
    irq_set_enabled(TIMER_IRQ_0, true);

    // Set TRIG pin high
    gpio_put(TRIG_PIN, 1);
}


float hcsr04_calculate_distance_cm() {
    float time_elapsed = (float)(end_time - start_time);  // Time in microseconds
    return (time_elapsed * 0.0343) / 2;  // Speed of sound is 343 m/s or 0.0343 cm/us
}


void
set_pulse_width(uint slice_num, uint8_t channel, float duty_cycle)
{
    if (channel == PWM_CHAN_A)
    {
        pwm_set_chan_level(slice_num, PWM_CHAN_A, 12500/2);
    }
    else if (channel == PWM_CHAN_B)
    {
        pwm_set_chan_level(slice_num, PWM_CHAN_B, 12500/2);
    }
    else
    {
        // Invalid channel
        printf("Error: Invalid PWM channel!\n");
    }
}

void
turnLeft()
{
    printf("Turn Left");
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 0);
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 1);
}

void
turnRight()
{
    printf("Turn Right");
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 1);
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 0);
}

void
moveForward()
{
    printf("Forward!");
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 1);
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 1);
}

void
moveBackward()
{
    printf("Backwards!");
    gpio_put(RIGHT_BACKWARD, 1);
    gpio_put(RIGHT_FORWARD, 0);
    gpio_put(LEFT_BACKWARD, 1);
    gpio_put(LEFT_FORWARD, 0);
}
void
stopMovement()
{
    printf("Stop!");
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 0);
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 0);
}

int
main()
{
    /// \tag::setup_pwm[]

    // Initialize the standard I/O for printf
    stdio_init_all();
    adc_init();
    gpio_init(RIGHT_BACKWARD);
    gpio_init(RIGHT_FORWARD);
    gpio_init(LEFT_BACKWARD);
    gpio_init(LEFT_FORWARD);

    struct repeating_timer timer;
    // Tell GPIO 0 and 1 they are allocated to the PWM
    gpio_set_function(RIGHT_PWM, GPIO_FUNC_PWM);
    gpio_set_function(LEFT_PWM, GPIO_FUNC_PWM);

    gpio_set_dir(RIGHT_BACKWARD, GPIO_OUT);
    gpio_set_dir(RIGHT_FORWARD, GPIO_OUT);
    gpio_set_dir(LEFT_BACKWARD, GPIO_OUT);
    gpio_set_dir(LEFT_FORWARD, GPIO_OUT);
    uint slice_num = pwm_gpio_to_slice_num(RIGHT_PWM);
    pwm_set_clkdiv(slice_num, 100);
    // Set period of 4 cycles (0 to 3 inclusive)
    pwm_set_wrap(slice_num, 12500);
    // Set channel A output high for one cycle before dropping
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 12500 );
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 12500 );
    // Set initial B output high for three cycles before dropping
    // pwm_set_chan_level(slice_num, PWM_CHAN_B, 3);
    // Set the PWM running
    pwm_set_enabled(slice_num, true);

    while (1)
    {
        if (time_us_32() - last_trigger_time > 500000) {
            hcsr04_trigger_measurement();
            last_trigger_time = time_us_32();
        }

        // Check if a new measurement is available
        if (new_measurement_available) {
            float distance = hcsr04_calculate_distance_cm();
            printf("Distance: %.2f cm\n", distance);
            new_measurement_available = false;
        }
    }
}