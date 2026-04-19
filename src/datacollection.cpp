#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <stdio.h>
#include <datacollection.h>




SensorData getReading(Adafruit_MPU6050 *mpu_obj) {
    SensorData data;
    mpu_obj->getEvent(&data.accel, &data.gyro, &data.temp);
    return data;
}