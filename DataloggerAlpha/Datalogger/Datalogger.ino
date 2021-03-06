/**
   @Author: Yichen Xu
   @Date:   09-Feb-2017 19:02:46
   @Email:  ichi@u.northwestern.edu
   @Last modified by:   Yichen Xu
   @Last modified time: 09-Feb-2017 19:02:52
*/

#include "init.h"
#include "dataCollection.h"
#include "dataWriter.h"
#include "PID.h"






// ================= FUNCTION DECLARATIONS =================

IntervalTimer myTimer;

// PIN DELCARATIONS
int chipSelect = BUILTIN_SDCARD;

File dataFile;

int xInput = 21;
int yInput = 20;
int zInput = 19;


int buttonPin = 39; // input from button
int directionPin = 36; // controls direction
int speedPin = 35; // controls speed of motor
int torquePin = 38; // torque limit
int motorPin = 37; // turns on and off motor
int collectPin = 14; // led pin to see if its collecting data or not
int calibrationPin = 15; // turns on LED when calibrated
// int encoderPin = 16;
int pulsePin = 23; // taking data from encoder on motor
int builtInLED = 13;

// TODO fill these out
int directionReadPin = 20;
int encoderMarks = 24;
// ============VARIABLE DECLARATION============================

// PID Variables;
DataSet sensorData[16];
uint8_t dataCounter = 0;
DataSet prevData;
DataSet currentData;
float kp = .339;
float ki = 0;
float kd = .065;


// I forgot what I made mode do lol
int MODE = 0;

volatile int encoderCounter;
volatile int encoderTime;
volatile float motorSpeed[5];
volatile uint8_t motorSpeedIndex;

int waitTime = 3000;

// =======Accel VARIABLES=======================

// min/max values for each axis
// get from calibration sketch
int xMin = 498;
int xMax = 512;

int yMin = 497;
int yMax = 512;

int zMin = 498;
int zMax = 512;

// WHOOO VARIABLES!


float xBase, yBase, zBase, xConv, yConv, zConv;
float xScaled, yScaled, zScaled;

// Initialize analog pin values
int xVal = 0;
int yVal = 0;
int zVal = 0;


// ============BNO Declaration===============
Adafruit_BNO055 bno = Adafruit_BNO055(55);

// ===================FILENAME SHENANIGANS=============
char outputFile[100];
String outputString;

// is this stuff running?
bool running = false;

// void pause();

void setup() {
  delay(1000); // idk just leave this in

  // initialize the digital pins
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pulsePin, INPUT);
  pinMode(builtInLED, OUTPUT);
  pinMode(calibrationPin, OUTPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(collectPin, OUTPUT);
  pinMode(torquePin, OUTPUT);
  pinMode(directionReadPin, INPUT);


  // Open serial communications and wait for port to open:
#if DEBUG
  Serial.begin(9600);
  waitTime = 5000;
#endif
  // Turn LED on to indicate its on
  digitalWrite(builtInLED, HIGH);

  // Initialize the stuff
  initializeBNO();
  initializeAccel(); // Honestly not neccessary anymore
  initializeSD();

  dataFile = SD.open(outputFile, FILE_WRITE);
  // Interrupts for the pins to keep track of buttons and encoders
  attachInterrupt(buttonPin, pause, FALLING);
  attachInterrupt(pulsePin, encoderReader, RISING);


}


uint32_t launchTimestamp = 0;
bool isLaunched = false;
double accel_vector = 0;
double launchGyro;

void pause() {
  delay(50);
  if (!digitalRead(buttonPin)) {

    isLaunched = false;
    digitalWrite(calibrationPin, LOW);
    running = !running;
#if DEBUG
    Serial.println("pls");
#endif

    if (running) {
      digitalWrite(collectPin, HIGH);
      newFile();
      myTimer.begin(doTheLoop, 10 * 1000);
    }
    else {
      digitalWrite(collectPin, LOW);
      myTimer.end();
    }
  }
}

void encoderReader() {
  int tempTime = encoderTime;

  encoderTime = micros();
  tempTime = encoderTime - tempTime;
  motorSpeedIndex = motorSpeedIndex % 5;

  if (digitalRead(directionReadPin)) {
    motorSpeed[motorSpeedIndex] = 1000000 / ((float) tempTime * encoderMarks);
  }
  else {
    motorSpeed[motorSpeedIndex] = -1000000 / ((float) tempTime * encoderMarks);
  }

  motorSpeedIndex++;


}

int isStopped = 0;
void doTheLoop() {
  sensorData[dataCounter] = getData();
  dataCounter = (dataCounter + 1) % 16;


#if DEBUG
  Serial.println(powerG);


  if (!isLaunched) {
    launchTimestamp = sensorData[dataCounter].time;
  }
  isLaunched = true;
#endif


  if (sqrt(pow(sensorData[dataCounter].bAccel.x, 2) + pow(sensorData[dataCounter].bAccel.y, 2) + pow(sensorData[dataCounter].bAccel.z, 2)) > 30 && !isLaunched) {
    isLaunched = true;
    launchTimestamp = sensorData[dataCounter].time;

  }


  if (((sensorData[dataCounter].time - launchTimestamp ) > (waitTime)) && isLaunched) {
    outputMotor(0);
    dataFile.close();
    isStopped = 1;
    myTimer.end();
  }



  if (isLaunched) {
    digitalWrite(calibrationPin, HIGH);
    doTheThing(launchTimestamp);
  }

  sensorData[dataCounter].power = powerG;


}
void loop() {
#if DEBUG
  Serial.println("pls");
#endif





  // todo ONLY DO THIS SO OFTEN


  if (dataCounter == 0) {
    for (int i = 0; i < 16; i++) {
      writeData(sensorData);
    }
  }

  while (isStopped) {
    digitalWrite(calibrationPin, !digitalRead(calibrationPin));
    delay(500);
  }



}
