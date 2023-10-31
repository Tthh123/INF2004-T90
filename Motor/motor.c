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
    gpio_put(5, 0);
}
void
stopMovement()
{
    printf("Stop!");
    gpio_put(2, 0);
    gpio_put(3, 0);
    gpio_put(4, 0);
    gpio_put(5, 0);
}

int
main()
{
    /// \tag::setup_pwm[]

    // Initialize the standard I/O for printf
    stdio_init_all();
    adc_init();
    gpio_init(2);
    gpio_init(3);
    gpio_init(4);
    gpio_init(5);

    struct repeating_timer timer;
    // Tell GPIO 0 and 1 they are allocated to the PWM
    gpio_set_function(0, GPIO_FUNC_PWM);
    gpio_set_function(1, GPIO_FUNC_PWM);
    // Set ADC input pin
    gpio_set_function(26, GPIO_FUNC_SIO);

    gpio_set_dir(2, GPIO_OUT);
    gpio_set_dir(3, GPIO_OUT);
    gpio_set_dir(4, GPIO_OUT);
    gpio_set_dir(5, GPIO_OUT);
    uint slice_num = pwm_gpio_to_slice_num(0);
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
        sleep_ms(5000);
        moveForward();
        sleep_ms(5000);
        set_pulse_width(slice_num, PWM_CHAN_A,50);
        set_pulse_width(slice_num, PWM_CHAN_B,50);
    }
}
// sleep_ms(5000);
//         moveForward();
//         sleep_ms(5000);
//         stopMovement();
//         sleep_ms(5000);
//         moveBackward();
//         sleep_ms(5000);
//         turnLeft();
//         sleep_ms(5000);
//         turnRight();
//         set_pulse_width(slice_num, PWM_CHAN_A,50);
//         set_pulse_width(slice_num, PWM_CHAN_B,50);