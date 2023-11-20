
// /**
//  * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
//  *
//  * SPDX-License-Identifier: BSD-3-Clause
//  */

// #include <stdio.h>
// #include "pico/stdlib.h"

// int main() {
//     stdio_init_all();
//     while (true) {
//         printf("Hello, world!\n");
//         sleep_ms(1000);
//     }
// }

//Get readings from ultrasonic sensor

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/timer.h"

int timeout = 26100;

void setupUltrasonicPins(uint trigPin, uint echoPin)
{
    gpio_init(trigPin);
    gpio_init(echoPin);
    gpio_set_dir(trigPin, GPIO_OUT);
    gpio_set_dir(echoPin, GPIO_IN);
}

uint64_t getPulse(uint trigPin, uint echoPin)
{
    gpio_put(trigPin, 1);
    sleep_us(10);
    gpio_put(trigPin, 0);

    uint64_t width = 0;

    while (gpio_get(echoPin) == 0) tight_loop_contents();
    absolute_time_t startTime = get_absolute_time();
    while (gpio_get(echoPin) == 1) 
    {
        width++;
        sleep_us(1);
        if (width > timeout) return 0;
    }
    absolute_time_t endTime = get_absolute_time();
    
    return absolute_time_diff_us(startTime, endTime);
}

uint64_t getCm(uint trigPin, uint echoPin)
{
    uint64_t pulseLength = getPulse(trigPin, echoPin);
    return pulseLength / 29 / 2;
}

uint64_t getInch(uint trigPin, uint echoPin)
{
    uint64_t pulseLength = getPulse(trigPin, echoPin);
    return (long)pulseLength / 74.f / 2.f;
}

int main() {
    stdio_init_all();
    
    // Set up ultrasonic sensor pins
    uint trigPin = 0;  // Replace with the appropriate GPIO pin number
    uint echoPin = 1;  // Replace with the appropriate GPIO pin number
    setupUltrasonicPins(trigPin, echoPin);
    
    while (true) {
        // Get distance in centimeters
        uint64_t distance_cm = getCm(trigPin, echoPin);
        
        // Print distance to the console
        if (distance_cm > 0) {
            printf("Distance: %llu cm\n", distance_cm);
        } else {
            printf("No object detected\n");
        }
        
        sleep_ms(1000);
    }
}
