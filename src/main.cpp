#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>
#include <datacollection.h>
#include <wifi_notify.h>

Adafruit_MPU6050 mpu;
SensorData data;

float accel_mag;

//keep track of OLD values post-impact
const int hist_len = 20;
float history[hist_len]; 
int hist_index = 0;

float FREE_FALL_THRESHOLD = -7.0;
int lookback_n = 10;

bool checkFreeFall_pre(int currentIndex, int n) {
    for (int i = 0; i < n; i++) {
        int index = (currentIndex - n + i + hist_len) % hist_len;

        //check if we were in freefall within the pre-impact slice
        if (history[index] < FREE_FALL_THRESHOLD) {
            return true;
        }
    }
    return false;
}

bool fresh_values = true;


//keep track of NEW values pre-impact
const int post_len = 20;
const float post_len_for_avg = 20.0;
float post[post_len];
int post_index = 0;


float stabilization_threshold = 2;
int skip = 3;

bool checkStable() {

    for (int i = skip; i < post_len; i++) {

      if (abs(post[i]) > stabilization_threshold) {
      return false;
      }
        //check if we have stablized post-collision (indicates impact + abrupt stopping) freefall within the pre-impact slice
    }
    return true;
}

bool checkFreeFall_post() {
    for (int i = skip - 2; i < post_len; i++) {
        //check if we were in freefall within the post-impact slice
        if (post[i] < FREE_FALL_THRESHOLD) {
            return true;
        }
    }
    return false;
}

//use pre and post analyses to record a collision likelihood score
float col_llh = 0.0;
bool collision = false;
bool post_analysis = false;


bool impact = false;

float threshold = 20;

//calibrate so that stillness = acceleration of 0. Subtract gravity component. 
const int cal_length = 10;
const float cal_len_for_avg = 10.0; 
float calibration[cal_length];
int counter = 0;
float cal = 0;
float sum = 0;



void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  delay(1000);

  setupWifi();

  
  if (!mpu.begin(0x68)) {   // explicitly pass the address
    Serial.println("Failed to find MPU6050!");
  }

  Serial.println("MPU6050 Connected successfully!");


  // set aside the first few samples to calibrate the sensor, see what its default values are. Avg, subtract from future readings
  int counter = 0;

  for (int counter = 0; counter < cal_length; counter++) {
    data = getReading(&mpu);
    accel_mag = sqrt((data.accel.acceleration.x * data.accel.acceleration.x) + (data.accel.acceleration.y * data.accel.acceleration.y) + (data.accel.acceleration.z * data.accel.acceleration.z)) - cal;
    calibration[counter] = accel_mag;
    delay(10);
  }

  for (int i = 0; i < cal_length; i++) {
    sum += calibration[i];
  }

  cal = sum / cal_len_for_avg;

}

void loop() {


  //collect incoming data
  data = getReading(&mpu);
  accel_mag = sqrt((data.accel.acceleration.x * data.accel.acceleration.x) + (data.accel.acceleration.y * data.accel.acceleration.y) + (data.accel.acceleration.z * data.accel.acceleration.z)) - cal;
  Serial.printf("acceleration mag: %f\n", accel_mag);
  delay(100);


  //Keep track of old readings using history array


  //if we go over threshold, create a post analysis array of post_len samples and perform post-analysis
  if (impact) {

    if (post_index < post_len) {
      post[post_index] = accel_mag;
      post_index += 1;
    }


    else {
      //if there was an impact, and then the device is still immediately after -> collision
      if (checkStable()) {
        col_llh += 0.6;
      }

      //if there was an impact, and then the device is falling immediately after -> collision
      if (checkFreeFall_post()) {
        col_llh += 0.6;
      }
      impact = false;
      fresh_values = false;
      hist_index = 0;
      Serial.println("COMPLETED POST ANALYSIS");
      post_analysis = true;
    }
    
  }

  if (col_llh > 0.5 and not collision and post_analysis) {
    collision = true;
    Serial.printf("COLLISION DETECTED, collision llh: %f\n", col_llh);
    notifyCrash(col_llh);
  }
 
  //define running history of hist_len values, keeping track of when we get fresh ones immediately after a impact
  if (not impact) {
    history[hist_index] = accel_mag;
    hist_index = (hist_index + 1) % hist_len;

    if (hist_index > lookback_n) {

      fresh_values = true;
    }
  }

  //check for if our incoming acceleration crosses our threshold, and only allow this when we are not performing post analysis or when we have enough fresh values after post analysis
  if (accel_mag > threshold and not impact and fresh_values) {
    col_llh = 0;
    col_llh += ((accel_mag - threshold) / 100.0);
    collision = false;
    impact = true;
    post_analysis = false;
    post_index = 0;
    //look back some samples. Were we in free-fall? (negative acceleration with magnitude 5-9)
    checkFreeFall_pre(hist_index, lookback_n);

    //if there was a fall right before impact -> increase likelihood score
    if (checkFreeFall_pre(hist_index, lookback_n)) {
      col_llh += 0.4;
    }
  }


}