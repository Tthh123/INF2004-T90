#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdlib.h>

#define THRESHOLD 1100
#define IR_SENSOR_PIN 26
#define INITIAL_SIZE 5

volatile bool white_surface_detected = false;
absolute_time_t white_surface_start_time;
absolute_time_t black_surface_start_time;
uint32_t elapsed_seconds = 0;

int *array;
int size, capacity;

// Define a struct to represent Code 39 characters and their corresponding binary strings
struct Code39Character
{
    char character;
    const char *binary;
};

// Function to match a binary string to its corresponding Code 39 character
char findCode39Character(const char *barcodeString, const struct Code39Character *characters, int numCharacters)
{
    // Iterate through the Code 39 characters to find a match
    for (int i = 0; i < numCharacters; i++)
    {
        if (strcmp(barcodeString, characters[i].binary) == 0)
        {
            return characters[i].character; // Return the matched character
        }
    }
    return '?'; // Else return '?' if no match is found
}

// Function to initialize a dynamic array for storing integers
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

// Function to append a value to the dynamic array, resizing it if necessary
void appendValue(int **array, int *size, int *capacity, int value)
{
    // Check if resizing is needed
    if (*size >= *capacity)
    {
        //If the current size exceeds the capacity, reallocate more space
        *capacity *= 2; // Double capacity
        *array = (int *)realloc(*array, *capacity * sizeof(int)); // Reallocate memory
        if (*array == NULL)
        {
            fprintf(stderr, "Memory reallocation failed.\n");
            exit(1); // Exit if reallocation fails
        }
    }

    (*array)[(*size)++] = value; // Append the value and increment the size
}

// Function to free the memory allocated to the dynamic array
void freeDynamicArray(int *array)
{
    free(array); /
}

// Function to convert the contents of the dynamic array into a string
char *concatenateArrayToString(int *array, int size)
{
    char *result = (char *)malloc((size * 4 + 1) * sizeof(char)); // Allocate memory for the string
    if (result == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1); 
    }

    int offset = 0; // Initialize offset for string concatenation
    for (int i = 0; i < size; i++)
    {
        int value = array[i];
        // Convert the integer to a string and concatenate
        int written = snprintf(result + offset, (size * 4 + 1) - offset, "%d", value);
        if (written < 0 || written >= (size * 4 + 1) - offset)
        {
            fprintf(stderr, "Error converting integer to string.\n");
            free(result); // Free memory if conversion fails
            exit(1);
        }
        offset += written; // Update the offset
    }
    result[offset] = '\0'; 

    return result; // Return the concatenated string
}

// Interrupt handler for detecting changes in surface color
void white_surface_detected_handler(uint gpio, uint32_t events)
{
    // Handle falling edge events (transition from black to white)
    if (events & GPIO_IRQ_EDGE_FALL)
    {
        // Check the elapsed time to determine the pattern
        if (elapsed_seconds < THRESHOLD)
        {
            appendValue(&array, &size, &capacity, 1); 
        }
        else if (elapsed_seconds < 5000 && elapsed_seconds > THRESHOLD)
        {
            appendValue(&array, &size, &capacity, 111); 
        }

        // Update flags and timestamps for white surface detection
        white_surface_detected = true;
        white_surface_start_time = get_absolute_time();
    }
    // Handle rising edge events (transition from white to black)
    else if (events & GPIO_IRQ_EDGE_RISE)
    {
        // Check the elapsed time to determine the pattern
        if (elapsed_seconds < THRESHOLD)
        {
            appendValue(&array, &size, &capacity, 2); 
        }
        else if (elapsed_seconds < 5000 && elapsed_seconds > THRESHOLD)
        {
            appendValue(&array, &size, &capacity, 222); 
        }
        // Update flags and timestamps for black surface detection
        white_surface_detected = false;
        black_surface_start_time = get_absolute_time();
    }
}

int main()
{
    stdio_init_all(); 
    adc_init(); 

    // Set up ADC for reading IR sensor values
    adc_gpio_init(IR_SENSOR_PIN);
    adc_select_input(0); // Use ADC channel 0 (pin 26)

    // Initialize GPIO pin for IR sensor and set up interrupt handling
    gpio_init(IR_SENSOR_PIN);
    gpio_set_dir(IR_SENSOR_PIN, GPIO_IN); // Set IR sensor pin as input
    gpio_set_irq_enabled_with_callback(IR_SENSOR_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &white_surface_detected_handler);

    // Initialize Code 39 characters with their binary representations
    struct Code39Character characters[] = {
        // List of Code 39 characters and their corresponding binary strings
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
        {'Z', "12221211121112121222111211121212122212111211121"}
       
    };

    int numCharacters = sizeof(characters) / sizeof(characters[0]); // Calculate the number of characters

    initializeDynamicArray(&array, &size, &capacity); // Initialize the dynamic array

    while (1)
    {
        uint16_t sensor_value = adc_read(); // Read the IR sensor value
        // Detect the current surface color and calculate elapsed time
        if (white_surface_detected)
        {
            absolute_time_t current_time = get_absolute_time();
            elapsed_seconds = absolute_time_diff_us(white_surface_start_time, current_time) / 1000;
            printf("WHITE -- Time elapsed: %d seconds\n", elapsed_seconds);
        }
        else
        {
            absolute_time_t current_time = get_absolute_time();
            elapsed_seconds = absolute_time_diff_us(black_surface_start_time, current_time) / 1000;
            printf("BLACK -- Time elapsed: %d seconds\n", elapsed_seconds);
        }

        // Print the values stored in the dynamic array
        printf("Dynamic Array Values: ");
        for (int i = 0; i < size; i++)
        {
            printf("%d ", array[i]);
        }
        printf("\n");

        // Convert the array contents into a string representation
        char *barcodeString = concatenateArrayToString(array, size);
        printf("Barcode String: %s\n", barcodeString);

        // Find the corresponding Code 39 character from the binary string
        char character = findCode39Character(barcodeString, characters, numCharacters);
        printf("Code 39 Character: %c\n", character);

        // Add a delay to avoid rapid readings
        if (size >= 40)
        {
            // Free the memory allocated for the dynamic array
            freeDynamicArray(array);
            // break; // Optionally break the loop if a certain condition is met
        }
        sleep_ms(50); // Sleep for 50 milliseconds to control loop timing
    }

    return 0;
}


