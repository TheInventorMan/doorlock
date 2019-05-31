#include <Servo.h>
#include <Keypad.h>
#include <SPI.h>

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
const int unlockPos = 2150;
const int lockPos = 900;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {9, 8, 7, 6};
byte servoPin = 2;
byte buzzPin = 5;
byte overridePin = A4;
byte entrySignal = A5;
//available pins: 3
String inputBuffer = String();
String passcode = "4D096";

bool lastOPState = false;
int OPState = 0;
unsigned long lastTime = 0;

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
Servo unlocker;

void setup() {
  Serial.begin(9600);
  pinMode(buzzPin, OUTPUT);
  pinMode(overridePin, INPUT);
  pinMode(entrySignal, OUTPUT);
  delay(1000);
}

void loop() {
  if (inputBuffer.charAt(0) == '*' or inputBuffer.charAt(0) == '#') {
    clb();
  }

  char key = keypad.getKey();

  if (key == '*') {
    if (checkPasscode()) {
      doorUnlock();
      clb();
    } else {
      Serial.println(inputBuffer);
      Serial.println("incorrect passcode");
      inBeep();
      clb();
    }
  }

  if (key == '#') {
    clb();
    blip();
  }

  if (key) {
    Serial.println(key);
    inputBuffer += key;
    blip();
  }

  OPState = digitalRead(overridePin);
  if (OPState != lastOPState) {
    if (OPState == HIGH) {
      lastTime = millis();
    } else {
      if ((millis() - lastTime) < 3000) {
        doorUnlock();
      }
    }
  }
}

bool checkPasscode() {
  inputBuffer.trim();
  return (inputBuffer == passcode);
}

void blip() {
  digitalWrite(buzzPin, HIGH);
  delay(30);
  digitalWrite(buzzPin, LOW);
}

void inBeep() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(buzzPin, HIGH);
    delay(50);
    digitalWrite(buzzPin, LOW);
    delay(75);
  }
}

void clb() {
  inputBuffer.remove(0);
}

void enableServo(){
  unlocker.attach(servoPin);
  unlocker.writeMicroseconds(lockPos);
}

void disableServo(){
  unlocker.detach();
}

void doorUnlock() {
  //unlock door routine
  digitalWrite(buzzPin, HIGH);
  digitalWrite(entrySignal, HIGH);
  enableServo();
  unlocker.writeMicroseconds(unlockPos);
  Serial.println("servo engaged");
  delay(500);
  digitalWrite(buzzPin, LOW);
  digitalWrite(entrySignal, LOW);
  delay(3000);
  unlocker.writeMicroseconds(lockPos);
  Serial.println("servo disengaged");
  delay(500);
  disableServo();
}


