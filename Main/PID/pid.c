#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include <stdio.h>

#define RIGHT_PWM                    0
#define LEFT_PWM                     1
#define RIGHT_BACKWARD               2
#define RIGHT_FORWARD                3
#define LEFT_BACKWARD                4
#define LEFT_FORWARD                 5
#define TRIG_PIN                     6
#define ECHO_PIN                     7
#define WHEEL_ENCODER_1              28
#define WHEEL_ENCODER_2              27
#define DEBOUNCE_TIME_MS             10 // Debounce time in milliseconds
#define TOTAL_NOTCHES_PER_REVOLUTION 20
#define WHEEL_CIRCUMFERENCE_CM       21

u_int8_t          notch_count               = 0;
volatile uint32_t start_time                = 0;
volatile uint32_t end_time                  = 0;
volatile bool     new_measurement_available = false;

uint32_t last_interrupt_time_ms = 0;

bool     startMeasuringRight = false;
uint32_t lastRisingRight     = 0;
uint32_t lastFallingRight    = 0;
uint32_t totalPeriodRight    = 0;
uint32_t notchPeriodRight    = 0;
double   totalDistRight      = 0.0;
float    currSpeedRight      = 0.0;

bool     startMeasuringLeft = false;
uint32_t lastRisingLeft     = 0;
uint32_t lastFallingLeft    = 0;
uint32_t totalPeriodLeft    = 0;
uint32_t notchPeriodLeft    = 0;
double   totalDistLeft      = 0.0;
float    currSpeedLeft      = 0.0;

float Kp_right = 1.0, Ki_right = 0.01, Kd_right = 0.05;
float Kp_left = 0.1, Ki_left = 0.01, Kd_left = 0.05;
float error_right = 0, integral_right = 0, derivative_right = 0,
      last_error_right = 0;
float error_left = 0, integral_left = 0, derivative_left = 0,
      last_error_left    = 0;
float target_speed_right = 20.0; // Set your target speeds here
float target_speed_left  = 20.0;

bool forwardMovement = 1;

float
calculate_speed(uint32_t notch_period_ms)
{
    // Convert time from milliseconds to seconds
    double time_s = (double)notch_period_ms / 1000.0;

    // Calculate distance per notch
    double distance_per_notch
        = WHEEL_CIRCUMFERENCE_CM / TOTAL_NOTCHES_PER_REVOLUTION;

    // Calculate speed (cm/s)
    double speed = distance_per_notch / time_s;
    return speed;
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

    if (events == GPIO_IRQ_EDGE_RISE && gpio == WHEEL_ENCODER_1)
    {
        if (startMeasuringRight == true)
        {
            notchPeriodRight
                = to_ms_since_boot(get_absolute_time()) - lastFallingRight;
            startMeasuringRight = false;
        }
        totalPeriodRight
            = to_ms_since_boot(get_absolute_time()) - lastRisingRight;
        lastRisingRight = to_ms_since_boot(get_absolute_time());
        currSpeedRight  = calculate_speed(totalPeriodRight);
        printf("Right speed: %f\n", currSpeedRight);
    }
    if (events == GPIO_IRQ_EDGE_FALL && gpio == WHEEL_ENCODER_1)
    {
        notch_count++;
        if (notch_count == 20)
        {
            notch_count = 0;
        }
        startMeasuringRight = true;
        lastFallingRight    = to_ms_since_boot(get_absolute_time());
        // Calculate the distance per notch
        double distance_per_notch
            = WHEEL_CIRCUMFERENCE_CM / TOTAL_NOTCHES_PER_REVOLUTION;

        // Increment the total distance travelled
        totalDistRight += distance_per_notch;
    }
    if (events == GPIO_IRQ_EDGE_RISE && gpio == WHEEL_ENCODER_2)
    {
        if (startMeasuringLeft == true)
        {
            notchPeriodLeft
                = to_ms_since_boot(get_absolute_time()) - lastFallingLeft;
            startMeasuringLeft = false;
        }
        totalPeriodLeft
            = to_ms_since_boot(get_absolute_time()) - lastRisingLeft;
        lastRisingLeft = to_ms_since_boot(get_absolute_time());
        currSpeedLeft  = calculate_speed(totalPeriodLeft);
        printf("Left speed: %f\n", currSpeedLeft);
    }
    if (events == GPIO_IRQ_EDGE_FALL && gpio == WHEEL_ENCODER_2)
    {
        notch_count++;
        if (notch_count == 20)
        {
            notch_count = 0;
        }
        startMeasuringLeft = true;
        lastFallingLeft    = to_ms_since_boot(get_absolute_time());
        // Calculate the distance per notch
        double distance_per_notch
            = WHEEL_CIRCUMFERENCE_CM / TOTAL_NOTCHES_PER_REVOLUTION;

        // Increment the total distance travelled
        totalDistLeft += distance_per_notch;
    }
}


float
pid_controller(float  current_speed,
               float  target_speed,
               float *error,
               float *integral,
               float *derivative,
               float *last_error,
               float  Kp,
               float  Ki,
               float  Kd,
               float  minOutput,
               float  maxOutput)
{
    float previous_error = *last_error;
    *error               = target_speed - current_speed;
    // printf("Error: %f\n", *error);
    //  Integral windup guard

    // Update the integral term with the current error
    *integral += *error;

    *derivative = *error - previous_error;
    *last_error = *error;

    // PID calculation
    float output = (Kp * (*error)) + (Ki * (*integral)) - (Kd * (*derivative));

    // Clamping the output to min and max
    if (output > maxOutput)
    {
        output = maxOutput;
    }
    else if (output < minOutput)
    {
        output = minOutput;
    }

    return output;
}

void
set_pulse_width(uint slice_num, uint8_t channel, float duty_cycle)
{
    if (channel == PWM_CHAN_A)
    {
        pwm_set_chan_level(slice_num, PWM_CHAN_A, 12500 * (duty_cycle / 100));
    }
    else if (channel == PWM_CHAN_B)
    {
        pwm_set_chan_level(slice_num, PWM_CHAN_B, 12500 * (duty_cycle / 100));
    }
    else
    {
        // Invalid channel
        printf("Error: Invalid PWM channel!\n");
    }
}

void
update_motors()
{
    float pwm_right = pid_controller(currSpeedRight,
                                     target_speed_right,
                                     &error_right,
                                     &integral_right,
                                     &derivative_right,
                                     &last_error_right,
                                     Kp_right,
                                     Ki_right,
                                     Kd_right,
                                     40,
                                     70);
    float pwm_left  = pid_controller(currSpeedLeft,
                                    target_speed_left,
                                    &error_left,
                                    &integral_left,
                                    &derivative_left,
                                    &last_error_left,
                                    Kp_left,
                                    Ki_left,
                                    Kd_left,
                                    15,
                                    30);

    // printf("Right: %f\n", pwm_right);
    // printf("Left: %f\n", pwm_left);
    //   Adjust PWM duty cycle
    // set_pulse_width(0, PWM_CHAN_A, pwm_right);
    set_pulse_width(0, PWM_CHAN_B, pwm_left);
}

void
turnLeft()
{
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 0);
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 1);
}

void
turnRight()
{
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 1);
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 0);
}

void
moveForward()
{

    gpio_put(RIGHT_BACKWARD, 1);
    gpio_put(RIGHT_FORWARD, 0);
    gpio_put(LEFT_BACKWARD, 1);
    gpio_put(LEFT_FORWARD, 0);
}

void
moveBackward()
{
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 1);
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 1);
}
void
stopMovement()
{
    gpio_put(2, 0);
    gpio_put(3, 0);
    gpio_put(4, 0);
    gpio_put(5, 0);
}

void
init_pins()
{
    adc_init();
    gpio_init(RIGHT_PWM);
    gpio_init(LEFT_PWM);
    gpio_init(RIGHT_BACKWARD);
    gpio_init(RIGHT_FORWARD);
    gpio_init(LEFT_BACKWARD);
    gpio_init(LEFT_FORWARD);
    gpio_init(WHEEL_ENCODER_1);
    gpio_init(WHEEL_ENCODER_2);
    gpio_set_dir(WHEEL_ENCODER_1, GPIO_IN);
    gpio_set_dir(WHEEL_ENCODER_2, GPIO_IN);
    gpio_set_function(RIGHT_PWM, GPIO_FUNC_PWM);
    gpio_set_function(LEFT_PWM, GPIO_FUNC_PWM);
    gpio_set_dir(RIGHT_BACKWARD, GPIO_OUT);
    gpio_set_dir(RIGHT_FORWARD, GPIO_OUT);
    gpio_set_dir(LEFT_BACKWARD, GPIO_OUT);
    gpio_set_dir(LEFT_FORWARD, GPIO_OUT);
    pwm_set_clkdiv(0, 100);
    pwm_set_wrap(0, 16075);
    pwm_set_chan_level(0, PWM_CHAN_A, 6000);
    pwm_set_chan_level(0, PWM_CHAN_B, 6500);
    pwm_set_enabled(0, true);
}

int
main()
{
    stdio_init_all();
    uint32_t last_trigger_time = 0;
    sleep_ms(3000);
    printf("Start");
    init_pins();

    // gpio_set_irq_enabled_with_callback(WHEEL_ENCODER_1,
    //                                    GPIO_IRQ_EDGE_RISE |
    //                                    GPIO_IRQ_EDGE_FALL, true,
    //                                    &handle_notch);
    // gpio_set_irq_enabled_with_callback(WHEEL_ENCODER_2,
    //                                    GPIO_IRQ_EDGE_RISE |
    //                                    GPIO_IRQ_EDGE_FALL, true,
    //                                    &handle_notch);

    while (1)
    {
        moveForward();
        update_motors();
    }
}