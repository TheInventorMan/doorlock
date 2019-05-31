#include <Servo.h>
#include <Keypad.h>
#include <SPI.h>
#include <DhcpV2_0.h>
#include <DnsV2_0.h>
#include <EthernetClientV2_0.h>
#include <EthernetServerV2_0.h>
#include <EthernetUdpV2_0.h>
#include <EthernetV2_0.h>
#include <utilV2_0.h>

#define W5200_CS  10
#define SDCARD_CS 4

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
const byte unlockPos = 1200;
const byte lockPos = 1800;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
const String sendNumber = "+17747770011";
const String apiKey = "JFLISM1K0U1UIYYQ";

byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {9, 8, 7, 6};
byte servoPin = 2;
byte buzzPin = 5;
byte overridePin = A4;
byte entrySignal = A5;
//available pins: 3
String inputBuffer = String();
String passcode = "4D096";
String OTP = "FFFFFF";

bool lastOPState = false;
int OPState = 0;
unsigned long lastTime = 0;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "api.thingspeak.com";

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
Servo unlocker;
IPAddress ip(192, 168, 1, 177);
EthernetClient client;

void setup() {
  Serial.begin(9600);
  unlocker.attach(servoPin);
  pinMode(buzzPin, OUTPUT);
  pinMode(SDCARD_CS, OUTPUT);
  pinMode(overridePin, INPUT);
  pinMode(entrySignal, OUTPUT);
  digitalWrite(SDCARD_CS, HIGH);
  delay(500);
  Serial.println("Setting up Ethernet...");
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
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
  if (inputBuffer == "ABCD") {
    genSendOTP();
    clb();
  }
  if (inputBuffer == OTP) {
    OTP.remove(0);
    OTP += "FFFFFF";
    return true;
  }
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

void doorUnlock() {
  //unlock door routine
  digitalWrite(buzzPin, HIGH);
  digitalWrite(entrySignal, HIGH);
  unlocker.writeMicroseconds(unlockPos);
  Serial.println("servo engaged");
  delay(500);
  digitalWrite(buzzPin, LOW);
  digitalWrite(entrySignal, LOW);
  delay(3000);
  unlocker.writeMicroseconds(lockPos);
  Serial.println("servo disengaged");
}

String URLEncode(const char* msg)
{
  const char *hex = "0123456789abcdef";
  String encodedMsg = "";

  while (*msg != '\0') {
    if ( ('a' <= *msg && *msg <= 'z')
         || ('A' <= *msg && *msg <= 'Z')
         || ('0' <= *msg && *msg <= '9') ) {
      encodedMsg += *msg;
    }
    else {
      encodedMsg += '%';
      encodedMsg += hex[*msg >> 4];
      encodedMsg += hex[*msg & 15];
    }
    msg++;
  }
  return encodedMsg;
}

void sendSMS(String number, String message)
{
  // Make a TCP connection to remote host
  if (client.connect(server, 80))
  {

    //should look like this...
    //api.thingspeak.com/apps/thinghttp/send_request?api_key={api key}&number={send to number}&message={text body}

    client.print("GET /apps/thinghttp/send_request?api_key=");
    client.print(apiKey);
    client.print("&number=");
    client.print(number);
    client.print("&message=");
    client.print(message);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
  }
  else
  {
    Serial.println(F("Connection failed"));
  }

  // Check for a response from the server, and route it
  // out the serial port.
  while (client.connected())
  {
    if ( client.available() )
    {
      char c = client.read();
      Serial.print(c);
    }
  }
  Serial.println();
  client.stop();
}

void genSendOTP() {
  Serial.println("Sending SMS");
  const byte keyChars[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, "A", "B", "C", "D"};
  OTP.remove(0);
  for (int i = 0; i < 6; i++) {
    OTP += keyChars[random(0, 14)];
  };
  char sendStr[10];
  OTP.toCharArray(sendStr, 10);
  sendSMS(sendNumber, URLEncode(sendStr));
}

