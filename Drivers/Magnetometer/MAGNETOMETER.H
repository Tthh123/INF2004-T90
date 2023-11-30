// LSM303DLHC.h
#ifndef LSM303DLHC_H
#define LSM303DLHC_H

#include "hardware/i2c.h"
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include <math.h>

// LSM303DLHC I2C address and register addresses
#define MAGNETOMETER_I2C_ADDR 0x1E
// Register addresses
#define CRA_REG_M 0x00
#define CRB_REG_M 0x01
#define MR_REG_M 0x02
#define OUT_X_H_M 0x03
//accel
#define CTRL_REG1_A 0X20
#define CTRL_REG4_A 0x23
#define OUT_X_L_A 0x28
#define OUT_Y_L_A 0x2A
#define OUT_Z_L_A 0x2C

#define M_PI 3.14159265358979323846 // Defining Pi


typedef struct {
    i2c_inst_t *i2c;
    uint8_t accel_address;
    uint8_t mag_address;
} LSM303;

//for calibration
typedef struct {
    int16_t minX;
    int16_t maxX;
    int16_t minY;
    int16_t maxY;
    int16_t minZ;
    int16_t maxZ;
} MagnetometerCalibrationData;

void custom_i2c_init(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin);

// Initialization functions
void LSM303_enable_accelerometer(LSM303 *lsm303);
void LSM303_enable_default(LSM303 *lsm303);

// Register write functions
void LSM303_write_accel_reg(LSM303 *lsm303, uint8_t reg, uint8_t value);
void LSM303_write_mag_reg(LSM303 *lsm303, uint8_t reg, uint8_t value);

// Data read functions
int16_t LSM303_read_accel_data(LSM303 *lsm303, uint8_t reg);
int16_t LSM303_read_mag_data(LSM303 *lsm303, uint8_t reg);

// Function to calculate tilt angles
void calculate_tilt_angles(LSM303 *lsm303);

//Function to detect free fall
void detect_free_fall(LSM303 *lsm303);

//for calibration
void update_calibration_data(MagnetometerCalibrationData *calData, int16_t magX, int16_t magY, int16_t magZ);

#endif // LSM303DLHC_H
