# Sun tracker
Project for tracking position of the sun on the sky. 
This device scans certain part of sphere and searches for the brightest point on it. 
This project can be used when the sun position is important, e.g. in solar panel system or any other. 
You can adapt and modify this code as you need.

## Hardware

### List of components
* ESP8266 NodeMCU v2
* 2x servo SG90 analog
* PCA9685 16-channel PWM servo driver
* photoresistor + pull up 10k resistor
* hand made skeleton
* some RC parts for step motors and servos

## Scripts

### fix_insolation_matrix.py
This script reads insolation matrix from file `insolation_matrix.csv`, fixes points orientation and writes this to `proper_insolation_matrix.csv`.
It is required because the sun tracker returns the insolation values in improper order (movements optimization). 

### insolation_matrix_to_png.py
It reads fixed file `proper_insolation_matrix.csv` and generates png file to visualize the sun tracker results.

## Generate PNG file
Let's read insolation values from tracker, e.g. using serial port monitor in Arduino IDE. 
They are in CSV format. 
Save them into file `insolation_matrix.csv` and run:
```
pip install -r requirements.txt
python fix_insolation_matrix.py
python insolation_matrix_to_png.py
```
Finally you will have `insolation.png` file.

Enjoy!
