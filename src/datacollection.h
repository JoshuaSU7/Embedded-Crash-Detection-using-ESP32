#ifndef datacollection_h
#define datacollection_h

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <stdio.h>


struct SensorData {
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
};

SensorData getReading(Adafruit_MPU6050 *mpu);



#endif




