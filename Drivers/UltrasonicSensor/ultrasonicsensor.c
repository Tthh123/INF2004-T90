#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include <stdio.h>

// Define GPIO pins for the HC-SR04 ultrasonic sensor
#define TRIG_PIN 6  // Trigger pin
#define ECHO_PIN 7  // Echo pin

// Volatile variables to store timestamps and flag for new measurement
volatile uint32_t start_time = 0;
volatile uint32_t end_time = 0;
volatile bool new_measurement_available = false;

// Interrupt Service Routine for the Echo pin
void echo_isr() {
    if (gpio_get(ECHO_PIN)) { // Rising edge detected
        start_time = time_us_32(); // Record time when echo goes high
    } else { // Falling edge detected
        end_time = time_us_32(); // Record time when echo goes low
        new_measurement_available = true; // Set flag for new measurement
    }
}

// Timer callback function to set trigger pin low
void timer_callback() {
    gpio_put(TRIG_PIN, 0); // Set TRIG pin low
    hw_clear_bits(&timer_hw->intr, 1u << 0); // Disable the timer interrupt
}

// Initialize the HC-SR04 sensor
void hcsr04_init() {
    gpio_init(TRIG_PIN); // Initialize TRIG pin
    gpio_set_dir(TRIG_PIN, GPIO_OUT); // Set TRIG pin as output
    gpio_put(TRIG_PIN, 0); // Set TRIG pin low

    gpio_init(ECHO_PIN); // Initialize ECHO pin
    gpio_set_dir(ECHO_PIN, GPIO_IN); // Set ECHO pin as input

    // Set up interrupt on ECHO pin for both rising and falling edges
    gpio_set_irq_enabled_with_callback(ECHO_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &echo_isr);
}

// Function to trigger a new measurement with the HC-SR04 sensor
void hcsr04_trigger_measurement() {
    hw_set_bits(&timer_hw->inte, 1u << 0); // Enable timer interrupt
    timer_hw->alarm[0] = timer_hw->timerawl + 10; // Set timer for 10 microseconds
    
    irq_set_exclusive_handler(TIMER_IRQ_0, timer_callback); // Set timer interrupt handler
    irq_set_enabled(TIMER_IRQ_0, true); // Enable timer interrupt

    gpio_put(TRIG_PIN, 1); // Set TRIG pin high
}

// Calculate the distance in centimeters based on time elapsed
float hcsr04_calculate_distance_cm() {
    float time_elapsed = (float)(end_time - start_time); // Calculate time difference in microseconds
    return (time_elapsed * 0.0343) / 2; // Convert time to distance (speed of sound is 343 m/s or 0.0343 cm/us)
}

int main() {
    stdio_init_all(); 
    hcsr04_init();

    uint32_t last_trigger_time = 0; // Variable to track last trigger time

    while (1) {
        // Trigger a new measurement every 500 ms
        if (time_us_32() - last_trigger_time > 500000) {
            hcsr04_trigger_measurement();
            last_trigger_time = time_us_32();
        }

        // Check if a new measurement is available
        if (new_measurement_available) {
            float distance = hcsr04_calculate_distance_cm();
            printf("Distance: %.2f cm\n", distance); // Print the measured distance
            new_measurement_available = false; // Reset the measurement flag
        }
    }

    return 0;
}
