/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwipopts.h"
#include "lwip/ip4_addr.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ping.h"
#include "lwip/apps/httpd.h"
#include "hardware/adc.h"
#include "message_buffer.h"

#include "hardware/pwm.h"

#include "hardware/timer.h"
#include "hardware/irq.h"

// #include "motor_driver.h"
// #include "encoder_driver.h"
// #include "pid.h"
// // #include "driver/pid/pid_autotune.h"
// #include "ultrasonic_driver.h"

// SSI tags - tag length limited to 8 bytes by default
const char *ssi_tags[] = {"volt", "temp", "led", "char"};

#ifndef PING_ADDR
#define PING_ADDR "142.251.35.196"
#endif
#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0

#endif

#define TEST_TASK_PRIORITY (tskIDLE_PRIORITY + 1UL)

static MessageBufferHandle_t xSimpleMessageBuffer;
static MessageBufferHandle_t xMovingMessageBuffer;
static MessageBufferHandle_t xPrintSimpleAvgMessageBuffer;
static MessageBufferHandle_t xPrintMovingAvgMessageBuffer;

#define THRESHOLD 100
#define IR_SENSOR_PIN 26
#define INITIAL_SIZE 5

volatile bool white_surface_detected = false;
absolute_time_t white_surface_start_time;
absolute_time_t black_surface_start_time;
uint32_t elapsed_seconds = 0;

int *array;
int size, capacity;

char character;

absolute_time_t timeDiff;

#define RIGHT_PWM 0
#define LEFT_PWM 1
#define RIGHT_BACKWARD 2
#define RIGHT_FORWARD 3
#define LEFT_BACKWARD 4
#define LEFT_FORWARD 5
#define TRIG_PIN 6
#define ECHO_PIN 7
#define WHEEL_ENCODER_1 28
#define WHEEL_ENCODER_2 27
#define DEBOUNCE_TIME_MS 10 // Debounce time in milliseconds
#define TOTAL_NOTCHES_PER_REVOLUTION 20
#define WHEEL_CIRCUMFERENCE_CM 21

u_int8_t notch_count = 0;
volatile uint32_t start_time = 0;
volatile uint32_t end_time = 0;
volatile bool new_measurement_available = false;

uint32_t last_interrupt_time_ms = 0;

bool startMeasuringRight = false;
uint32_t lastRisingRight = 0;
uint32_t lastFallingRight = 0;
uint32_t totalPeriodRight = 0;
uint32_t notchPeriodRight = 0;
double totalDistRight = 0.0;
float currSpeedRight = 0.0;

bool startMeasuringLeft = false;
uint32_t lastRisingLeft = 0;
uint32_t lastFallingLeft = 0;
uint32_t totalPeriodLeft = 0;
uint32_t notchPeriodLeft = 0;
double totalDistLeft = 0.0;
float currSpeedLeft = 0.0;

float Kp_right = 1.0, Ki_right = 0.01, Kd_right = 0.05;
float Kp_left = 0.90, Ki_left = 0.01, Kd_left = 0.05;
float error_right = 0, integral_right = 0, derivative_right = 0,
      last_error_right = 0;
float error_left = 0, integral_left = 0, derivative_left = 0,
      last_error_left = 0;
float target_speed_right = 20.0; // Set your target speeds here
float target_speed_left = 20.0;

bool forwardMovement = 1;

uint32_t counter = 0;

#pragma region SSI

u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen)
{
    size_t printed;
    switch (iIndex)
    {
    case 0: // volt
    {
        const float voltage = adc_read() * 3.3f / (1 << 12);
        printed = snprintf(pcInsert, iInsertLen, "%f", voltage);
    }
    break;
    case 1: // temp
    {
        const float voltage = adc_read() * 3.3f / (1 << 12);
        const float tempC = 27.0f - (voltage - 0.706f) / 0.001721f;
        printed = snprintf(pcInsert, iInsertLen, "%f", tempC);
    }
    break;
    case 2: // led
    {
        bool led_status = cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN);
        if (led_status == true)
        {
            printed = snprintf(pcInsert, iInsertLen, "ON");
        }
        else
        {
            printed = snprintf(pcInsert, iInsertLen, "OFF");
        }
    }
    break;
    case 3: // char
    {
        printed = snprintf(pcInsert, iInsertLen, "%c", character);
        break;
    }
    default:
        printed = 0;
        break;
    }

    return (u16_t)printed;
}

// Initialise the SSI handler
void ssi_init()
{
    // Initialise ADC (internal pin)
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
}
#pragma endregion

#pragma region CGI
// CGI handler which is run when a request for /led.cgi is detected
const char *cgi_led_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    // Check if an request for LED has been made (/led.cgi?led=x)
    if (strcmp(pcParam[0], "led") == 0)
    {
        // Look at the argument to check if LED is to be turned on (x=1) or off (x=0)
        if (strcmp(pcValue[0], "0") == 0)
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        else if (strcmp(pcValue[0], "1") == 0)
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    }

    // Send the index page back to the user
    return "/index.shtml";
}

// tCGI Struct
// Fill this with all of the CGI requests and their respective handlers
static const tCGI cgi_handlers[] = {
    {// Html request for "/led.cgi" triggers cgi_handler
     "/led.cgi", cgi_led_handler},
};

void cgi_init(void)
{
    http_set_cgi_handlers(cgi_handlers, 1);
}
#pragma endregion

#pragma region barcode
// Define a struct to hold the Code 39 characters and their binary representations
struct Code39Character
{
    char character;
    const char *binary;
};

// Function to initialize the dynamic array
void initializeDynamicArray(int **array, int *size, int *capacity)
{
    *size = 0;
    *capacity = INITIAL_SIZE;
    *array = (int *)malloc(INITIAL_SIZE * sizeof(int));
    if (*array == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }
}

// Function to append a value to the dynamic array
void appendValue(int **array, int *size, int *capacity, int value)
{
    if (*size >= *capacity)
    {
        // If the current size exceeds the capacity, reallocate more space
        *capacity *= 2;
        *array = (int *)realloc(*array, *capacity * sizeof(int));
        if (*array == NULL)
        {
            fprintf(stderr, "Memory reallocation failed.\n");
            exit(1);
        }
    }

    (*array)[(*size)++] = value;
}

void freeDynamicArray(int *array)
{
    free(array);
}

void resetDynamicArray(int *size)
{
    *size = 0; // Simply reset the size to 0, effectively clearing the array
}

// Function to concatenate values in the dynamic array into a string
char *concatenateArrayToString(int *array, int size)
{
    char *result = (char *)malloc((size * 4 + 1) * sizeof(char)); // +1 for the null-terminator
    if (result == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    int offset = 0;
    for (int i = 0; i < size; i++)
    {
        int value = array[i];
        // Convert the integer to a string and append it to the result
        int written = snprintf(result + offset, (size * 4 + 1) - offset, "%d", value);
        if (written < 0 || written >= (size * 4 + 1) - offset)
        {
            fprintf(stderr, "Error converting integer to string.\n");
            free(result);
            exit(1);
        }
        offset += written;
    }
    result[offset] = '\0'; // Null-terminate the string

    return result;
}

void white_surface_detected_handler(uint gpio, uint32_t events)
{
    absolute_time_t current_time = get_absolute_time();
    if (events & GPIO_IRQ_EDGE_FALL) // black to white surface
    {
        elapsed_seconds = absolute_time_diff_us(black_surface_start_time, current_time) / 1000;
        if (elapsed_seconds < THRESHOLD)
        {
            printf("Black Time elapsed: %d seconds\n", elapsed_seconds);
            appendValue(&array, &size, &capacity, 1);
        }
        else if (elapsed_seconds < 5000 && elapsed_seconds > THRESHOLD)
        {
            printf("Black Time elapsed: %d seconds\n", elapsed_seconds);

            appendValue(&array, &size, &capacity, 111);
        }

        white_surface_detected = true;
        white_surface_start_time = get_absolute_time();
    }
    else if (events & GPIO_IRQ_EDGE_RISE) // white to black surface
    {
        elapsed_seconds = absolute_time_diff_us(white_surface_start_time, current_time) / 1000;
        if (elapsed_seconds < THRESHOLD)
        {
            printf("White Time elapsed: %d seconds\n", elapsed_seconds);

            appendValue(&array, &size, &capacity, 2);
        }
        else if (elapsed_seconds < 5000 && elapsed_seconds > THRESHOLD)
        {
            printf("White Time elapsed: %d seconds\n", elapsed_seconds);

            appendValue(&array, &size, &capacity, 222);
        }
        white_surface_detected = false;
        black_surface_start_time = get_absolute_time();
    }
}

#pragma endregion

#pragma region pid

float calculate_speed(uint32_t notch_period_ms)
{
    // Convert time from milliseconds to seconds
    double time_s = (double)notch_period_ms / 1000.0;

    // Calculate distance per notch
    double distance_per_notch = WHEEL_CIRCUMFERENCE_CM / TOTAL_NOTCHES_PER_REVOLUTION;

    // Calculate speed (cm/s)
    double speed = distance_per_notch / time_s;
    return speed;
}

// Callback function for button press with debounce
void handle_notch(uint gpio, uint32_t events)
{
    // Debounce algorithm using time (100ms debounce time)
    absolute_time_t current_time = get_absolute_time();
    uint32_t current_timestamp_ms = to_ms_since_boot(current_time);

    if ((current_timestamp_ms - last_interrupt_time_ms) < DEBOUNCE_TIME_MS)
    {
        // Ignore events within the debounce time
        return;
    }

    if (gpio == IR_SENSOR_PIN)
    {
        if (events & GPIO_IRQ_EDGE_FALL) // black to white surface
        {
            elapsed_seconds = absolute_time_diff_us(black_surface_start_time, current_time) / 1000;
            if (elapsed_seconds < THRESHOLD)
            {
                appendValue(&array, &size, &capacity, 1);
            }
            else if (elapsed_seconds < 5000 && elapsed_seconds > THRESHOLD)
            {
                appendValue(&array, &size, &capacity, 111);
            }

            white_surface_detected = true;
            white_surface_start_time = get_absolute_time();
        }
        else if (events & GPIO_IRQ_EDGE_RISE) // white to black surface
        {
            elapsed_seconds = absolute_time_diff_us(white_surface_start_time, current_time) / 1000;
            if (elapsed_seconds < THRESHOLD)
            {
                appendValue(&array, &size, &capacity, 2);
            }
            else if (elapsed_seconds < 5000 && elapsed_seconds > THRESHOLD)
            {
                appendValue(&array, &size, &capacity, 222);
            }
            white_surface_detected = false;
            black_surface_start_time = get_absolute_time();
        }
    }
}

void update_motors()
{

    // printf("Right: %f\n", pwm_right);
    // printf("Left: %f\n", pwm_left);
    //   Adjust PWM duty cycle
    // set_pulse_width(0, PWM_CHAN_A, pwm_right);
    set_pulse_width(0, PWM_CHAN_B, 68);
}

void turnLeft()
{
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 0);
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 1);
}

void turnRight()
{
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 1);
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 0);
}

void moveForward()
{

    gpio_put(RIGHT_BACKWARD, 1);
    gpio_put(RIGHT_FORWARD, 0);
    gpio_put(LEFT_BACKWARD, 1);
    gpio_put(LEFT_FORWARD, 0);
}

void moveBackward()
{
    gpio_put(RIGHT_BACKWARD, 0);
    gpio_put(RIGHT_FORWARD, 1);
    gpio_put(LEFT_BACKWARD, 0);
    gpio_put(LEFT_FORWARD, 1);
}
void stopMovement()
{
    gpio_put(2, 0);
    gpio_put(3, 0);
    gpio_put(4, 0);
    gpio_put(5, 0);
}
#pragma endregion

void main_task(__unused void *params)
{
    // if (cyw43_arch_init())
    // {
    //     printf("failed to initialise\n");
    //     return;
    // }
    // cyw43_arch_enable_sta_mode();
    // printf("Connecting to Wi-Fi...\n");
    // if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    // {
    //     printf("failed to connect.\n");
    //     exit(1);
    // }
    // else
    // {
    //     printf("Connected.\n");
    // }

    // ip_addr_t ping_addr;
    // ipaddr_aton(PING_ADDR, &ping_addr);
    // ping_init(&ping_addr);

    // while (true)
    // {
    //     // not much to do as LED is in another task, and we're using RAW (callback) lwIP API
    //     vTaskDelay(100);
    // }

    // cyw43_arch_deinit();
}

void init_pins()
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
    pwm_set_chan_level(0, PWM_CHAN_A, 7500); // pin 0 right
    pwm_set_chan_level(0, PWM_CHAN_B, 8000); // pin 1 left
    pwm_set_enabled(0, true);
}

void initWifi(__unused void *params)
{
    cyw43_arch_init();

    cyw43_arch_enable_sta_mode();

    // Connect to the WiFI network - loop until connected
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0)
    {
        printf("Attempting to connect...\n");
    }
    // Print a success message once connected
    printf("Connected! \n");
    ip4_addr_t ipaddr, netmask, gw;
    struct netif *netif = netif_default;
    if (netif)
    {
        printf("IP Address: %s\n", ip4addr_ntoa(&netif->ip_addr));
        printf("Netmask: %s\n", ip4addr_ntoa(&netif->netmask));
        printf("Gateway: %s\n", ip4addr_ntoa(&netif->gw));
    }
    else
    {
        printf("Network interface not found\n");
    }

    // Initialise web server
    httpd_init();
    printf("Http server initialised\n");

    // Configure SSI and CGI handler
    ssi_init();
    printf("SSI Handler initialised\n");
    cgi_init();
    printf("CGI Handler initialised\n");

    // Set the ADC channel for the IR sensor pin
    adc_gpio_init(IR_SENSOR_PIN);
    adc_select_input(0); // Use ADC channel 0 (pin 26)

    gpio_init(IR_SENSOR_PIN);
    gpio_set_dir(IR_SENSOR_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(IR_SENSOR_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &handle_notch);
    gpio_set_irq_enabled_with_callback(WHEEL_ENCODER_1,
                                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                       true,
                                       &handle_notch);
    gpio_set_irq_enabled_with_callback(WHEEL_ENCODER_2,
                                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                       true,
                                       &handle_notch);

    printf("Start");

    // Define the list of Code 39 characters and their binary representations
    struct Code39Character characters[] = {
        {'A', "12221211121112121112121222121112122212111211121"},
        {'B', "12221211121112121211121222121112122212111211121"},
        {'C', "12221211121112121112111212221212122212111211121"},
        {'D', "12221211121112121212111222121112122212111211121"},
        {'E', "12221211121112121112121112221212122212111211121"},
        {'F', "12221211121112121211121112221212122212111211121"},
        {'G', "12221211121112121212122211121112122212111211121"},
        {'H', "12221211121112121112121222111212122212111211121"},
        {'I', "12221211121112121211121222111212122212111211121"},
        {'J', "12221211121112121212111222111212122212111211121"},
        {'K', "12221211121112121112121212221112122212111211121"},
        {'L', "12221211121112121211121212221112122212111211121"},
        {'M', "12221211121112121112111212122212122212111211121"},
        {'N', "12221211121112121212111212221112122212111211121"},
        {'O', "12221211121112121112121112122212122212111211121"},
        {'P', "12221211121112121211121112122212122212111211121"},
        {'Q', "12221211121112121212121112221112122212111211121"},
        {'R', "12221211121112121112121211122212122212111211121"},
        {'S', "12221211121112121211121211122212122212111211121"},
        {'T', "12221211121112121212111211122212122212111211121"},
        {'U', "12221211121112121112221212121112122212111211121"},
        {'V', "12221211121112121222111212121112122212111211121"},
        {'W', "12221211121112121112221112121212122212111211121"},
        {'X', "12221211121112121222121112121112122212111211121"},
        {'Y', "12221211121112121112221211121212122212111211121"},
        {'Z', "12221211121112121222111211121212122212111211121"},
    };

    int numCharacters = sizeof(characters) / sizeof(characters[0]);

    initializeDynamicArray(&array, &size, &capacity);

    while (1)
    {
        counter++;

        moveForward();
        // update_motors();
        //  uint16_t sensor_value = adc_read();
        // if (white_surface_detected)
        // {
        //     printf("WHITE -- Time elapsed: %d seconds\n", elapsed_seconds);
        // }
        // else
        // {
        //     printf("BLACK -- Time elapsed: %d seconds\n", elapsed_seconds);
        // }

        // Print all values in the dynamic array
        // printf("Dynamic Array Values: ");
        // for (int i = 0; i < size; i++)
        // {
        //     printf("%d ", array[i]);
        // }
        // printf("\n");

        // Concatenate the dynamic array into a string
        char *barcodeString = concatenateArrayToString(array, size);
        printf("Barcode String: %s\n", barcodeString);

        // Add a delay to avoid rapid readings (adjust as needed)
        if (size >= 40 || elapsed_seconds > 5000)
        {
            // Free the dynamic array
            resetDynamicArray(&size);
            // break; // Exit the loop when the array size reaches 40
        }

        if (elapsed_seconds > 120000)
        {
            resetDynamicArray(&size);
        }
        sleep_ms(50); // Sleep for 100 milliseconds
    }
}

void vLaunch(void)
{
    TaskHandle_t task;
    xTaskCreate(initWifi, "TestMainThread", configMINIMAL_STACK_SIZE, NULL, TEST_TASK_PRIORITY, &task);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUM_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

int main(void)
{
    stdio_init_all();
    adc_init();
    init_pins();

    /* Configure the hardware ready to run the demo. */
    const char *rtos_name;
#if (portSUPPORT_SMP == 1)
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif

#if (portSUPPORT_SMP == 1) && (configNUM_CORES == 2)
    printf("Starting %s on both cores:\n", rtos_name);
    vLaunch();
#elif (RUN_FREERTOS_ON_CORE == 1)
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true)
        ;
#else
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif
    return 0;
}