#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/time.h"

// Define constants for wheel encoder pins, debounce time, total notches per wheel revolution, and wheel circumference
#define WHEEL_ENCODER_1              28
#define WHEEL_ENCODER_2              27
#define DEBOUNCE_TIME_MS             10 // Debounce time in milliseconds for handling noise in signal
#define TOTAL_NOTCHES_PER_REVOLUTION 20 // Total number of notches in one wheel revolution
#define WHEEL_CIRCUMFERENCE_CM       21 // Wheel circumference in centimeters

// Global variables for tracking wheel encoder events and measurements
u_int8_t notch_count            = 0;     // Counts the number of notches encountered
bool     startMeasuring         = false; // Flag to start measuring speed
uint32_t last_interrupt_time_ms = 0;     // Timestamp of the last interrupt event for debounce
uint32_t last_rising            = 0;     // Timestamp of the last rising edge event
uint32_t last_falling           = 0;     // Timestamp of the last falling edge event
uint32_t total_period           = 0;     // Time period between consecutive rising edge events
uint32_t notch_period           = 0;     // Time period between a falling and the subsequent rising edge event
double   total_dist_travelled   = 0.0;   // Total distance travelled by the wheel

// Function to calculate and print the speed of the wheel based on the notch period
void calculate_speed(uint32_t notch_period_ms)
{
    // Convert time from milliseconds to seconds
    double time_s = (double)notch_period_ms / 1000.0;

    // Calculate distance travelled for one notch
    double distance_per_notch = WHEEL_CIRCUMFERENCE_CM / TOTAL_NOTCHES_PER_REVOLUTION;

    // Calculate speed in cm/s and print it
    double speed = distance_per_notch / time_s;
    printf("Speed: %.2f cm/s\n", speed);
}

// Interrupt callback function to handle wheel encoder events, applies debounce logic
void handle_notch(uint gpio, uint32_t events)
{
    uint32_t current_timestamp_ms = to_ms_since_boot(get_absolute_time());

    // Ignore events occurring within the debounce period
    if ((current_timestamp_ms - last_interrupt_time_ms) < DEBOUNCE_TIME_MS)
    {
        return;
    }
    last_interrupt_time_ms = current_timestamp_ms; 

    // Handle rising edge events for speed calculation
    if (events == GPIO_IRQ_EDGE_RISE)
    {
        // Calculate the period between the falling and rising edges if measuring is active
        if (startMeasuring == true)
        {
            notch_period = current_timestamp_ms - last_falling;
            printf("Notch period:%d ", notch_period);
            startMeasuring = false; /
        }
        // Calculate the total period between consecutive rising edges
        total_period = current_timestamp_ms - last_rising;
        last_rising = current_timestamp_ms;
        printf("Total period:%d\n", total_period);
        calculate_speed(total_period); 
    }

    // Handle falling edge events for distance calculation
    if (events == GPIO_IRQ_EDGE_FALL)
    {
        notch_count++;
        // Reset notch count after a complete revolution
        if (notch_count == TOTAL_NOTCHES_PER_REVOLUTION)
        {
            notch_count = 0;
        }
        startMeasuring = true; // Set flag to start measuring speed
        last_falling = current_timestamp_ms; // Update last falling edge timestamp

        // Calculate distance travelled per notch
        double distance_per_notch = WHEEL_CIRCUMFERENCE_CM / TOTAL_NOTCHES_PER_REVOLUTION;
        total_dist_travelled += distance_per_notch; // Increment total distance travelled

        printf("Distance travelled for this notch: %.2f cm\n", distance_per_notch);
        printf("Total Distance travelled: %.2f cm\n", total_dist_travelled);
    }
}

int main()
{

    stdio_init_all();

    // Initialize GPIO pins for wheel encoders and configure them as input
    gpio_init(WHEEL_ENCODER_1);
    gpio_init(WHEEL_ENCODER_2);
    gpio_set_dir(WHEEL_ENCODER_1, GPIO_IN);
    gpio_set_dir(WHEEL_ENCODER_2, GPIO_IN);

    // Enable GPIO interrupts for both wheel encoders, using the handle_notch callback
    gpio_set_irq_enabled_with_callback(WHEEL_ENCODER_1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &handle_notch);
    gpio_set_irq_enabled_with_callback(WHEEL_ENCODER_2, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &handle_notch);

    while (1)
    {
        tight_loop_contents();
    }
    return 0;
}
