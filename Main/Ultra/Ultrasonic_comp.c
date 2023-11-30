#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include <stdio.h>


#define IN1_PIN 6
#define IN2_PIN 7
#define IN3_PIN 4
#define IN4_PIN 3
#define EN_A_PIN 8
#define EN_B_PIN 2
#define TRIG_PIN 0
#define ECHO_PIN 1

volatile uint32_t start_time = 0;
volatile uint32_t end_time = 0;
volatile bool new_measurement_available = false;
const uint32_t wrap_speed = 64515;
const float left_adjustments = 0.965;

void move_forward()
{
    gpio_put(IN1_PIN, true);
    gpio_put(IN2_PIN, false);
    gpio_put(IN3_PIN, false);
    gpio_put(IN4_PIN, true);
}

void move_backward()
{
    gpio_put(IN1_PIN, false);
    gpio_put(IN2_PIN, true);
    gpio_put(IN3_PIN, true);
    gpio_put(IN4_PIN, false);
}

void stop()
{
    gpio_put(IN1_PIN, false);
    gpio_put(IN2_PIN, false);
    gpio_put(IN3_PIN, false);
    gpio_put(IN4_PIN, false);
}


void move_forward_both()
{
    // For the first 5 seconds, both motors at normal speed
    // set_duty_cycle(pwm_gpio_to_slice_num(EN_A_PIN), normal_speed);
    // set_duty_cycle(pwm_gpio_to_slice_num(EN_B_PIN), normal_speed);
    set_duty_cycle(pwm_gpio_to_slice_num(EN_A_PIN), wrap_speed * left_adjustments); // left
    set_duty_cycle(pwm_gpio_to_slice_num(EN_B_PIN), wrap_speed);
    move_forward();
    // printf("Forwaard\n");
}

void move_backwards_both()
{
    // For the first 5 seconds, both motors at normal speed
    // set_duty_cycle(pwm_gpio_to_slice_num(EN_A_PIN), normal_speed);
    // set_duty_cycle(pwm_gpio_to_slice_num(EN_B_PIN), normal_speed);
    set_duty_cycle(pwm_gpio_to_slice_num(EN_A_PIN), wrap_speed * left_adjustments); // left
    set_duty_cycle(pwm_gpio_to_slice_num(EN_B_PIN), wrap_speed);
    move_backward();
    // printf("Forwaard\n");
}

void move_turn_left()
{
    set_duty_cycle(pwm_gpio_to_slice_num(EN_A_PIN), (wrap_speed * left_adjustments) * turn_value); // left
    set_duty_cycle(pwm_gpio_to_slice_num(EN_B_PIN), wrap_speed);
    move_forward();
    // printf("Left\n");
}

void move_turn_right()
{
    set_duty_cycle(pwm_gpio_to_slice_num(EN_A_PIN), wrap_speed * left_adjustments); // left
    set_duty_cycle(pwm_gpio_to_slice_num(EN_B_PIN), wrap_speed * turn_value);
    move_forward();
    // printf("right\n");
}

void move_stop()
{
    set_duty_cycle(pwm_gpio_to_slice_num(EN_A_PIN), wrap_speed * left_adjustments * turn_value); // left
    set_duty_cycle(pwm_gpio_to_slice_num(EN_B_PIN), wrap_speed * turn_value);
    move_forward();
}

void set_duty_cycle(uint slice_num, uint16_t duty_cycle)
{
    pwm_set_chan_level(slice_num, PWM_CHAN_A, duty_cycle);
}

void init_motor_control()
{
    gpio_init(IN1_PIN);
    gpio_set_dir(IN1_PIN, GPIO_OUT);
    gpio_init(IN2_PIN);
    gpio_set_dir(IN2_PIN, GPIO_OUT);

    gpio_init(IN3_PIN);
    gpio_set_dir(IN3_PIN, GPIO_OUT);
    gpio_init(IN4_PIN);
    gpio_set_dir(IN4_PIN, GPIO_OUT);

    // Initialize PWM for EN_A and EN_B
    gpio_set_function(EN_A_PIN, GPIO_FUNC_PWM);
    gpio_set_function(EN_B_PIN, GPIO_FUNC_PWM);

    uint slice_num_a = pwm_gpio_to_slice_num(EN_A_PIN);
    uint slice_num_b = pwm_gpio_to_slice_num(EN_B_PIN);
    // Set frequency for PWM
    pwm_set_clkdiv_int_frac(slice_num_a, 1, 15);
    pwm_set_clkdiv_int_frac(slice_num_b, 1, 15);

    pwm_set_wrap(slice_num_a, 64515);
    pwm_set_wrap(slice_num_b, 64515);
    pwm_set_enabled(slice_num_a, true);
    pwm_set_enabled(slice_num_b, true);
}

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
	// Setup GPIO and interrupts for the ultrasonic sensor.
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
	// If an object is detected within 5 cm, move backward; otherwise, move forward.
}
    stdio_init_all();
    hcsr04_init();

    uint32_t last_trigger_time = 0;

    while (1) {
        // Trigger a new measurement every 500 ms
        if (time_us_32() - last_trigger_time > 500000)
        {
            hcsr04_trigger_measurement();
            last_trigger_time = time_us_32();
        }

        // Check if a new measurement is available
        if (new_measurement_available)
        {
            float distance = hcsr04_calculate_distance_cm();
            printf("Distance: %.2f cm\n", distance);

            if (distance < 5.0)
            {
                moveBackward();
            }
            else
            {
                move_forward_both();
            }
            new_measurement_available = false;
        }
    }

    return 0;
}
