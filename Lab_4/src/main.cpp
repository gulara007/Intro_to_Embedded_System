#include <Arduino.h>

const int ledXPositive = 2;
const int ledXNegative = 3;
const int ledYPositive = 5;
const int ledYNegative = 4;

const int joystickX = A0;
const int joystickY = A1;

const int threshold = 200;
const int centerValue = 512;

void controlLEDs(int axisValue, int posPin, int negPin);

void setup() {
  pinMode(ledXPositive, OUTPUT);
  pinMode(ledXNegative, OUTPUT);
  pinMode(ledYPositive, OUTPUT);
  pinMode(ledYNegative, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  int xValue = analogRead(joystickX);
  int yValue = analogRead(joystickY);

  Serial.print("X: ");
  Serial.print(xValue);
  Serial.print("\tY: ");
  Serial.println(yValue);

  controlLEDs(xValue, ledXPositive, ledXNegative);
  controlLEDs(yValue, ledYPositive, ledYNegative);

  delay(50);
}

void controlLEDs(int axisValue, int posPin, int negPin) {
  if (axisValue > centerValue + threshold) {
    digitalWrite(posPin, HIGH);
    digitalWrite(negPin, LOW);
  } 
  else if (axisValue < centerValue - threshold) {
    digitalWrite(posPin, LOW);
    digitalWrite(negPin, HIGH);
  } 
  else {
    digitalWrite(posPin, LOW);
    digitalWrite(negPin, LOW);
  }
}