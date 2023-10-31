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

bool
interrupt_callback(struct repeating_timer *t)
{
    printf("Value: %d\n", adc_read());
    return true;
}

void
turnLeft()
{
    printf("Turn Left");
    gpio_put(2, 0);
    gpio_put(3, 0);
    gpio_put(4, 0);
    gpio_put(5, 1);
}

void
turnRight()
{
    printf("Turn Right");
    gpio_put(2, 0);
    gpio_put(3, 1);
    gpio_put(4, 0);
    gpio_put(5, 0);
}

void
moveForward()
{
    printf("Forward!");
    gpio_put(2, 0);
    gpio_put(3, 1);
    gpio_put(4, 0);
    gpio_put(5, 1);
}

void
moveBackward()
{
    printf("Backwards!");
    gpio_put(2, 1);
    gpio_put(3, 0);
    gpio_put(4, 1);
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
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 12500 / 2);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 12500 / 2);
    // Set initial B output high for three cycles before dropping
    // pwm_set_chan_level(slice_num, PWM_CHAN_B, 3);
    // Set the PWM running
    pwm_set_enabled(slice_num, true);

    while (1)
    {
        sleep_ms(2000);
        moveForward();
        sleep_ms(5000);
        stopMovement();
        sleep_ms(5000);
        moveBackward();
        sleep_ms(5000);
        turnLeft();
        turnRight();
    }
}