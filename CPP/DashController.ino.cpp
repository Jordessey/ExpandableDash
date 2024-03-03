#include <Arduino.h>
#line 1 "C:\\Users\\Jorde\\Documents\\Arduino\\DashController\\DashController.ino"
#include <PCF8574.h>
#include <Wire.h>
#include <PicoGamepad.h>
#define ENCEngagePeriod 80

MbedI2C Wire1(6u, 7u);

//Encoder Setup
const int ENCCount = 5;
int counter[ENCCount];
int counterref[ENCCount];
int currentStateCLK[ENCCount];
int previousStateCLK[ENCCount];
int ENC_CLK[5] = { 16u, 18u, 20u, 22u, 24u };
int ENC_DT[5] = { 17u, 19u, 21u, 23u, 25u };
int ENC_INC[5] = { 25, 27, 29, 31, 33 };
int ENC_DeINC[5] = { 24, 26, 28, 30, 32 };
bool EncoderUP[5];


//RP2040 Analog Inputs
int ARead[4] = { 0, 0, 0, 0 };         //Analog Value Array
int Apin[4] = { 26u, 27u, 28u, 29u };  //Analog Pin Array



PCF8574 pcf0(0x20);
PCF8574 pcf1(0x21);
PCF8574 pcf2(0x22);
PicoGamepad gamepad;

bool PCF0Int = false;
bool PCF1Int = false;
bool PCF2Int = false;
bool ExpanderPresent;
bool SetUnusedSliders = false;




#line 41 "C:\\Users\\Jorde\\Documents\\Arduino\\DashController\\DashController.ino"
void setup();
#line 66 "C:\\Users\\Jorde\\Documents\\Arduino\\DashController\\DashController.ino"
void loop();
#line 120 "C:\\Users\\Jorde\\Documents\\Arduino\\DashController\\DashController.ino"
void PCF0();
#line 123 "C:\\Users\\Jorde\\Documents\\Arduino\\DashController\\DashController.ino"
void PCF1();
#line 126 "C:\\Users\\Jorde\\Documents\\Arduino\\DashController\\DashController.ino"
void PCF2();
#line 131 "C:\\Users\\Jorde\\Documents\\Arduino\\DashController\\DashController.ino"
void EncoderCheck(uint32_t ENCNum);
#line 148 "C:\\Users\\Jorde\\Documents\\Arduino\\DashController\\DashController.ino"
void EncoderUpdate(uint32_t ENCNum);
#line 41 "C:\\Users\\Jorde\\Documents\\Arduino\\DashController\\DashController.ino"
void setup() {
  for (int i = 0; i < ENCCount; i++) {
    previousStateCLK[ENCCount] = digitalRead(ENC_CLK[ENCCount]);
  }
  Serial.begin(9600);
  Wire.begin();
  Wire.setClock(400000);
  Wire1.begin();
  Wire1.setClock(400000);

  Wire1.beginTransmission(0x4D);
  if (Wire1.endTransmission() != 0) {
    Serial.print("Expansion Board Not Present");
    gamepad.SetX(0);
    gamepad.SetY(0);
    gamepad.SetRx(-32767);
    gamepad.SetRy(-32767);
  }

  analogReadResolution(12);
  attachInterrupt(13u, PCF0, FALLING);
  attachInterrupt(14u, PCF1, FALLING);
  attachInterrupt(15u, PCF2, FALLING);
}

void loop() {
  if (PCF0Int == true) {
    for (int i = 0; i < 8; i++) {
      gamepad.SetButton(i, !pcf0.read(i));
    }
  }
  if (PCF1Int == true) {
    for (int i = 0; i < 8; i++) {
      int j = i + 8;
      gamepad.SetButton(j, !pcf1.read(i));
    }
  }
  if (PCF2Int == true) {
    for (int i = 0; i < 8; i++) {
      int j = i + 16;
      if ((pcf2.read(i) == LOW) && (pcf1.read(7) == LOW)) {
        gamepad.SetButton(15, LOW);
        gamepad.SetButton(j, !pcf2.read(i));
      } else {
        gamepad.SetButton(j, !pcf2.read(i));
      }
    }
  }
  for (int i = 0; i < 4; i++) {
    ARead[i] = analogRead(Apin[i]);
    ARead[i] = map(ARead[i], 0, 4096, -32767, 32767);
  }
  static uint32_t task1Counter = 0;
  static uint32_t lastTime = millis();

  if (millis() - lastTime) {
    lastTime = millis();
    ++task1Counter;
    if (task1Counter == ENCEngagePeriod) {
      for (int i = 0; i < ENCCount; i++) {
        EncoderUpdate(i);
      }

      task1Counter = 0;
    }
  }
  for (int i = 0; i < ENCCount; i++) {
    EncoderCheck(i);
  }

  gamepad.SetRz(ARead[0]);
  gamepad.SetS0(ARead[1]);
  gamepad.SetThrottle(ARead[2]);
  gamepad.SetZ(ARead[3]);
  gamepad.send_update();
  sleep_us(500);
}


void PCF0() {
  PCF0Int = true;
}
void PCF1() {
  PCF1Int = true;
}
void PCF2() {
  PCF2Int = true;
}


void EncoderCheck(uint32_t ENCNum) {
  currentStateCLK[ENCNum] = digitalRead(ENC_CLK[ENCNum]);
  if (currentStateCLK[ENCNum] != previousStateCLK[ENCNum]) {

    // If the inputDT state is different than the inputCLK state then
    // the encoder is rotating counterclockwise
    if (digitalRead(ENC_DT[ENCNum]) != currentStateCLK[ENCNum]) {
      counter[ENCNum]--;
    } else {
      // Encoder is rotating clockwise
      counter[ENCNum]++;
    }
  }
  // Update previousStateCLK with the current state
  previousStateCLK[ENCNum] = currentStateCLK[ENCNum];
}

void EncoderUpdate(uint32_t ENCNum) {
  if (EncoderUP[ENCNum] == false) {
    if (counterref[ENCNum] > counter[ENCNum]) {
      gamepad.SetButton(ENC_DeINC[ENCNum], HIGH);
      EncoderUP[ENCNum] = true;
    } else if (counterref[ENCNum] < counter[ENCNum]) {
      gamepad.SetButton(ENC_INC[ENCNum], HIGH);
      EncoderUP[ENCNum] = true;
    }
  } else {
    gamepad.SetButton(ENC_DeINC[ENCNum], LOW);
    gamepad.SetButton(ENC_INC[ENCNum], LOW);
    EncoderUP[ENCNum] = false;
  }
  counterref[ENCNum] = counter[ENCNum];
}

