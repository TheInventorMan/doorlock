#include <Servo.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>

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
//RFID Consts
constexpr uint8_t RST_PIN = A5;
constexpr uint8_t SS_PIN = 10;

byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {9, 8, 7, 6};
byte servoPin = 2;
byte buzzPin = 5;
byte overridePin = A4;
//available pins: 3
String inputBuffer = String();
String passcode = "4D096";

bool lastOPState = false;
int OPState = 0;
unsigned long lastTime = 0;
byte nuidPICC[4];
bool flag = false;
byte myCard[4] = {206, 129, 16, 141};

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
Servo unlocker;
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(9600);
  pinMode(buzzPin, OUTPUT);
  pinMode(overridePin, INPUT);
  SPI.begin(); // Init SPI bus

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));


  delay(1000);
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

void enableServo() {
  unlocker.attach(servoPin);
  unlocker.writeMicroseconds(lockPos);
}

void disableServo() {
  unlocker.detach();
}

void doorUnlock() {
  //unlock door routine
  digitalWrite(buzzPin, HIGH);
  enableServo();
  unlocker.writeMicroseconds(unlockPos);
  Serial.println("servo engaged");
  delay(500);
  digitalWrite(buzzPin, LOW);
  delay(3000);
  unlocker.writeMicroseconds(lockPos);
  Serial.println("servo disengaged");
  delay(500);
  disableServo();
}

void checkRFID() {

  rfid.PCD_Init();
  // Look for new cards
  if ( ! rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  //  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
  //      rfid.uid.uidByte[1] != nuidPICC[1] ||
  //      rfid.uid.uidByte[2] != nuidPICC[2] ||
  //      rfid.uid.uidByte[3] != nuidPICC[3] ) {

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
    Serial.println(nuidPICC[i]);
  }
  for (byte i = 0; i < 4; i++) {
    if (nuidPICC[i] != myCard[i]) {
      flag = true;
      break;
    }
  }
  if (flag == false) {
    doorUnlock();
  } else {
    flag = false;
    inBeep();
  }
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = 0;
  }


  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}


void loop() {

  checkRFID();

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



