#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include <stdio.h>

#define TRIG_PIN 6
#define ECHO_PIN 7

volatile uint32_t start_time = 0;
volatile uint32_t end_time = 0;
volatile bool new_measurement_available = false;

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

int main() {
    stdio_init_all();
    hcsr04_init();

    uint32_t last_trigger_time = 0;

    while (1) {
        // Trigger a new measurement every 500 ms
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

    return 0;
}
