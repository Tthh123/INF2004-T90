// main.c
#include "LSM303DLHC.H"
#include <stdint.h>


int main() {
    stdio_init_all();
    LSM303 lsm303 = {
        .i2c = i2c0,
        .mag_address = MAGNETOMETER_I2C_ADDR
    };

    MagnetometerCalibrationData calData = {
    .minX = INT16_MAX, .maxX = INT16_MIN,
    .minY = INT16_MAX, .maxY = INT16_MIN,
    .minZ = INT16_MAX, .maxZ = INT16_MIN
};

    custom_i2c_init(lsm303.i2c, 0, 1);
    LSM303_enable_default(&lsm303);
    LSM303_enable_accelerometer(&lsm303);

    while (1) {
        int16_t mag_x = LSM303_read_mag_data(&lsm303, OUT_X_H_M);
        int16_t mag_z = LSM303_read_mag_data(&lsm303, OUT_X_H_M + 2); // Auto address increment
        int16_t mag_y = LSM303_read_mag_data(&lsm303, OUT_X_H_M + 4); // Auto address increment
        // Assuming magX, magY, magZ are your current magnetometer readings
        update_calibration_data(&calData, magX, magY, magZ);

        // Read accelerometer data
        //int16_t accel_x = LSM303_read_accel_data(&lsm303, OUT_X_L_A | 0x80);
        //int16_t accel_y = LSM303_read_accel_data(&lsm303, OUT_Y_L_A | 0x80);
        //int16_t accel_z = LSM303_read_accel_data(&lsm303, OUT_Z_L_A | 0x80);



        // Calculate heading
        float heading = atan2((float)mag_y, (float)mag_x);
        heading = heading * (180 / M_PI); // Convert to degrees
        if (heading < 0) heading += 360;

        //Print magnetometer data and heading
        printf("Magnetometer Data - X: %d, Y: %d, Z: %d, Heading: %f\n", mag_x, mag_y, mag_z, heading);

        sleep_ms(1000);
    }

    return 0;
}


