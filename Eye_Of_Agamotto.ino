/*
 
MIT License

Copyright (c) 2022 Crash Works 3D

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

DESCRIPTION
====================
The purpose of this code is to automate the servo and pixels for the Eye of Agamotto

ORIGINAL CREATOR
====================
MSquared94
3D Model: https://www.thingiverse.com/thing:4970358

DEVELOPED BY
====================
Crash Works 3D: Cranshark, Dropwire
Link Tree: https://linktr.ee/crashworks3d

WORKING DEMO
====================
https://www.youtube.com/watch?v=XaKzuAUb2fQ

*/

// Version.  Don't change unless authorized by Cranshark
#define VERSION "0.0.1.1"

// Referenced libraries

// For installation instructions see https://github.com/adafruit/Adafruit_SoftServo
#include "Adafruit_SoftServo.h"

// For installation instructions see: https://github.com/mathertel/OneButton
#include <OneButton.h>

// For installation instructions see https://github.com/cpldcpu/light_ws2812
#include "WS2812.h"

#ifndef __AVR_ATtiny85__
#error "Code designed for ATTiny85 chipset only."
#endif

#define SERVO_PIN                                0 // Signal pin driving the servo
#define PIXELS_PIN                               1 // Signal pin controlling the pixels
#define BUTTON_PIN                               2 // Pin for switch

#define OPEN_POS                               150 // Degrees (0-180) of the servo in the open position
#define CLOSE_POS                               10 // Degrees (0-180) of the servo in the closed position

// Declare values for servo control
#define SERVO_CLOSE_SPEED                     3000 // set the speed of the servo close function (0 = Max Speed  / 10000 = Slowest Speed) 
#define SERVO_OPEN_SPEED                      3000 // set the speed of the servo opening recommend set to max speed to aid in lift (0 = Max Speed  / 10000 = Slowest Speed) 

// Declare values for pixel control
#define PIXEL_NUM                                7 // Number of bits/LEDs in pixel (default is NeoPixel Jewel)
#define PIXEL_INCREMENT                          1 // Number to increase/decrease RGB value when changing color
#define PIXEL_DELAY                             20 // Time in milli-seconds between changing pixel color
#define PIXEL_OFF                (cRGB){ 0, 0, 0 } // RGB value (black) for "off"
#define PIXEL_COLOR_1          (cRGB){ 0, 100, 0 } // RGB value for first color (default green)
#define PIXEL_COLOR_2          (cRGB){ 100, 0, 0 } // RGB value for second color (red)
#define PIXEL_COLOR_3          (cRGB){ 0, 0, 100 } // RGB value for third color (blue)
#define PIXEL_COLOR_4      (cRGB){ 100, 100, 100 } // RGB value for fourth color (white)

cRGB curColor = PIXEL_COLOR_1; // Keeps track of the current pixels color
cRGB targetColor = PIXEL_OFF; // Keeps track of the target pixels color
int curColorState = 1; // Keeps track of the current state of the pixels color; 0 = Off
int prevColorState = 1; // Keeps track of the previous state of the pixels color
long curMillis = millis(); // Keeps track of the current number of milli-seconds that have passed since the code started running

bool isOpen = 0; // Keeps track whether or not the lid is open or closed

// Create object to control servo
Adafruit_SoftServo servo;

// Create object for WS2812 pixel LEDs
WS2812 pixels(PIXEL_NUM);

// Create object for primary button to handle
OneButton primaryButton = OneButton(BUTTON_PIN, true, true);

/**
 * Helper Method
 * Simulate a delay (milli-seconds) in processing without disabling the processor completely
 * 
 * @param[out] period - the amount of time in milliseconds to delay
 * 
 * See: https://randomnerdtutorials.com/why-you-shouldnt-always-use-the-arduino-delay-function/
*/
void simDelayMillis(unsigned long period){
  unsigned long delayMillis = millis() + period;
  while (millis() <= delayMillis)
  {
    byte x = 0; // dummy variable, does nothing
  }
}

/**
 * @brief Helper Method
 * Simulate a delay (micro-seconds) in processing without disabling the processor completely
 * 
 * @param period 
 */
void simDelayMicros(unsigned long period){
  unsigned long delayMicros = micros() + period;

  while (micros() <= delayMicros)
  {
    byte x = 0;
  }
}

/**
 * @brief Initializes the primary button
 * 
 */
void initPrimaryButton(){
    primaryButton.attachClick(handlePrimaryButtonSingleTap);
    primaryButton.attachDoubleClick(handlePrimaryButtonDoubleTap);
}

/**
 * @brief Initializes the WS2812 LED pixels
 * 
 */
void initPixels(){
    pixels.setOutput(PIXELS_PIN);
    pixels.setColorOrderRGB();
}

/**
 * @brief Method to activate the servo and move the eye lid to the open position
 * 
 */
void openEye(){
    servo.attach(SERVO_PIN); // Attaches the servo to the pin for use
    
    // Instead of immediately moving to the target position, use a
    // loop function with a delay to control the speed of movement
    for(int i = CLOSE_POS; i <= OPEN_POS; i++){
        servo.write(i); // Send position value signal to servo
        simDelayMicros(SERVO_OPEN_SPEED); // Controls the speed at which the servo moves to the next position
    }

    simDelayMillis(500); // Allow enough time for the servo to complete movement

    servo.detach(); // Stop sending signals to the servo and save power
}

/**
 * @brief Method to activate the servo and move the eye lid to the closed position
 * 
 */
void closeEye(){
    servo.attach(SERVO_PIN); // Attaches the servo to the pin for use
    
    
    // Instead of immediately moving to the target position, use a
    // loop function with a delay to control the speed of movement
    for(int i = OPEN_POS; i >= CLOSE_POS; i--){
        servo.write(i); // Send position value signal to servo
        simDelayMicros(SERVO_OPEN_SPEED); // Controls the speed at which the servo moves to the next position
    }

    simDelayMillis(500); // Allow enough time for the servo to complete movement

    servo.detach(); // Stop sending signals to the servo and save power
}

/**
 * @brief Sets the target color of the pixels object
 * 
 */
void setPixels(){
    switch (curColorState){
        case 0:
            targetColor = PIXEL_OFF;
            break;
        case 1:
            targetColor = PIXEL_COLOR_1;
            break;
        case 2:
            targetColor = PIXEL_COLOR_2;
            break;
        case 3:
            targetColor = PIXEL_COLOR_3;
            break;
        case 4:
            targetColor = PIXEL_COLOR_4;
            break;
        default:
            break;
    }
}

/**
 * @brief Turns the pixels on
 * 
 */
void pixelsOn(){
     // Checks if the pixels were off and sets to the previous color
    if(curColorState == 0){
        curColorState = prevColorState;
    }

    setPixels(); // Calls the method to set the target pixels value
}

/**
 * @brief Turns the pixels off
 * 
 */
void pixelsOff(){
    prevColorState = curColorState; // Stores the previous color state
    curColorState = 0; // Sets the current color state to "off"
    
    setPixels(); // Calls the method to set the target pixels value
}

/**
 * Method to turn the NEO LEDs on
 */
void setPixels(int r, int g, int b){
    cRGB color; // Required color object to pass to pixels object
    color.r = r; color.g = g; color.b = b; // Sets the RGB values of the color object

    // Loop through the bits/pixels
    for(uint8_t i = 0; i < PIXEL_NUM; i++){
        pixels.set_crgb_at(i, color); // Set the color value of each bit/pixel
    }
    pixels.sync(); // Activate the bits/pixels in the array of pixels
}

/**
 * @brief Combine functions to create special effects when opening the eye
 * 
 */
void openEyeFx(){
    isOpen = true; // Set that the eye is open

    pixelsOn(); // Turn on the pixels

    openEye(); // Move the servo to the open position
}

/**
 * @brief Combine functions to create special effects when closing the eye
 * 
 */
void closeEyeFx(){
    isOpen = false;  // Set that the eye is closed

    closeEye(); // Move the servo to the closed positon

    pixelsOff(); // Turn off the pixels
}

/**
 * @brief Handles what to do when the switch is activated once
 * 
 */
void handlePrimaryButtonSingleTap(){
    if(isOpen){
        closeEyeFx(); // If the eye is open, run the close special effects
    } else {
        openEyeFx(); // If the eye is closed, run the open special effects
    }
}

/**
 * @brief Handles what to do when the switch is pressed twice in rapid sequence
 * 
 */
void handlePrimaryButtonDoubleTap(){
    if(!isOpen){ // Don't do anything, the pixels should be off
        return; // Exits the method without running any code below this line
    }

    prevColorState = curColorState; // Keep track of the color state

    // Drop through the color state and set to the next state
    switch (curColorState){
        case 0:
            curColorState = 1;
            break;
        case 1:
            curColorState = 2;
            break;
        case 2:
            curColorState = 3;
            break;
        case 3:
            curColorState = 4;
            break;
        case 4:
        default:
            curColorState = 1;
            break;
    }

    pixelsOn(); // Turn on the pixels
}

/**
 * @brief Monitors the state/color of the pixels asynchronously
 * Other code can be running and the pixels can change color at the same time ;D
 */
void monitorPixels(){
    if((millis() - curMillis) > PIXEL_DELAY){ // When the timer interval is met, run this code
        curMillis = millis(); // Reset the timer

        // Begin transistioning the red color to the target red color
        if (curColor.r < targetColor.r){
            curColor.r = curColor.r + PIXEL_INCREMENT;
        } else if (curColor.r > targetColor.r) {
            curColor.r = curColor.r - PIXEL_INCREMENT;
        }
        
        // Begin transitioning the green color to the target green color
        if (curColor.g < targetColor.g){
            curColor.g = curColor.g + PIXEL_INCREMENT;
        } else if (curColor.g > targetColor.g){
            curColor.g = curColor.g - PIXEL_INCREMENT;
        }
        
        // Begin transitioning the blue color to the target blue color
        if (curColor.b < targetColor.b){
            curColor.b = curColor.b + PIXEL_INCREMENT;
        } else if (curColor.b > targetColor.b){
            curColor.b = curColor.b + PIXEL_INCREMENT;
        }

        setPixels(curColor.r, curColor.g, curColor.b); // Send the latest RGB colors to the pixels
    }
}

/**
 * @brief Code that runs when the board first powers on
 * 
 */
void setup(){
    // Turn on the timer register
    OCR0A = 0x00;          // any number is OK (Configuration for ATtiny85)
    TIMSK |= _BV(TOIE0);   // (Configuration for ATtiny85) 

    initPrimaryButton(); // Initialize the switch

    initPixels(); // Initialize the pixels

    closeEyeFx(); // Start by having the eye closed and the pixels off
}

/**
 * @brief Code that runs continuously on the board
 * 
 */
void loop(){
    primaryButton.tick(); // Monitors for button presses

    monitorPixels(); // Monitors pixels state
}

// We'll take advantage of the built in millis() timer that goes off
// to keep track of time, and refresh the servo every 20 milliseconds
volatile uint8_t counter = 0;
ISR(TIMER0_OVF_vect) {
  // this gets called every 2 milliseconds
  counter += 2;
  // every 20 milliseconds, refresh the servos!
  if (counter >= 20) {
    counter = 0;
    servo.refresh();
  }
}