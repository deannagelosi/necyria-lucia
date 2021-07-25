// Necyria lucia (Box Show 2021) control code
// In collaboration with https://github.com/travisformayor/
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_MPR121.h> // Capacitive Sensor
#include <Adafruit_NeoPixel.h>

// == Start Capacitive Init == //
#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

Adafruit_MPR121 cap = Adafruit_MPR121();

// Track last pin touched to detect when 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;
// == End Capacitive Init == //

// == Start Stepper Motor Init == //
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 2);

int totalSteps = 0; // track steps per run to return to starting
// == End Stepper Motor Init == //

// == Neopixels Init == //
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define LED_PIN   6
#define LED_COUNT 4
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGBW + NEO_KHZ800);

int currR = 0; // current red rgbw numerical value
int currG = 0; // current green rgbw numerical value
int currB = 0; // current blue rgbw numerical value
int currW = 0; // current white rgbw numerical value
// == End Neopixels init == //

void setup() {
  Serial.begin(9600); // set up Serial
  delay(3000); // Pause setup for Serial init
  
  Serial.println("Butterfly Initializing ...");

  // Set up capacitive sensor
  if (!cap.begin(0x5A)) {
    Serial.println("Cap sensor not found, check wiring?");
    while (1);
  }

  // Setup motor
  AFMS.begin();  // create with the default frequency 1.6KHz
  myMotor->setSpeed(100); // Init speed. 0 (off) -> 255 (max speed)

  // Setup Neopixels
  strip.begin();  // Initialize NeoPixel strip object
  strip.show();  // Turn OFF all pixels ASAP
  
  strip.setBrightness(225); // Set BRIGHTNESS to about 1/5 (max = 255)
  strip.show();
}

void loop() {
  //  Check if box touched...
  currtouched = cap.touched(); // Get currently touched pads

  // if it *is* touched and *wasnt* touched before, log and wake butterfly
  if ((currtouched & _BV(1)) && !(lasttouched & _BV(1)) ) {
    Serial.println("Pad 1 touched");
    
    pokeButterfly();
  }

  lasttouched = currtouched; // reset state

  delay(2000); // Delay helps cap with false positives, is a "hold to activate" time
}

void pokeButterfly() {
  lightsOff();

  Serial.println("Stage 1 - Wake up");
  //  - Fade in white lights
  //  - Run at slow speed for a bit after fade in done
  fadeInWhite(100,5); // delay per fade, steps per fade
  runMotor(20, 50); // speed, steps (slow)
  
  Serial.println("Stage 2 - Freak out");
  //  - Cycle colors through preset list of colors (config timing and list)
  //     - Last color return it to white
  //  - Flutter wings, ie oscillate motor speed
  colorCycle();
  
  Serial.println("Stage 3 - Go to sleep");
  //  - Lights fade out, motor runs down slow
  //  - Return wings to inital starting rest position
  fadeOutWhite(100,5); // delay per fade, steps per fade
  goToStart(); // return wings to starting position

  lightsOff();
  myMotor->release(); // release motor so it can be manually repositioned if needed
  Serial.println("Butterfly sequence complete");
  delay(3000); // Delay when next activation can happen
}

void colorCycle() {
  // == Start Config
  //  colors[rows][columns]
  //  {r, g, b, w}
  int numColors = 4;
  int colors[numColors][4] = {{0, 255, 0, 0},  // Green
                              {0, 0, 255, 0},  // Blue
                              {0, 40, 130, 0}, // Indigo
                              {0, 0, 0, 255}}; // White

  // Level of gradient detail in each color transition. Low means more colors.
  int colorDetail[numColors] = {20, 10, 20, 10};

  // Pause time (ms) between the gradient changes in each color transition
  int colorDelay[numColors] = {25, 10, 75,30};

  // Number of motor steps during each pause time
  int motorSteps[numColors] = {60, 50 ,75, 25};
  // == End Config

  for(int i=0; i < numColors; i++) { // Note: Need to manually set the number of loops to the number of colors
    colorTransition(colors[i], colorDetail[i], colorDelay[i], motorSteps[i]);
  }
}

void colorTransition(int rgbw[], int detail, int wait, int steps) {
    while(rgbw[0] != currR || 
          rgbw[1] != currG || 
          rgbw[2] != currB || 
          rgbw[3] != currW) {

    // Next color change in the transition
    int nextR = nextColor(currR, rgbw[0], detail);
    int nextG = nextColor(currG, rgbw[1], detail);
    int nextB = nextColor(currB, rgbw[2], detail);
    int nextW = nextColor(currW, rgbw[3], detail);
  
    setAll(nextR, nextG, nextB , nextW); 
    delayStep(wait, steps); // Delay while stepping the motor
  }
}

int nextColor(int current, int target, int colorMove) {
  // Return color value, 0 - 255, moved +1 or -1 towards target value
  int colorDiff = current - target;
  int nextValue;

  if (colorDiff == 0 || abs(colorDiff) < colorMove) {
    // Target reached
    nextValue = target;
  }
  else if ( colorDiff > 0 ) {
    // Current value larger than target value. Step current down.
    nextValue = current - colorMove;
  }
  else if ( colorDiff < 0) {
    // Current value smaller than target. Step current up.
    nextValue = current + colorMove;
  }

  return nextValue;
}

void setAll(int r, int g, int b, int w){
  uint32_t color = strip.Color(g, r, b, w); // G & R are flipped
  
  for(int i=0; i<strip.numPixels(); i++) {
    // Loop each pixel
    strip.setPixelColor(i, color);
  }
  
  // Update strip
  strip.show();
  // Update current color state
  currR = r;
  currG = g;
  currB = b;
  currW = w;

}

void fadeInWhite(int fadeTime, int fadeSteps) {
  for(int c=0; c<=255; c+=10) { // Loop up to full white
    setAll(0, 0, 0, c);
    delayStep(fadeTime, fadeSteps); // Delay while stepping
  }
  setAll(0, 0, 0, 255);
}

void fadeOutWhite(int fadeTime, int fadeSteps) {
  for(int c=255; c>=0; c-=10) { // Loop down to no white
    setAll(0, 0, 0, c);
    delayStep(fadeTime, fadeSteps); // Delay while stepping
  }
  setAll(0, 0, 0, 0);
}

void goToStart() {
  // Return the stepper to the starting position
  // Note: 200 steps per rotation
  Serial.println("Total motor steps taken: "); Serial.println(totalSteps);
  int stepsToGo = 0;

  if ( totalSteps > 200 ) {
    int rotations = (int)(totalSteps / 200);
    int stepsOver = totalSteps - (rotations * 200);
    stepsToGo = 200 - stepsOver;
  }
  else {
    // 200 or less stepped, under 1 rotation
    stepsToGo = 200 - totalSteps;
  }
  Serial.println("Steps left to starting position: "); Serial.println(stepsToGo);
  runMotor(20, stepsToGo); // (speed, steps) - Slow finish

  // Done. Reset total steps
  totalSteps = 0;
}

void lightsOff() {
    strip.clear();
    strip.show();
}

void runMotor(int speed, int steps) {
  // Note: This blocks program execution till finished
  myMotor->setSpeed(speed); // Init speed. 0 (off) -> 255 (max speed)
  myMotor->step(steps, FORWARD, DOUBLE);

  totalSteps += steps; // Track steps
}

void delayStep(int totalDelay, int steps) {
  // Custom Delay: Step function is blocking, and delay blocks steps
  // Need a delay option that steps as well as waits
  // Divide requested delay by number of steps and round to nearest ms
  // (Note: If more steps than delay ms is requested, do 1 step per 1 ms till done)
  // Do one step for each divided ms amount

  // Note: 
  // After max speed delay will slow down, Motor cant do anywhere near 1 step/ms.
  // Request less steps to speed the time back up.
  
  int delayPer;

  if ( totalDelay > steps ) {
    delayPer = (int)(totalDelay / steps) + 1;
  }
  else {
    delayPer = 1;
  }

  for(int i=0; i<steps; i++) {
    // Make one step
    myMotor->onestep(FORWARD, DOUBLE);
    // Wait till next step
    delay(delayPer);
  }

  totalSteps += steps; // Track Steps
}
