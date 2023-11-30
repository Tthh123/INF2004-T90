#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include <stdio.h>

// Define pin constants for motor control
// Output PWM signals on pins 0 and 1 for motor control
#define RIGHT_PWM      0
#define LEFT_PWM       1
#define RIGHT_BACKWARD 2
#define RIGHT_FORWARD  3
#define LEFT_BACKWARD  4
#define LEFT_FORWARD   5

// Function to set the pulse width for a PWM channel
void set_pulse_width(uint slice_num, uint8_t channel, float duty_cycle) {
    if (channel == PWM_CHAN_A) {
        // Set PWM level for channel A
        pwm_set_chan_level(slice_num, PWM_CHAN_A, 12500/2); // Example: set to 50% duty cycle
    } else if (channel == PWM_CHAN_B) {
        // Set PWM level for channel B
        pwm_set_chan_level(slice_num, PWM_CHAN_B, 12500/2); // Example: set to 50% duty cycle
    } else {
        printf("Error: Invalid PWM channel!\n"); // Error message for invalid channel
    }
}

// Function to turn the vehicle left
void turnLeft() {
    printf("Turn Left");
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 0);
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 1); // Activate left forward only
}

// Function to turn the vehicle right
void turnRight() {
    printf("Turn Right");
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 1); // Activate right forward only
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 0);
}

// Function to move the vehicle forward
void moveForward() {
    printf("Forward!");
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 1); // Activate both forwards
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 1);
}

// Function to move the vehicle backward
void moveBackward() {
    printf("Backwards!");
    gpio_put(RIGHT_BACKWARD, 1); // Activate both backwards
    gpio_put(RIGHT_FORWARD, 0);
    gpio_put(LEFT_BACKWARD, 1);
    gpio_put(5, 0);
}

// Function to stop all motor movement
void stopMovement() {
    printf("Stop!");
    gpio_put(2, 0);
    gpio_put(3, 0);
    gpio_put(4, 0);
    gpio_put(5, 0); // Deactivate all motor control pins
}

int main() {
    
    stdio_init_all();
    adc_init();

    
    gpio_init(2);
    gpio_init(3);
    gpio_init(4);
    gpio_init(5);

    // Tell GPIO 0 and 1 they are allocated to the PWM
    gpio_set_function(0, GPIO_FUNC_PWM);
    gpio_set_function(1, GPIO_FUNC_PWM);
    // Set ADC input pin
    gpio_set_function(26, GPIO_FUNC_SIO);

    // Set GPIO pins as output for motor control
    gpio_set_dir(2, GPIO_OUT);
    gpio_set_dir(3, GPIO_OUT);
    gpio_set_dir(4, GPIO_OUT);
    gpio_set_dir(5, GPIO_OUT);

    // Configure PWM settings
    uint slice_num = pwm_gpio_to_slice_num(0);
    pwm_set_clkdiv(slice_num, 100); // Set PWM clock divider
    pwm_set_wrap(slice_num, 12500); // Set PWM wrap value
    // Set PWM channel A and B to 50% duty cycle initially
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 12500 / 2);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 12500 / 2);
    pwm_set_enabled(slice_num, true); // Enable PWM

    while (1) {
        sleep_ms(5000); // Delay for demonstration
        moveForward(); // Move vehicle forward
        sleep_ms(5000); // Delay for demonstration
        set_pulse_width(slice_num, PWM_CHAN_A, 50); // Adjust PWM for channel A
        set_pulse_width(slice_num, PWM_CHAN_B, 50); // Adjust PWM for channel B
    }
}
