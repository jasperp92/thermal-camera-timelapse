/***************************************************************************
  This is a sketch to build a timelapse camera trigger with the amg8833.
  When a person is out of range of the IR camera, it triggers the DSLR.
  Jasper Precht, 2022
 ***************************************************************************/

#include <Wire.h>
#include <Adafruit_AMG88xx.h>

Adafruit_AMG88xx amg;

int LED = 11; //digital pin of the LED indicating a motion
int CAMERA = 10; //digital pin of the camera trigger
float pixels[AMG88xx_PIXEL_ARRAY_SIZE];
float initialTemperature[AMG88xx_PIXEL_ARRAY_SIZE];
float maxTemp;
bool trigger = false;
unsigned long currentTime; // timer for shutter interval
unsigned long previousTime;
unsigned long currentTime2; // timer for outTime
unsigned long previousTime2;
unsigned long currentTime3; // timer for inTime
unsigned long previousTime3;
int inTime = 8000;
int outTime = 2000;
float threshold = 4; // threshold of temperature difference between enviroment and person
int shutterInterval = 20000; // minimum shutterinterval for timelapse

void setup() {
  pinMode(CAMERA, OUTPUT); //initialise Pin as camera trigger
  Serial.begin(9600);
  Serial.println(F("AMG88xx pixels"));

  bool status;

  // initialize AMG88
  status = amg.begin();
  if (!status) {
    Serial.println("Could not find a valid AMG88xx sensor, check wiring!");
    while (1);
  }

  delay(1000); // let sensor boot up

  amg.readPixels(pixels);   // read out amg pixels

  initialTemp(); // get the initial temperature

  previousTime = millis();
  previousTime2 = millis();
  previousTime3 = millis();

} //end of setup


void loop() {
  //read all the pixels and update
  amg.readPixels(pixels);

// update the current time of all timers
  currentTime = millis();
  currentTime2 = millis();
  currentTime3 = millis();

//check if a person is inside
  if (personInside()) {
    digitalWrite(LED, HIGH); //LED indicating person lights up
    previousTime2 = currentTime2; // reset outTime
    
    // if the intime interval is exceeded it activates the camera trigger 
    // to prevent shooting to many images without a progress
    if (currentTime3 - previousTime3 > inTime) {
    trigger = true;
    }
  } else {
   // if the outtime is exceeded, it resets the trigger for shooting a next image
    if (currentTime2 - previousTime2 > outTime) {
      // this is another if statement to check the minimal shutter interval
      if (trigger && currentTime - previousTime > shutterInterval) {
        digitalWrite(CAMERA, HIGH); //trigger camera
        delay(500);              
        digitalWrite(CAMERA, LOW);
        trigger = false;
        previousTime = currentTime; // reset shutter timer
      }
    }
    digitalWrite(LED, LOW);
    previousTime3 = currentTime3; // reset inTime
  }
}

// function to calculate the average temperature of pixels
float calcAverageTemp(int amount, int interval) {

  float cycles;
  for (int i = 0; i < amount; i++) {
    float averagePixelTemperature = 0;
    for (int j = 1; j <= AMG88xx_PIXEL_ARRAY_SIZE; j++) {
      averagePixelTemperature += pixels[j - 1];
    }
    cycles += averagePixelTemperature / AMG88xx_PIXEL_ARRAY_SIZE;
    delay(interval);
    Serial.println(cycles);
  }
  return cycles / amount;
}

// function to calculate highest measured temperature
float calcMaxTemp() {

  float maximum = 0;
  for (int j = 1; j <= AMG88xx_PIXEL_ARRAY_SIZE; j++) {
    if (pixels[j - 1] > maximum) {
      maximum = pixels[j - 1];
    }
  }
  return maximum;
}

// function to check the initial temperature at the start
void initialTemp() {
  for (int i = 1; i <= AMG88xx_PIXEL_ARRAY_SIZE; i++) {
    initialTemperature[i - 1] = pixels[i - 1];
    Serial.print(initialTemperature[i - 1]);
    Serial.print(", ");
    if ( i % 8 == 0 ) Serial.println();
  }
  Serial.println("]");
  Serial.println();
}

// function to check if a person is inside the range of the sensor
bool personInside() {
  int sum = 0;
  for (int i = 1; i <= AMG88xx_PIXEL_ARRAY_SIZE; i++) {
    //check where is top and left, to narrow the sensors FOV, otherwise divide /
    if ( 
       round(i % 8) != 0 
    || round(i % 8) != 7 
    || round(i % 8) != 1 
    || round(i % 8) != 6) { 
      if (pixels[i - 1] >= initialTemperature[i - 1] + threshold) {
        sum += 1;   // if one pixel is above threshold add 1
      }
    }
  }
 // if one pixel is over threshold a person is inside, thus return true
  if (sum > 0) {
    return true;
  } else {
    return false;
  }

}
