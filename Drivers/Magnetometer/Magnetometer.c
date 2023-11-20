
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define MAGNETOMETER_I2C_ADDR 0x1E // 7-bit address for LSM303 magnetometer

// Register addresses
#define CRA_REG_M 0x00
#define CRB_REG_M 0x01
#define MR_REG_M 0x02
#define OUT_X_H_M 0x03

typedef struct {
    i2c_inst_t *i2c;
    uint8_t mag_address;
} LSM303;

void custom_i2c_init(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin) {
    i2c_init(i2c, 400 * 1000);  // Initialize I2C with 400 kHz rate
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);  // Set pin for SDA
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);  // Set pin for SCL
    gpio_pull_up(sda_pin);  // Enable pull-up for SDA
    gpio_pull_up(scl_pin);  // Enable pull-up for SCL
}

void LSM303_write_mag_reg(LSM303 *lsm303, uint8_t reg, uint8_t value) {
    uint8_t buffer[] = {reg, value};
    i2c_write_blocking(lsm303->i2c, lsm303->mag_address, buffer, 2, false);
}

int16_t LSM303_read_mag_data(LSM303 *lsm303, uint8_t reg) {
    uint8_t buffer[2];
    i2c_write_blocking(lsm303->i2c, lsm303->mag_address, &reg, 1, true); // true to keep master control of bus
    i2c_read_blocking(lsm303->i2c, lsm303->mag_address, buffer, 2, false); // false - finished with bus
    return (int16_t)(buffer[0] << 8 | buffer[1]);
}

void LSM303_enable_default(LSM303 *lsm303) {
    LSM303_write_mag_reg(lsm303, CRA_REG_M, 0x0C);
    LSM303_write_mag_reg(lsm303, CRB_REG_M, 0x20);
    LSM303_write_mag_reg(lsm303, MR_REG_M, 0x00);
}

int main() {
    stdio_init_all();
    LSM303 lsm303 = {
        .i2c = i2c0,
        .mag_address = MAGNETOMETER_I2C_ADDR
    };

    custom_i2c_init(lsm303.i2c, 0, 1);
    LSM303_enable_default(&lsm303);

    while (1) {
        int16_t mag_x = LSM303_read_mag_data(&lsm303, OUT_X_H_M);
        int16_t mag_z = LSM303_read_mag_data(&lsm303, OUT_X_H_M + 2); // Auto address increment
        int16_t mag_y = LSM303_read_mag_data(&lsm303, OUT_X_H_M + 4); // Auto address increment

        printf("Magnetometer Data - X: %d, Y: %d, Z: %d\n", mag_x, mag_y, mag_z);
        sleep_ms(1000);
    }

    return 0;
}
