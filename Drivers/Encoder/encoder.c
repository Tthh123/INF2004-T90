#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/time.h"

// Define the GPIO pin for the START button and the debounce time
#define WHEEL_ENCODER_1              28
#define WHEEL_ENCODER_2              27
#define DEBOUNCE_TIME_MS             10 // Debounce time in milliseconds
#define TOTAL_NOTCHES_PER_REVOLUTION 20
#define WHEEL_CIRCUMFERENCE_CM       21

u_int8_t notch_count            = 0;
bool     startMeasuring         = false;
uint32_t last_interrupt_time_ms = 0;
uint32_t last_rising            = 0;
uint32_t last_falling           = 0;
uint32_t total_period           = 0;
uint32_t notch_period           = 0;
double   total_dist_travelled   = 0.0;

void
calculate_speed(uint32_t notch_period_ms)
{
    // Convert time from milliseconds to seconds
    double time_s = (double)notch_period_ms / 1000.0;

    // Calculate distance per notch
    double distance_per_notch
        = WHEEL_CIRCUMFERENCE_CM / TOTAL_NOTCHES_PER_REVOLUTION;

    // Calculate speed (cm/s)
    double speed = distance_per_notch / time_s;

    // Print speed
    printf("Speed: %.2f cm/s\n", speed);
}

// Callback function for button press with debounce
void
handle_notch(uint gpio, uint32_t events)
{
    // Debounce algorithm using time (100ms debounce time)
    uint32_t current_timestamp_ms = to_ms_since_boot(get_absolute_time());

    if ((current_timestamp_ms - last_interrupt_time_ms) < DEBOUNCE_TIME_MS)
    {
        // Ignore events within the debounce time
        return;
    }
    last_interrupt_time_ms = current_timestamp_ms;

    if (events == GPIO_IRQ_EDGE_RISE)
    {
        if (startMeasuring == true)
        {
            notch_period = to_ms_since_boot(get_absolute_time()) - last_falling;
            printf("Notch period:%d ", notch_period);
            startMeasuring = false;
        }
        total_period = to_ms_since_boot(get_absolute_time()) - last_rising;
        last_rising  = to_ms_since_boot(get_absolute_time());
        printf("Total period:%d\n", total_period);
        calculate_speed(total_period);
    }
    if (events == GPIO_IRQ_EDGE_FALL)
    {
        notch_count++;
        if (notch_count == 20)
        {
            notch_count = 0;
        }
        startMeasuring = true;
        last_falling   = to_ms_since_boot(get_absolute_time());
        // Calculate the distance per notch
        double distance_per_notch
            = WHEEL_CIRCUMFERENCE_CM / TOTAL_NOTCHES_PER_REVOLUTION;

        // Increment the total distance travelled
        total_dist_travelled += distance_per_notch;

        // Print the distance
        printf("Distance travelled for this notch: %.2f cm\n",
               distance_per_notch);
        printf("Total Distance travelled: %.2f cm\n", total_dist_travelled);
    }
}

int
main()
{
    // Initialize the standard I/O for printf
    stdio_init_all();

    // Initialize the GPIO pin for the START button and configure the pull-up
    // resistor
    gpio_init(WHEEL_ENCODER_1);
    gpio_init(WHEEL_ENCODER_2);
    gpio_set_dir(WHEEL_ENCODER_1, GPIO_IN);
    gpio_set_dir(WHEEL_ENCODER_2, GPIO_IN);

    // Enable interrupts on the START button with the debounce callback
    gpio_set_irq_enabled_with_callback(WHEEL_ENCODER_1,
                                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                       true,
                                       &handle_notch);
    gpio_set_irq_enabled_with_callback(WHEEL_ENCODER_2,
                                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                       true,
                                       &handle_notch);

    while (1)
    {
        // Enter a tight loop, waiting for button events and handling them in
        // the callback
        tight_loop_contents();
    }
    return 0;
}
