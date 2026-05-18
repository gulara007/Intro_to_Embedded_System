#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <IRremote.hpp>

// -------------------- Function Declarations --------------------
void readPasswordFromKeypad();
void readPasswordFromRemote();
void checkRemotePassword();
void readRFIDCard();
String getUIDString();
void stopRFIDCommunication();
void showSystemStatus();
void blinkBothLEDs();
char convertIRCommandToDigit(uint16_t command);

// -------------------- Pins --------------------
const byte RFID_RST_PIN = 9;
const byte RFID_SS_PIN  = 10;

const byte IR_PIN        = 3;
const byte RED_LED_PIN   = 6;
const byte GREEN_LED_PIN = 7;

// -------------------- Settings --------------------
const byte CODE_SIZE = 4;

const unsigned long IR_DEBOUNCE_TIME = 450;
const unsigned long RFID_DELAY_TIME  = 3000;

// -------------------- System Mode --------------------
enum SecurityMode {
  ENTERING_NEW_CODE,
  SYSTEM_LOCKED,
  SYSTEM_UNLOCKED
};

SecurityMode mode = ENTERING_NEW_CODE;

String savedPassword = "";
String typedPassword = "";

// -------------------- Keypad Setup --------------------
const byte ROW_COUNT = 4;
const byte COL_COUNT = 4;

char keypadLayout[ROW_COUNT][COL_COUNT] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROW_COUNT] = {4, 5, A0, A1};
byte colPins[COL_COUNT] = {A2, A3, A4, A5};

Keypad keypad = Keypad(
  makeKeymap(keypadLayout),
  rowPins,
  colPins,
  ROW_COUNT,
  COL_COUNT
);

// -------------------- RFID Object --------------------
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

// -------------------- Setup --------------------
void setup() {
  Serial.begin(9600);

  SPI.begin();
  rfid.PCD_Init();

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  Serial.println("--- SECURITY SYSTEM STARTED ---");
  Serial.println("Enter a 4-digit password using the keypad.");
}

// -------------------- Main Loop --------------------
void loop() {
  showSystemStatus();

  if (mode == ENTERING_NEW_CODE) {
    readPasswordFromKeypad();
  } 
  else if (mode == SYSTEM_LOCKED) {
    readPasswordFromRemote();
  } 
  else if (mode == SYSTEM_UNLOCKED) {
    readRFIDCard();
  }
}

// -------------------- Keypad Password Setup --------------------
void readPasswordFromKeypad() {
  char pressedKey = keypad.getKey();

  if (pressedKey == NO_KEY) {
    return;
  }

  if (isDigit(pressedKey)) {
    typedPassword += pressedKey;
    Serial.print("*");

    if (typedPassword.length() == CODE_SIZE) {
      savedPassword = typedPassword;
      typedPassword = "";

      mode = SYSTEM_LOCKED;

      Serial.println();
      Serial.print("[PASSWORD SAVED] Length: ");
      Serial.println(savedPassword.length());
      Serial.println("[LOCKED] Now use the IR remote to unlock.");
    }
  }

  if (pressedKey == '*') {
    typedPassword = "";
    Serial.println();
    Serial.println("[INPUT CLEARED]");
  }
}

// -------------------- IR Remote Password Check --------------------
void readPasswordFromRemote() {
  static unsigned long previousIRTime = 0;
  static uint16_t previousCommand = 0;

  if (!IrReceiver.decode()) {
    return;
  }

  uint16_t receivedCommand = IrReceiver.decodedIRData.command;
  unsigned long currentTime = millis();

  bool isRepeatSignal = IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;

  if (isRepeatSignal) {
    IrReceiver.resume();
    return;
  }

  bool sameCommandTooSoon =
    receivedCommand == previousCommand &&
    currentTime - previousIRTime < IR_DEBOUNCE_TIME;

  if (sameCommandTooSoon) {
    IrReceiver.resume();
    return;
  }

  char receivedDigit = convertIRCommandToDigit(receivedCommand);

  if (receivedDigit != 'X') {
    typedPassword += receivedDigit;

    Serial.print("Remote digit: ");
    Serial.print(receivedDigit);
    Serial.print(" (");
    Serial.print(typedPassword.length());
    Serial.println("/4)");

    previousCommand = receivedCommand;
    previousIRTime = currentTime;

    if (typedPassword.length() == CODE_SIZE) {
      checkRemotePassword();
    }
  }

  IrReceiver.resume();
}

void checkRemotePassword() {
  if (typedPassword == savedPassword) {
    mode = SYSTEM_UNLOCKED;
    Serial.println("[ACCESS GRANTED] RFID reader is now active.");
  } 
  else {
    Serial.println("[ACCESS DENIED] Wrong password. Try again.");
  }

  typedPassword = "";
}

// -------------------- RFID Reading --------------------
void readRFIDCard() {
  static String previousUID = "";
  static unsigned long previousRFIDTime = 0;

  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  String currentUID = getUIDString();
  unsigned long currentTime = millis();

  bool sameCardTooSoon =
    currentUID == previousUID &&
    currentTime - previousRFIDTime < RFID_DELAY_TIME;

  if (sameCardTooSoon) {
    stopRFIDCommunication();
    return;
  }

  previousUID = currentUID;
  previousRFIDTime = currentTime;

  blinkBothLEDs();

  Serial.print("DATA_PACKET:");
  Serial.println(currentUID);

  stopRFIDCommunication();
}

String getUIDString() {
  String uidText = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uidText += "0";
    }

    uidText += String(rfid.uid.uidByte[i], HEX);
  }

  uidText.toUpperCase();
  return uidText;
}

void stopRFIDCommunication() {
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// -------------------- LED Status --------------------
void showSystemStatus() {
  static unsigned long previousBlinkTime = 0;
  static bool blinkValue = false;

  if (mode == ENTERING_NEW_CODE) {
    if (millis() - previousBlinkTime > 500) {
      previousBlinkTime = millis();
      blinkValue = !blinkValue;

      digitalWrite(RED_LED_PIN, blinkValue);
      digitalWrite(GREEN_LED_PIN, blinkValue);
    }
  } 
  else if (mode == SYSTEM_LOCKED) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
  } 
  else if (mode == SYSTEM_UNLOCKED) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
  }
}

void blinkBothLEDs() {
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);
  delay(150);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);
}

// -------------------- IR Code Conversion --------------------
char convertIRCommandToDigit(uint16_t command) {
  switch (command) {
    case 0x16: return '0';
    case 0x0C: return '1';
    case 0x18: return '2';
    case 0x5E: return '3';
    case 0x08: return '4';
    case 0x1C: return '5';
    case 0x5A: return '6';
    case 0x42: return '7';
    case 0x52: return '8';
    case 0x4A: return '9';

    default:
      return 'X';
  }
}

  