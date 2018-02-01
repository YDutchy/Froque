#include "FastLED.h"
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    76
#define NUM_RINGS   15
#define NUM_WIND    8
#define BRIGHTNESS  50
#define FRAMES_PER_SECOND 30

CRGB leds[NUM_RINGS + 2][NUM_LEDS]; // +2 so we can precalculate things

const int mapping[15] = {12, 11, 0, 13, 10, 8, 7 , 14, 9, 1, 5, 6, 2, 3, 4};
const int animationMap[15] = {10, 8, 14, 12, 5, 6, 2, 3, 7,  11, 0, 13, 9, 1, 4};
#define COLOR_ORDER GRB

struct LEDring {
  boolean Activated = false;
  int State = 0;
  unsigned int Animation0State = 0;
  int Animation0Cycle = 0;
  bool Animation0Switch = true;
  long StartTime = 0;
  long StateTime;
  long LastSeen;
  int count = 0;
  int check = 1;
} LEDRings[NUM_RINGS];

struct windInsert {
  long startTime = 0;
  long interval = 0;
  bool state = false;
} windInserts[NUM_WIND];

struct GlobalVar {
  int brightness = 100; // just a starting value;
  int brightnessRGB = 0;
  int cycleIncrement = 1; // adjustable value
  int fadeState = 1; // just a starting value between 1 and 4
  int cycle = 0;
  int cycleSwitch = 1;
  int fadeDirections = 1;
  int ringSize[4] = {12, 24, 36, 24}; // added ringsize 4 for fadingcycle ease
  int TimeOut = 1000; // used for downscaling;
  int ActivatedSize = 0;
  int color[3] = {100, 150, 200};
  int maxState;
  long timeSinceWave;
  int windMap[8] = {2, 4, 3, 5, 10, 12, 11, 14};
  int windPins[8] = {6, 8, 10, 12, 13, 11, 9, 7};
  int windCorrection[15] = {12, 11, 0, 13, 10, 8, 7, 14, 9, 1, 5, 6, 2, 3, 4};
  int connext[4] = {0};

} GlobalVars;

void setup() {
  Serial.begin(9600);

  //Declare input and output pins for active states

  //Declare Statepins
  for (int i = A0; i <= A15; i++) {
    pinMode (i, INPUT_PULLUP);
  }

  // Declare input pins Wemos
  for (int i = 2; i < 6; i++)
    pinMode (i, INPUT);

  // Declare outputs for fan relais
  for (int i = 6; i <= 13; i++) {
    pinMode (i, OUTPUT);
  }

  // Add leds to FastLED
  FastLED.addLeds<LED_TYPE, 22, COLOR_ORDER>(leds[0],  NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 24, COLOR_ORDER>(leds[1],  NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 26, COLOR_ORDER>(leds[2],  NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 28, COLOR_ORDER>(leds[3],  NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 30, COLOR_ORDER>(leds[4],  NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 32, COLOR_ORDER>(leds[5],  NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 34, COLOR_ORDER>(leds[6],  NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 36, COLOR_ORDER>(leds[7],  NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 38, COLOR_ORDER>(leds[8],  NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 40, COLOR_ORDER>(leds[9],  NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 42, COLOR_ORDER>(leds[10], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 44, COLOR_ORDER>(leds[11], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 46, COLOR_ORDER>(leds[12], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 48, COLOR_ORDER>(leds[13], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 50, COLOR_ORDER>(leds[14], NUM_LEDS).setCorrection(TypicalLEDStrip);


  // Light up all rings for a short time to show setup is done and then clear them.
  for (int j = 0; j < 150; j++) {
    for (int i = 0; i < NUM_RINGS; i++) {

      LEDRings[i].Animation0State = random(0, 400);

      fill_rainbow( leds[i], 12, j + 15, 15 );
      fill_rainbow( leds[i] + 12, 24, j + 7, 9 );
      fill_rainbow( leds[i] + 36, 40, j, 6 );
      if (j < 75) {
        fadeLightBy(leds[i], 76, map(j, 0, 75, 255, 0));
      }
      else {
        fadeLightBy(leds[i], 76, map(j, 75, 150, 0, 255));
      }
    }
    FastLED.show();
  }

  FastLED.clear();

}

void loop() {

  UpdateStates();
  UpdateMQTT();
  //CycleVariables();
  ShowAnimation();
  RegulateFans();
  LoopProtection();

}

void UpdateStates() {

  GlobalVars.maxState = 0;
  GlobalVars.ActivatedSize = 0;

  for (int i = 0; i < 15; i++) {

    if (LEDRings[i].State == 0) {
      LEDRings[i].StateTime = 0;
      LEDRings[i].LastSeen = 0;
    }
    LEDRings[i].count = 0;
    for (int j = 0; j < 3; j++) {
      if (digitalRead(A0 + i) == 0) {
        LEDRings[i].count++;
      }
    }

    if (LEDRings[i].count >= 1) {
      LEDRings[i].LastSeen = millis();

      if (LEDRings[i].State == 0) {
        LEDRings[i].StartTime = millis();   // ToDo remove startTime for state time
        LEDRings[i].StateTime = millis();
        LEDRings[i].State = 1;
      }

      if (LEDRings[i].State == 1 && millis() - LEDRings[i].StateTime >= 3500 ) {
        LEDRings[i].State = 2;
        LEDRings[i].StateTime = millis();
        LEDRings[i].LastSeen = millis(); // beundingetje
      }
    }


    if (LEDRings[i].State > 0 && millis() - LEDRings[i].LastSeen >= GlobalVars.TimeOut) {
      LEDRings[i].State--;
      LEDRings[i].LastSeen = millis();
      LEDRings[i].StateTime = millis();
    }

    if (LEDRings[i].State != 0) {
      //GlobalVars.ActivatedSize++;
    }

    if (LEDRings[i].State == 0) {
      GlobalVars.ActivatedSize--;
      LEDRings[i].StartTime = 0;
    }


    if (LEDRings[i].State == 2) {
      GlobalVars.maxState = 1;

    }
  }
}

void UpdateMQTT() {

  int dataIn[4];

  for ( int i = 0; i < 4; i++) {
    dataIn[i] = digitalRead(i + 2);
  }

  if (dataIn[0] == 0) {
    GlobalVars.connext[0] = 0;
  } else if (GlobalVars.connext[0] == 0) {
    GlobalVars.connext[0] = 1;
    switch (4 * dataIn[3] + 2 * dataIn[2] + dataIn[1]) {
      case 0: // ID:6
        /*
          GlobalVars.connext[1] = 100;
          GlobalVars.connext[2] = 100;
          GlobalVars.connext[3] = 255;
          break;*/
        WaveOverWall(255, 255, 0);
        WaveOverWall(255, 200, 50);
        WaveOverWall(255, 100, 150);
        WaveOverWall(255, 0, 255);
        WaveOverWall(200, 50, 255);
        WaveOverWall(100, 150, 255);
        WaveOverWall(0, 255, 255);
        WaveOverWall(50, 255, 200);
        WaveOverWall(150, 255, 100);
        WaveOverWall(255, 255, 0);
        return;
      case 1: // ID:8
        /*
          GlobalVars.connext[1] = 200;
          GlobalVars.connext[2] = 255;
          GlobalVars.connext[3] = 0;
          break;*/
        for (int j = 0; j < 150; j++) {
          for (int i = 0; i < NUM_RINGS; i++) {

            LEDRings[i].Animation0State = random(0, 400);

            fill_rainbow( leds[i], 12, j + 15, 15 );
            fill_rainbow( leds[i] + 12, 24, j + 7, 9 );
            fill_rainbow( leds[i] + 36, 40, j, 6 );
            if (j < 75) {
              fadeLightBy(leds[i], 76, map(j, 0, 75, 255, 0));
            }
            else {
              fadeLightBy(leds[i], 76, map(j, 75, 150, 0, 255));
            }
          }
          FastLED.show();
        }
        return;
      case 2: // ID:14
        /*
          GlobalVars.connext[1] = 0;
          GlobalVars.connext[2] = 255;
          GlobalVars.connext[3] = 0;
          break;*/
        FastLED.clear();
        curtain(128, 0, 0);
        curtain(0, 0, 0);
        return;
      case 3: // ID:22
        GlobalVars.connext[1] = 0;
        GlobalVars.connext[2] = 0;
        GlobalVars.connext[3] = 255;
        break;
      case 4: // ID:49
        GlobalVars.connext[1] = 255;
        GlobalVars.connext[2] = 0;
        GlobalVars.connext[3] = 255;
        break;
      case 5: // ID:66

        GlobalVars.connext[1] = 255;
        GlobalVars.connext[2] = 127;
        GlobalVars.connext[3] = 0;
        break;

      case 6: // ID:75

        GlobalVars.connext[1] = 0;
        GlobalVars.connext[2] = 255;
        GlobalVars.connext[3] = 127;
        break;


      case 7: // ID:90

        GlobalVars.connext[1] = 0;
        GlobalVars.connext[2] = 255;
        GlobalVars.connext[3] = 255;
        break;
    }
    WaveOverWall(GlobalVars.connext[1], GlobalVars.connext[2], GlobalVars.connext[3]);
    GlobalVars.timeSinceWave = millis();
  }
}

void CycleVariables() {

  if (GlobalVars.fadeDirections == 1) {
    GlobalVars.brightnessRGB += GlobalVars.cycleIncrement;
    if (GlobalVars.brightnessRGB >= 25) {
      GlobalVars.brightnessRGB = 25;
      GlobalVars.fadeDirections = 0;
    }
  } else {
    GlobalVars.brightnessRGB -= GlobalVars.cycleIncrement;
    if (GlobalVars.brightnessRGB <= 0) {
      GlobalVars.brightnessRGB = 0;
      GlobalVars.fadeDirections = 1;

      GlobalVars.fadeState++;

      if (GlobalVars.fadeState == 5) {
        GlobalVars.fadeState = 1;
        if (GlobalVars.cycleSwitch == 1) {
          GlobalVars.cycle++;
          if (GlobalVars.cycle >= 3) {
            GlobalVars.cycleSwitch = 0;
          }
        } else {
          GlobalVars.cycle--;
          if (GlobalVars.cycle <= 0) {
            GlobalVars.cycleSwitch = 1;
          }
        }
      }
    }
  }


  /* if (GlobalVars.brightness >= 100 || GlobalVars.brightness < 10) {
     GlobalVars.fadeDirections = - GlobalVars.fadeDirections;
    }


    GlobalVars.brightness = GlobalVars.brightness + GlobalVars.fadeDirections * GlobalVars.cycleIncrement;
    if (GlobalVars.brightness < 10) {
     GlobalVars.fadeState++;

     if (GlobalVars.fadeState == 5) {
       GlobalVars.fadeState = 1;
       GlobalVars.cycle++;
       if (GlobalVars.cycle == 3) {
         GlobalVars.cycle = 0;
       }

       for (int i = 0; i < 3; i++) { // only update colors once per cycle
         GlobalVars.color[i] = GlobalVars.color[i] + random(0, 50);
         if (GlobalVars.color[i] > 255) {
           GlobalVars.color[i] = 0;
         }
       }

     }*/
}


void ShowAnimation() {


  if (GlobalVars.ActivatedSize > 7 && GlobalVars.ActivatedSize < 15 && millis() - GlobalVars.timeSinceWave >= 60000) {
    WaveOverWall(0, 0, 0);
    GlobalVars.timeSinceWave = millis();
    return;
  }

  if (GlobalVars.ActivatedSize == 15) {
    SystemWideAnimation();
  }

  if (GlobalVars.maxState > 0) {
    PreCalculateRainbow();
  }

  //  if (GlobalVars.ActivatedSize >= 5) {
  //
  //  SystemWideAnimation();
  //  }
  //  else {

  // for( int i = 0; i<15; i++){
  //   Serial.print(LEDRings[i].State);
  //   Serial.print("  ");
  // }
  // Serial.println();

  FastLED.clear();
  for (int i = 0; i < 15 ; i++) {

    switch (LEDRings[i].State) {
      case 0:
        animation0(i);
        break;
      case 1:
        animation1(i);
        break;
      case 2:
        animation2(i);
        break;


    }
  }

  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);

}

void animation0(int LEDindex) {

  int R , G , B;

  if (GlobalVars.connext[0] == 0) {
    switch (LEDRings[LEDindex].Animation0Cycle) {
      case 0:
        R = 33;
        G = 64;
        B = 154;
        break;
      case 1:
        R = 0;
        G = 118;
        B = 129;
        break;
      case 2:
        R = 0;
        G = 166;
        B = 81;
        break;
      case 3:
        R = 40;
        G = 170;
        B = 30;
        break;
      default:
        R = 255;
        G = 0;
        B = 0;
        break;
    }
  } else {
    R = GlobalVars.connext[1];
    G = GlobalVars.connext[2];
    B = GlobalVars.connext[3];
  }

  if (LEDRings[LEDindex].Animation0State >= 0 && LEDRings[LEDindex].Animation0State < 50) {
    R = map(LEDRings[LEDindex].Animation0State, 0, 49, 0, R);
    G = map(LEDRings[LEDindex].Animation0State, 0, 49, 0, G);
    B = map(LEDRings[LEDindex].Animation0State, 0, 49, 0, B);
    for (int i = 0; i < 12; i++) {
      leds[LEDindex][i].setRGB(R, G, B);
    }
  } else if (LEDRings[LEDindex].Animation0State >= 50 && LEDRings[LEDindex].Animation0State < 100) {
    R = map(LEDRings[LEDindex].Animation0State, 50, 99, R, 0);
    G = map(LEDRings[LEDindex].Animation0State, 50, 99, G, 0);
    B = map(LEDRings[LEDindex].Animation0State, 50, 99, B, 0);
    for (int i = 0; i < 12; i++) {
      leds[LEDindex][i].setRGB(R, G, B);
    }
  } else if (LEDRings[LEDindex].Animation0State >= 100 && LEDRings[LEDindex].Animation0State < 150) {
    R = map(LEDRings[LEDindex].Animation0State, 100, 149, 0, R);
    G = map(LEDRings[LEDindex].Animation0State, 100, 149, 0, G);
    B = map(LEDRings[LEDindex].Animation0State, 100, 149, 0, B);
    for (int i = 12; i < 36; i++) {
      leds[LEDindex][i].setRGB(R, G, B);
    }
  } else if (LEDRings[LEDindex].Animation0State >= 150 && LEDRings[LEDindex].Animation0State < 200) {
    R = map(LEDRings[LEDindex].Animation0State, 150, 199, R, 0);
    G = map(LEDRings[LEDindex].Animation0State, 150, 199, G, 0);
    B = map(LEDRings[LEDindex].Animation0State, 150, 199, B, 0);
    for (int i = 12; i < 36; i++) {
      leds[LEDindex][i].setRGB(R, G, B);
    }
  } else if (LEDRings[LEDindex].Animation0State >= 200 && LEDRings[LEDindex].Animation0State < 250) {
    R = map(LEDRings[LEDindex].Animation0State, 200, 249, 0, R);
    G = map(LEDRings[LEDindex].Animation0State, 200, 249, 0, G);
    B = map(LEDRings[LEDindex].Animation0State, 200, 249, 0, B);
    for (int i = 36; i < 76; i++) {
      leds[LEDindex][i].setRGB(R, G, B);
    }
  } else if (LEDRings[LEDindex].Animation0State >= 250 && LEDRings[LEDindex].Animation0State < 300) {
    R = map(LEDRings[LEDindex].Animation0State, 250, 299, R, 0);
    G = map(LEDRings[LEDindex].Animation0State, 250, 299, G, 0);
    B = map(LEDRings[LEDindex].Animation0State, 250, 299, B, 0);
    for (int i = 36; i < 76; i++) {
      leds[LEDindex][i].setRGB(R, G, B);
    }
  } else if (LEDRings[LEDindex].Animation0State >= 300 && LEDRings[LEDindex].Animation0State < 350) {
    R = map(LEDRings[LEDindex].Animation0State, 300, 349, 0, R);
    G = map(LEDRings[LEDindex].Animation0State, 300, 349, 0, G);
    B = map(LEDRings[LEDindex].Animation0State, 300, 349, 0, B);
    for (int i = 12; i < 36; i++) {
      leds[LEDindex][i].setRGB(R, G, B);
    }
  } else if (LEDRings[LEDindex].Animation0State >= 350 && LEDRings[LEDindex].Animation0State < 400) {
    R = map(LEDRings[LEDindex].Animation0State, 350, 399, R, 0);
    G = map(LEDRings[LEDindex].Animation0State, 350, 399, G, 0);
    B = map(LEDRings[LEDindex].Animation0State, 350, 399, B, 0);
    for (int i = 12; i < 36; i++) {
      leds[LEDindex][i].setRGB(R, G, B);
    }
  }
  LEDRings[LEDindex].Animation0State++;
  if (LEDRings[LEDindex].Animation0State >= 400) {
    LEDRings[LEDindex].Animation0State = 0;
    if (LEDRings[LEDindex].Animation0Switch == true) {
      LEDRings[LEDindex].Animation0Cycle++;
      if (LEDRings[LEDindex].Animation0Cycle >= 3) {
        LEDRings[LEDindex].Animation0Switch = false;
      }
    } else {
      LEDRings[LEDindex].Animation0Cycle--;
      if (LEDRings[LEDindex].Animation0Cycle <= 0) {
        LEDRings[LEDindex].Animation0Switch = true;
      }
    }

  }

}

bool gReverseDirection = false;
#define COOLING 50
#define SPARKING 140

void animation1(int LEDindex) {

  fill_gradient( leds[LEDindex], 0, CHSV(120, 255, 255), 11, CHSV(200, 255, 255), SHORTEST_HUES);
  fill_gradient( leds[LEDindex], 12, CHSV(120, 255, 255), 35, CHSV(200, 255, 255), SHORTEST_HUES);
  fill_gradient( leds[LEDindex], 36, CHSV(120, 255, 255), 75, CHSV(200, 255, 255), SHORTEST_HUES);
}

void animation2(int LEDindex) {
  //fill_rainbow( leds[LEDindex], 12, GlobalVars.brightness + 15, 15 );
  //fill_rainbow( leds[LEDi ndex] + 12, 24, GlobalVars.brightness + 7, 9 );
  //fill_rainbow( leds[LEDindex] + 36, 40, GlobalVars.brightness, 6 );
  for (int i = 0; i < 76; i++) {
    leds[LEDindex][i] = leds[16][i];

  }
}

void RegulateFans() {

  int index;

  for (int i = 0; i < NUM_WIND; i++) {

    index = GlobalVars.windCorrection[GlobalVars.windMap[i]];

    switch (LEDRings[index].State) {
      case 1:
        windControl(i, 1000, 0.25);
        break;
      case 2:
        windControl(i, 1000, 1);
        break;
      default:
        windControl(i, 1000, 0);
        break;
    }
  }

}

void windControl(int index, long period, float duty) {
  long currentMillis = millis();
  if (currentMillis - windInserts[index].startTime >= windInserts[index].interval) {
    windInserts[index].startTime = currentMillis;
    if (windInserts[index].state == false && duty != 0) {
      windInserts[index].state = true;
      windInserts[index].interval = period * duty;
    } else if (windInserts[index].state == true && duty != 1) {
      windInserts[index].state = false;
      windInserts[index].interval = period * (1 - duty);
    }
    digitalWrite(GlobalVars.windPins[index], windInserts[index].state);
  }
}

void SystemWideAnimation() {

  // random colored speckles that blink in and fade smoothly
  for (int i = 0; i < 15; i++) {
    fill_solid(leds[animationMap[i]], 76, CRGB(random(0, 255), random(0, 255), random(0, 255)));

  }
}

void LoopProtection() {

  for (int i = 0; i < 15 ; i++) {                       // ToDo remove startTime for satate time
    if (millis() - LEDRings[i].StartTime > 30000) {
      LEDRings[i].State = 0;
      LEDRings[i].StartTime = 0;
      LEDRings[i].LastSeen = 0;
    }
  }
}

void PreCalculateRainbow() {

  fill_rainbow( leds[16], 12, GlobalVars.brightness + 15, 15 );
  fill_rainbow( leds[16] + 12, 24, GlobalVars.brightness + 7, 9 );
  fill_rainbow( leds[16] + 36, 40, GlobalVars.brightness, 6 );

  GlobalVars.brightness += 5;
  if ( GlobalVars.brightness > 255) {
    GlobalVars.brightness -= 255;
  }

}

void WaveOverWall(int R, int G, int B) {

  if (R == 0 && G == 0 && B == 0) {
    R = GlobalVars.color[1];
    G = GlobalVars.color[2];
    B = GlobalVars.color[3];
  }


  //FastLED.clear();
  FastLED.show();

  for (int s = 0; s < 7; s++) {
    switch (s) {
      case 0:
        for (int i = 0; i <= 12; i++) {
          WaveSingle(mapping[0], i, R, G, B, 0);
          FastLED.delay(20);
        }
        FastLED.delay(30);
        break;
      case 1:
        for (int i = 0; i <= 12; i++) {
          WaveSingle(mapping[1], i, R, G, B, 0);
          WaveSingle(mapping[2], i, R, G, B, 0);
          FastLED.delay(20);
        }
        FastLED.delay(30);
        break;
      case 2:
        for (int i = 0; i <= 12; i++) {
          WaveSingle(mapping[3], i, R, G, B, 0);
          WaveSingle(mapping[4], i, R, G, B, 0);
          FastLED.delay(20);
        }
        FastLED.delay(30);
        break;
      case 3:
        for (int i = 0; i <= 12; i++) {
          WaveSingle(mapping[5], i, R, G, B, 0);
          WaveSingle(mapping[6], i, R, G, B, 0);
          FastLED.delay(20);
        }
        FastLED.delay(30);
        break;
      case 4:
        for (int i = 0; i <= 12; i++) {
          WaveSingle(mapping[7], i, R, G, B, 0);
          WaveSingle(mapping[8], i, R, G, B, 0);
          FastLED.delay(20);
        }
        FastLED.delay(30);
        break;
      case 5:
        for (int i = 0; i <= 12; i++) {
          WaveSingle(mapping[9], i, R, G, B, 0);
          WaveSingle(mapping[10], i, R, G, B, 0);
          FastLED.delay(20);
        }
        FastLED.delay(30);
        break;
      case 6:
        for (int i = 0; i <= 12; i++) {
          WaveSingle(mapping[11], i, R, G, B, 0);
          WaveSingle(mapping[12], i, R, G, B, 0);
          WaveSingle(mapping[13], i, R, G, B, 0);
          WaveSingle(mapping[14], i, R, G, B, 0);
          FastLED.delay(20);
        }
        //FastLED.delay(100);
        break;
    }
  }

}

void WaveSingle(int index, int count, int R, int G, int B, int direc) {

  int k = 0;
  switch (count) {
    case 1:
    case 2:
      k = 2 * count - 1;
      break;
    case 4:
    case 5:
    case 6:
      k = 2 * count - 3;
      break;
    case 7:
    case 8:
      k = 2 * count - 4;
      break;
    case 10:
    case 11:
    case 12:
      k = 2 * count - 6;
      break;

  }

  if (direc == 0) {
    leds[index][count / 2].setRGB(R, G, B);
    leds[index][12 - count / 2].setRGB(R, G, B);
    leds[index][12 + count].setRGB(R, G, B);
    leds[index][36 - count].setRGB(R, G, B);
    switch (count) {
      default:
        leds[index][36 + k].setRGB(R, G, B);
        leds[index][76 - k].setRGB(R, G, B);
        k++;
      case 6:
      case 12:
        leds[index][36 + k].setRGB(R, G, B);
        leds[index][76 - k].setRGB(R, G, B);
        k++;
        break;
      case 0:
        leds[index][36].setRGB(R, G, B);
        k++;
      case 3:
      case 9:
        break;
    }
  } else if (direc == 1) {
    leds[index][6 + count / 2].setRGB(R, G, B);
    leds[index][6 - count / 2].setRGB(R, G, B);
    leds[index][24 + count].setRGB(R, G, B);
    leds[index][24 - count].setRGB(R, G, B);
    switch (count) {
      default:
        leds[index][54 + k].setRGB(R, G, B);
        leds[index][54 - k].setRGB(R, G, B);
        k++;
      case 6:
      case 12:
        leds[index][54 + k].setRGB(R, G, B);
        leds[index][54 - k].setRGB(R, G, B);
        k++;
        leds[index][54 + k].setRGB(R, G, B);
        leds[index][54 - k].setRGB(R, G, B);
        k++;
        leds[index][54 + k].setRGB(R, G, B);
        break;
      case 0:
        leds[index][54].setRGB(R, G, B);
        k++;
      case 3:
      case 9:
        break;
    }
  }
}

void curtain( int R, int G, int B) {



  FastLED.show();


  for (int s = 0; s < 7; s++) {
    switch (s) {
      case 0:
        for (int i = 0; i <= 12; i++) {
          WaveSingle(mapping[5], i, R, G, B, 1);
          WaveSingle(mapping[6], i, R, G, B, 0);
          FastLED.delay(20);
        }
        FastLED.delay(30);
        break;
      case 1:
        for (int i = 0; i <= 12; i++) {
          WaveSingle(mapping[7], i, R, G, B, 0);
          WaveSingle(mapping[8], i, R, G, B, 0);


          WaveSingle(mapping[3], i, R, G, B, 1);
          WaveSingle(mapping[4], i, R, G, B, 1);
          FastLED.delay(20);
        }
        FastLED.delay(30);
        break;
      case 2:
        for (int i = 0; i <= 12; i++) {
          WaveSingle(mapping[9], i, R, G, B, 0);
          WaveSingle(mapping[10], i, R, G, B, 0);

          WaveSingle(mapping[1], i, R, G, B, 1);
          WaveSingle(mapping[2], i, R, G, B, 1);
          FastLED.delay(20);
        }
        FastLED.delay(30);
        break;
      case 3:
        for (int i = 0; i <= 12; i++) {
          WaveSingle(mapping[11], i, R, G, B, 0);
          WaveSingle(mapping[12], i, R, G, B, 0);
          WaveSingle(mapping[13], i, R, G, B, 0);
          WaveSingle(mapping[14], i, R, G, B, 0);

          WaveSingle(mapping[0], i, R, G, B, 1);
          FastLED.delay(20);
        }
        FastLED.delay(100);

        break;
    }
  }


}
