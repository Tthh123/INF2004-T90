// Include necessary headers for Raspberry Pi Pico functionality
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include <stdio.h>

// Define GPIO pins for motor control and ADC inputs
#define IN1_PIN 6
#define IN2_PIN 7
#define IN3_PIN 4
#define IN4_PIN 3
#define EN_A_PIN 8
#define EN_B_PIN 2
#define ADC_X_AXIS 27
#define ADC_Y_AXIS 26

// Define IR sensor threshold value
#define IR_THRESHOLD 1000

// Define PWM wrap speed and adjustments for left motor speed
const uint32_t wrap_speed = 64515;
const float left_adjustments = 0.965;

// Function to move both motors forward
void move_forward()
{
    // Set motor control pins to drive motors forward
    gpio_put(IN1_PIN, true);
    gpio_put(IN2_PIN, false);
    gpio_put(IN3_PIN, false);
    gpio_put(IN4_PIN, true);
}

// Function to move both motors backward
void move_backward()
{
    // Set motor control pins to drive motors backward
    gpio_put(IN1_PIN, false);
    gpio_put(IN2_PIN, true);
    gpio_put(IN3_PIN, true);
    gpio_put(IN4_PIN, false);
}

// Function to stop all motors
void stop()
{
    // Reset all motor control pins to stop motors
    gpio_put(IN1_PIN, false);
    gpio_put(IN2_PIN, false);
    gpio_put(IN3_PIN, false);
    gpio_put(IN4_PIN, false);
}

// Function to move forward with speed adjustments
void move_forward_both()
{
    // Set PWM duty cycle for both motors with adjustments for the left motor
    set_duty_cycle(pwm_gpio_to_slice_num(EN_A_PIN), wrap_speed * left_adjustments); // left
    set_duty_cycle(pwm_gpio_to_slice_num(EN_B_PIN), wrap_speed);
    move_forward();
}

// Function to move backward with speed adjustments
void move_backwards_both()
{
    // Set PWM duty cycle for both motors with adjustments for the left motor
    set_duty_cycle(pwm_gpio_to_slice_num(EN_A_PIN), wrap_speed * left_adjustments); // left
    set_duty_cycle(pwm_gpio_to_slice_num(EN_B_PIN), wrap_speed);
    move_backward();
}

// Function to turn left
void move_turn_left()
{
    // Set PWM duty cycle with turn adjustments and move forward
    set_duty_cycle(pwm_gpio_to_slice_num(EN_A_PIN), (wrap_speed * left_adjustments) * turn_value); // left
    set_duty_cycle(pwm_gpio_to_slice_num(EN_B_PIN), wrap_speed);
    move_forward();
}

// Function to turn right
void move_turn_right()
{
    // Set PWM duty cycle with turn adjustments and move forward
    set_duty_cycle(pwm_gpio_to_slice_num(EN_A_PIN), wrap_speed * left_adjustments); // left
    set_duty_cycle(pwm_gpio_to_slice_num(EN_B_PIN), wrap_speed * turn_value);
    move_forward();
}

// Function to stop movement
void move_stop()
{
    // Set PWM duty cycle with stop adjustments and move forward
    set_duty_cycle(pwm_gpio_to_slice_num(EN_A_PIN), wrap_speed * left_adjustments * turn_value); // left
    set_duty_cycle(pwm_gpio_to_slice_num(EN_B_PIN), wrap_speed * turn_value);
    move_forward();
}

// Function to set PWM duty cycle
void set_duty_cycle(uint slice_num, uint16_t duty_cycle)
{
    // Set PWM channel level for specified slice number
    pwm_set_chan_level(slice_num, PWM_CHAN_A, duty_cycle);
}

// Initialization function for motor control
void init_motor_control()
{
    // Initialize GPIO pins and set direction for motor control
    gpio_init(IN1_PIN);
    gpio_set_dir(IN1_PIN, GPIO_OUT);
    gpio_init(IN2_PIN);
    gpio_set_dir(IN2_PIN, GPIO_OUT);
    gpio_init(IN3_PIN);
    gpio_set_dir(IN3_PIN, GPIO_OUT);
    gpio_init(IN4_PIN);
    gpio_set_dir(IN4_PIN, GPIO_OUT);

    // Initialize PWM for motor speed control and set frequency
    gpio_set_function(EN_A_PIN, GPIO_FUNC_PWM);
    gpio_set_function(EN_B_PIN, GPIO_FUNC_PWM);
    uint slice_num_a = pwm_gpio_to_slice_num(EN_A_PIN);
    uint slice_num_b = pwm_gpio_to_slice_num(EN_B_PIN);
    pwm_set_clkdiv_int_frac(slice_num_a, 1, 15);
    pwm_set_clkdiv_int_frac(slice_num_b, 1, 15);
    pwm_set_wrap(slice_num_a, 64515);
    pwm_set_wrap(slice_num_b, 64515);
    pwm_set_enabled(slice_num_a, true);
    pwm_set_enabled(slice_num_b, true);

    // Initialize ADC for sensor readings
    adc_init();
    adc_gpio_init(ADC_X_AXIS);
    adc_gpio_init(ADC_Y_AXIS);
}

// Main function
int main()
{
    // Initialize standard input and output
    stdio_init_all();

    // Initialize motor control
    init_motor_control();

    // Main loop
    while (1)
    {
        // Default movement - move forward
        move_forward_both();

        // Read ADC values from sensors
        adc_select_input(0); // Select ADC channel for X-axis sensor
        uint16_t sensor_value_26 = adc_read();
        adc_select_input(1); // Select ADC channel for Y-axis sensor
        uint16_t sensor_value_27 = adc_read();

        // Decision making based on sensor readings
        if (sensor_value_26 > IR_THRESHOLD)
        {
            // If X-axis sensor is above threshold, turn right then left
            move_turn_right();
            sleep_ms(150);
            move_turn_left();
            sleep_ms(85);
        }
        else if (sensor_value_27 > IR_THRESHOLD)
        {
            // If Y-axis sensor is above threshold, turn left then right
            move_turn_left();
            sleep_ms(150);
            move_turn_right();
            sleep_ms(85);
        }
    }

    return 0;
}