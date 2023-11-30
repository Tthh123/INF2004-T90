#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include <stdio.h>

// Callback function for the timer interrupt, reads ADC value and prints it
bool interrupt_callback(struct repeating_timer *t) {
    printf("Value: %d\n", adc_read());
    return true;
}

// Function to turn the vehicle left
void turnLeft() {
    printf("Turn Left");
    // Set GPIO pins to appropriate values for turning left
    gpio_put(2, 0);
    gpio_put(3, 0);
    gpio_put(4, 0);
    gpio_put(5, 1);
}

// Function to turn the vehicle right
void turnRight() {
    printf("Turn Right");
    // Set GPIO pins to appropriate values for turning right
    gpio_put(2, 0);
    gpio_put(3, 1);
    gpio_put(4, 0);
    gpio_put(5, 0);
}

// Function to move the vehicle forward
void moveForward() {
    printf("Forward!");
    // Set GPIO pins to appropriate values for moving forward
    gpio_put(2, 0);
    gpio_put(3, 1);
    gpio_put(4, 0);
    gpio_put(5, 1);
}

// Function to move the vehicle backward
void moveBackward() {
    printf("Backwards!");
    // Set GPIO pins to appropriate values for moving backward
    gpio_put(2, 1);
    gpio_put(3, 0);
    gpio_put(4, 1);
    gpio_put(5, 0);
}

// Function to stop the vehicle's movement
void stopMovement() {
    printf("Stop!");
    // Set GPIO pins to zero to stop movement
    gpio_put(2, 0);
    gpio_put(3, 0);
    gpio_put(4, 0);
    gpio_put(5, 0);
}


int main() {
    stdio_init_all();
    adc_init();
    gpio_init(2);
    gpio_init(3);
    gpio_init(4);
    gpio_init(5);

    // Set up a repeating timer
    struct repeating_timer timer;

    // Configure GPIO pins 0 and 1 for PWM function
    gpio_set_function(0, GPIO_FUNC_PWM);
    gpio_set_function(1, GPIO_FUNC_PWM);

    // Set ADC input pin
    gpio_set_function(26, GPIO_FUNC_SIO);

    // Set direction of GPIO pins for controlling movement
    gpio_set_dir(2, GPIO_OUT);
    gpio_set_dir(3, GPIO_OUT);
    gpio_set_dir(4, GPIO_OUT);
    gpio_set_dir(5, GPIO_OUT);

    // Configure PWM settings
    uint slice_num = pwm_gpio_to_slice_num(0);
    pwm_set_clkdiv(slice_num, 100); // Set clock divider
    pwm_set_wrap(slice_num, 12500); // Set wrap value for the PWM counter
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 12500 / 2); // Set PWM channel A level
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 12500 / 2); // Set PWM channel B level
    pwm_set_enabled(slice_num, true); // Enable PWM

    while (1) {
        
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
