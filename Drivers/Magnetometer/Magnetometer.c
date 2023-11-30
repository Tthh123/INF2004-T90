#include "LSM303DLHC.H"

// Initialize the I2C communication for the LSM303 sensor
void custom_i2c_init(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin) {
    i2c_init(i2c, 400 * 1000);  // Set I2C communication speed to 400 kHz
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);  // Assign SDA function to pin
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);  // Assign SCL function to pin
    gpio_pull_up(sda_pin);  // Enable the internal pull-up resistor for SDA line
    gpio_pull_up(scl_pin);  // Enable the internal pull-up resistor for SCL line
}

// Write a register value to the magnetometer
void LSM303_write_mag_reg(LSM303 *lsm303, uint8_t reg, uint8_t value) {
    uint8_t buffer[] = {reg, value};
    i2c_write_blocking(lsm303->i2c, lsm303->mag_address, buffer, 2, false); // Write the register and value over I2C
}

// Read magnetometer data from a specific register
int16_t LSM303_read_mag_data(LSM303 *lsm303, uint8_t reg) {
    uint8_t buffer[2];
    i2c_write_blocking(lsm303->i2c, lsm303->mag_address, &reg, 1, true); // Send register address
    i2c_read_blocking(lsm303->i2c, lsm303->mag_address, buffer, 2, false); // Read 2-byte data from the magnetometer
    return (int16_t)(buffer[0] << 8 | buffer[1]); // Combine the bytes into a single 16-bit value
}

// Configure the magnetometer with default settings
void LSM303_enable_default(LSM303 *lsm303) {
    // Write configuration settings to magnetometer registers
    LSM303_write_mag_reg(lsm303, CRA_REG_M, 0x14); // Set data rate
    LSM303_write_mag_reg(lsm303, CRB_REG_M, 0x20); // Set gain
    LSM303_write_mag_reg(lsm303, MR_REG_M, 0x00);  // Set continuous conversion mode
}

// Enable and configure the accelerometer
void LSM303_enable_accelerometer(LSM303 *lsm303){
    // Enable accelerometer with a 50 Hz data rate and normal power mode
    LSM303_write_accel_reg(lsm303, CTRL_REG1_A, 0x27);
    // Set accelerometer scale to ±2g and enable high resolution mode
    LSM303_write_accel_reg(lsm303, CTRL_REG4_A, 0x08);
}

// Write a value to a register of the accelerometer
void LSM303_write_accel_reg(LSM303 *lsm303, uint8_t reg, uint8_t value) {
    uint8_t buffer[] = {reg, value};
    i2c_write_blocking(lsm303->i2c, lsm303->accel_address, buffer, 2, false); // Write the register and value over I2C
}

// Read accelerometer data from a specific register
int16_t LSM303_read_accel_data(LSM303 *lsm303, uint8_t reg) {
    uint8_t buffer[2];
    i2c_write_blocking(lsm303->i2c, lsm303->accel_address, &reg, 1, true); // Send register address
    i2c_read_blocking(lsm303->i2c, lsm303->accel_address, buffer, 2, false); // Read 2-byte data from the accelerometer
    return (int16_t)(buffer[0] | buffer[1] << 8); // Combine the bytes into a single 16-bit value
}

// Calculate tilt angles based on accelerometer data
void calculate_tilt_angles(LSM303 *lsm303) {
    // Read accelerometer data for all three axes
    int16_t accel_x = LSM303_read_accel_data(lsm303, OUT_X_L_A | 0x80);
    int16_t accel_y = LSM303_read_accel_data(lsm303, OUT_Y_L_A | 0x80);
    int16_t accel_z = LSM303_read_accel_data(lsm303, OUT_Z_L_A | 0x80);

    // Convert raw data to g-forces and calculate tilt angles
    double pitch = atan2(accel_x, sqrt(accel_y * accel_y + accel_z * accel_z)) * 180.0 / M_PI;
    double roll = atan2(accel_y, sqrt(accel_x * accel_x + accel_z * accel_z)) * 180.0 / M_PI;

    // Output the calculated pitch and roll angles
    printf("Pitch: %f degrees\n", pitch);
    printf("Roll: %f degrees\n", roll);
}

// Detect if the device is in free-fall
void detect_free_fall(LSM303 *lsm303) {
    // Read accelerometer data for all three axes
    int16_t accel_x = LSM303_read_accel_data(lsm303, OUT_X_L_A | 0x80);
    int16_t accel_y = LSM303_read_accel_data(lsm303, OUT_Y_L_A | 0x80);
    int16_t accel_z = LSM303_read_accel_data(lsm303, OUT_Z_L_A | 0x80);

    // Compute the magnitude of the acceleration vector
    double magnitude = sqrt(accel_x * accel_x + accel_y * accel_y + accel_z * accel_z);

    // Check if the magnitude is below a threshold that indicates free-fall
    if (magnitude < 500) {
        printf("Free-fall detected!\n");
    }
}

// Update the calibration data for the magnetometer: DOES NOT WORK
void update_calibration_data(MagnetometerCalibrationData *calData, int16_t magX, int16_t magY, int16_t magZ) {
    // Update the min and max values for each axis based on the new readings
    if (magX < calData->minX) calData->minX = magX;
    if (magX > calData->maxX) calData->maxX = magX;

    if (magY < calData->minY) calData->minY = magY;
    if (magY > calData->maxY) calData->maxY = magY;

    if (magZ < calData->minZ) calData->minZ = magZ;
    if (magZ > calData->maxZ) calData->maxZ = magZ;
}
