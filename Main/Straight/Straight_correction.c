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
#define ADC_X_AXIS 27
#define ADC_Y_AXIS 26

#define IR_THRESHOLD 1000

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
    adc_init();
    adc_gpio_init(ADC_X_AXIS);
    adc_gpio_init(ADC_Y_AXIS);
}

int main()
{
    stdio_init_all();

    uint32_t last_trigger_time = 0;

    while (1)
    {
        move_forward_both();
        // Read ADC value from GPIO 26 and GPIO 27
        adc_select_input(0); // Select ADC channel for GPIO 26
        sensor_value_26 = adc_read();
        adc_select_input(1); // Select ADC channel for GPIO 27
        sensor_value_27 = adc_read();

        if (sensor_value_26 > IR_THRESHOLD)
        {
            move_turn_right();
            sleep_ms(150);
            move_turn_left();
            sleep_ms(85);
        }
        else if (sensor_value_27 > IR_THRESHOLD)
        {
            move_turn_left();
            sleep_ms(150);
            move_turn_right();
            sleep_ms(85);
        }
    }

    return 0;
}
