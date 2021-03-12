/* 
 *  Hardware and software spec:
 *  - compiled in Arduino IDE 1.8.13 for: NodeMCU 1.0 (ESP-12E Module)
 *  - ESP8266 NodeMCU v2.
 *  - 2x servo SG90 analog.
 *  - PCA9685 16-channel PWM servo driver.
 *  - photoresistor + pull up 10k resistor.
 *  - hand mande skeleton. 
 *  - some RC parts for step motors and servos. 
 *  
 *  Changelog.
 *  v7:
 *  - removed WiFi commented code.
 *  - back to previous version of printing insolation matrix (memory issues). 
 *  - added Python script for transforming insolation matrix to proper one.
 *  
 *  v6:
 *  - commented part of code for sending data by WiFi.
 *  - added function to proper printing matrix with insolation points and their positions on sphere.
 *  
 *  v5:
 *  - added WiFi part of code (send JSON with insolation data to RESTful API server).
 *  
 *  v4:
 *  - new version of full_scan function (reversed h and v axis to avoid vibration from heavier part of mechanism).
 *  
 *  v3:
 *  - new function set_angle_microsec to calculate and make movement based on pwm.writeMicroseconds method. 
 *    It gives more smoothly and slower movements than previos function based on pwm.setPWM method.
 *    
 *  v2:
 *  - new function full_scan_max for scanning some bigger part of sphere.
 *    It gives better results than previos functions (find_brighter_position_h, find_brighter_position_v)
 *    to find point with better insolation value online. These two functions were generating problems
 *    and there are more sensitive on some fluctuactions from photoresistor.
 *    
*/

#include "Wire.h"
#include "Adafruit_PWMServoDriver.h"

// Movements range settings.
// The rest of settings is in full_scan function.
const int v_steps = 60; // the range shoud be equal 60 degrees. Why 60? It is empirical value.
const int h_steps = 60; // 30 or 60 steps; 

// Servo settings.
const int SERVO_MIN_MICROSEC_LEN = 500;  // rounded 'minimum' microsecond length based on the minimum pulse of 100.
const int SERVO_MAX_MICROSEC_LEN = 2600; // rounded 'maximum' microsecond length based on the maximum pulse of 600.
const int SERVO_FREQ = 50; // analog servos run at ~50 Hz updates

// H - horizontal.
// V - vertical.
const int H = 1;
const int V = 0;

// D1 -> SCL.
// D2 -> SDA.
// 0x40 - PCA9685.
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40); 

void setup() {
  Serial.begin(115200);

  // Servo driver init.
  pwm.begin();
  pwm.setOscillatorFrequency(25000000);
  pwm.setPWMFreq(SERVO_FREQ);
}


// This function takes servo number and angle to set and make this movement on given servo number.
// The writeMicrosecond function gives me more smoothly movement than setPWM method because of loop is more dense.
int servo_prev_angle[16] = {};  // 16 servo channels.
void set_angle_microsec(int servo_nr, int new_angle) {

  int prev_angle = servo_prev_angle[servo_nr];
  if (new_angle == prev_angle) {
    return;
  }
  
  // Map angles to microseconds.
  int prev_microsec = map(prev_angle, 0, 180, SERVO_MIN_MICROSEC_LEN, SERVO_MAX_MICROSEC_LEN);
  int new_microsec  = map(new_angle,  0, 180, SERVO_MIN_MICROSEC_LEN, SERVO_MAX_MICROSEC_LEN);

  // Run servo.
  int steps        = abs(new_microsec - prev_microsec);
  int inc_microsec = (new_microsec > prev_microsec) ? 1 : -1;
  int microsec     = prev_microsec;
  for(int i=0; i < steps; i++) {
    pwm.writeMicroseconds(servo_nr, microsec);
    microsec += inc_microsec;
  }

  // Save new angle for next movements.
  servo_prev_angle[servo_nr] = new_angle;
}


/* Best insolation attributes */
int best_insolation = 0; // higher value means brighter.
int best_angle_h = 0;
int best_angle_v = 0;

const int scan_delay = 5 * 60 * 1000; // in minutes
void loop() {
  /* 
     Find best position with best insolation.
     I always run full scan because of:
      - sometimes the clouds cover the sun. In this case for partial scanning the sensor can go in wrong place (local maximum).
      - less problems to solve.
      - simpler code.
      - better precision for full scanning instead of partial scanning.
   */
  full_scan();
  delay(1000);

  set_angle_microsec(V, best_angle_v);
  set_angle_microsec(H, best_angle_h);
  print_best_position_info();
  
  delay(scan_delay);
}


// Scanning defined part of sphere for finding best position of h, v with max insolation value.
// Horizontal movement is less often because this axis has bigger inercy and causes more vibration.
void full_scan() {
  
  // Reset best values.
  best_insolation = 0;
  best_angle_h = 0;
  best_angle_v = 0;  

  int current_insolation = 0; 

  /* 
   *  Definition of sphere to scan. 
   *  Starting point and range of movements.
  */

  // Movement in (v)ertical layer - processing in this way to eliminate one not needed transition. Final route has rectangular shape.
  int flag_v = 0; // to change direction of movement. 
  int v_inc  = -1;
  const int start_angle_v = 25; // 25 degree of sensor arm means horizontal position of this arm.
  const int stop_angle_v  = start_angle_v + (v_steps * abs(v_inc)); 
  
  // Movement in (h)orizontal layer.
  // Range 45-135 degrees (90 degrees wide), 3inc step --> 90/3 = 30 steps.
  // or 30-150 degrees (120 degrees wide), 2inc step --> 120/2 = 60 steps.
  int h     = 150; // 135 or 150
  int h_inc = -2; // -3 or -2

  // Main loop.
  const int delay_h_v = 30;
  for (int i=0; i < h_steps; i++) {
    
    // Set new h position.
    set_angle_microsec(H, h);
    delay(delay_h_v);

    // Reverse order of v iteration.
    int v = (flag_v % 2) ? stop_angle_v : start_angle_v;
    v_inc *= -1;

    // Movement in vertical layer.
    for (int j=0; j < v_steps; j++) {

      // Set new v position.
      set_angle_microsec(V, v);
      v += v_inc;
      
      current_insolation = analogRead(A0);
      if (current_insolation > best_insolation) {
        best_insolation = current_insolation;
        best_angle_h = h;
        best_angle_v = v;
      }

      Serial.print(current_insolation);
      if (j < v_steps) {
        Serial.print(",");
      }

      delay(delay_h_v);
    }
    Serial.println();

    flag_v++;
    h += h_inc;
  }
}


void print_best_position_info() {
  Serial.print("best values: v(");
  Serial.print(best_angle_v);
  Serial.print("), h(");
  Serial.print(best_angle_h);  
  Serial.print("), insolation: ");
  Serial.print(best_insolation);
  Serial.println("");
}

