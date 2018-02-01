// ---------------------------------------------------------------------------
// Polysense Sensor Arduino Sketch.
// Using 15 HC-SR04 distance sensors to determine movement in front of the
// Polysense installation
// ---------------------------------------------------------------------------

#include <NewPing.h>

byte num_Sensors = 15; // Amount of used sensors in the installation

struct Sensor { // Variables which differ for each individual sensor and change throughout the loops.
  int echoTime; // Time between pulse and the echo
  int distance; // Distance of the object in front of the sensor, for easy usability
  long lastSeen = 0; // The internal clocktime at which the sensor registered something the last time
  long StateTime;
  int State = 0;
} Sensors[15];  //

long sendTimeout = 50;
long lastSend = 0 ;

int medianMeasuraments = 2; // Amount of measurements per sensor, to remove outliers.
int minDistance = 1;        // Minimum distance threshold for the sensor to react.
int maxDistance = 30;       // Maximum distance threshold for the sensor to react.
int pushTime = 500;
int sensorDelay = 0;

const int mapRows = 3;
const int maping[mapRows][15] = {
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,  11,  12,  13,  14},
  {  2,  2,  1,  4,  6,  7,  4,  5,  5, 10, -1,  12,  10,  14,  13},
  { -1, -1, -1, -1, -1,  8, -1,  8,  7, -1, -1,  13,  13,  -1,  11}
};
const int mapingPins[15] = {12, 11, 0, 13, 10, 8, 7 , 14, 9, 1, 5, 6, 2, 3, 4};

#define max_Distance 150     // Maximum distance the sensor can detect within the NewPing library.

NewPing sonar[15] {                    // Declaring the sensors.
  NewPing(A0,   A1,  max_Distance),    // syntax is:
  NewPing(A2,   A3,  max_Distance),    // (trigger pin, echo pin, and the maximum waiting distance)
  NewPing(A4,   A5,  max_Distance),
  NewPing(A6,   A7,  max_Distance),
  NewPing(A8,   A9,  max_Distance),
  NewPing(A10,  A11, max_Distance),
  NewPing(A12,  A13, max_Distance),
  NewPing(A14,  A15, max_Distance),
  NewPing(30,   31,  max_Distance),
  NewPing(32,   33,  max_Distance),
  NewPing(34,   35,  max_Distance),
  NewPing(36,   37,  max_Distance),
  NewPing(38,   39,  max_Distance),
  NewPing(40,   41,  max_Distance),
  NewPing(42,   43,  max_Distance)
};

void setup() {
  Serial.begin(19200);

  for (int i = 0; i < 15; i++) { // Declare the communication pins for each sensor.
    pinMode (i + 2, OUTPUT);       // To remove outliers on the other Arduino we have used the PIN_PULLUP function
    digitalWrite(i + 2, HIGH);     // This filters out some noise and will only give a signal when there is a connection with the ground
    Serial.println(Sensors[i].State);
    // Resulting in having these pins on high from the setup onwards.
  }
}

void loop() {
  if (Serial.available()) {
    char inByte = Serial.read();
    for (int j = 0; j < 15; j++) {
      Serial.println(Sensors[j].State);
    }
  }
  measure(); // The function which measures the distance for each sensor and sends a signal to the other Arduino when neccesary.

  if (millis() - lastSend > sendTimeout) {
    lastSend = millis();
    for (int i = 0; i < num_Sensors; i++) {
      digitalWrite(i + 2, HIGH);
    }

    for (int i = 0; i < num_Sensors; i++) {
      if (Sensors[i].State > 0) {
        for (int j = 0; j < mapRows; j++) {
          if (maping[j][i] == -1){
            break;
          }
          digitalWrite(mapingPins[maping[j][i]]+2, LOW);
        }        
      }
    }
  }
}

void measure() {

  for (int i = 0; i < num_Sensors; i++) {            // Loop through sensor 1 to 15.
    delay(sensorDelay);                             // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.

    Sensors[i].echoTime = sonar[i].ping_median(medianMeasuraments);   // Take the average echotime of three measurements per sensor.
    Sensors[i].distance = sonar[i].convert_cm(Sensors[i].echoTime);   // Convert the echotime to an usable distance.


    if (Sensors[i].distance >= minDistance && Sensors[i].distance <= maxDistance) { // Filter values outside of the predetermined range.

      if (Sensors[i].State == 1 && millis() - Sensors[i].StateTime >= 2000) {
        Sensors[i].StateTime = millis();
        Sensors[i].State = 2;
      }

      if (Sensors[i].State == 0 /*&& Sensors[i].StateTime != 0*/) {
        Sensors[i].StateTime = millis();
        Sensors[i].State = 1;
      }


      Sensors[i].lastSeen = millis();                                               // Record the internal clocktime a sensor got a valid echo.
    }

    if (millis() - Sensors[i].lastSeen >= 2000 && Sensors[i].lastSeen != 0 && Sensors[i].State != 0) {
      Sensors[i].State--;
      Sensors[i].StateTime = millis();
      Sensors[i].lastSeen = millis();
      if (Sensors[i].State <= 0) {
        Sensors[i].StateTime = 0;
      }
    }
  }
}
