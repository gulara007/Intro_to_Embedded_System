#include <Arduino.h>
#include <Stepper.h>
#include <Servo.h>

Servo winnerServo;
const int servoPin = 11;


const int player1Button = 2;
const int player2Button = 3;

const int ledPlayer1 = 9;   
const int ledPlayer2 = 10;  

const int stepperIN1 = 4;
const int stepperIN2 = 5;
const int stepperIN3 = 6;
const int stepperIN4 = 7;

const int buzzerPin = 8;

const int stepsPerRevolution = 2048;
Stepper gameStepper(stepsPerRevolution, stepperIN1, stepperIN3, stepperIN2, stepperIN4);

int player1Score = 0;
int player2Score = 0;
const int winningScore = 3;
const int tugSteps = 256; 
const int maxWaitSeconds = 7; 

void startGame();
void playRound();
void awardPoint(int winner, long reactionTime);
void victorySpin(int winner);
void beepFalseStart();
void beepVictory();
bool buttonPressed(int buttonPin);
void waitForButtonsReleased();
void flashLeds(int times, int speed);
void checkSerialStart();

void setup() {
  Serial.begin(9600);

  pinMode(player1Button, INPUT_PULLUP);
  pinMode(player2Button, INPUT_PULLUP);
  pinMode(ledPlayer1, OUTPUT);
  pinMode(ledPlayer2, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  gameStepper.setSpeed(10);
  randomSeed(analogRead(A1)); 
  winnerServo.attach(servoPin);
  winnerServo.write(90);

  Serial.println("SYSTEM_READY");
}

void victorySpin() {
  gameStepper.setSpeed(15); 
  gameStepper.step(2048);
}

void loop() {
  checkSerialStart();

  if (buttonPressed(player1Button) && buttonPressed(player2Button)) {
    delay(500);
    waitForButtonsReleased();
    startGame();
  }
}

void checkSerialStart() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command == "START") {
      startGame();
    }
  }
}

void startGame() {
  player1Score = 0;
  player2Score = 0;
  
  Serial.println("GAME_STARTED"); 
  flashLeds(3, 100); 

  while (player1Score < winningScore && player2Score < winningScore) {
    playRound();
    
    Serial.print("SCORE:");
    Serial.print(player1Score);
    Serial.print(",");
    Serial.println(player2Score);
    
    delay(1500);
    digitalWrite(ledPlayer1, LOW);
    digitalWrite(ledPlayer2, LOW);
  }

  if (player1Score == winningScore) {
    Serial.println("WINNER:P1");
    victorySpin(1);
  } else {
    Serial.println("WINNER:P2");
    victorySpin(2);
  }
}

void playRound() {
  unsigned long waitTime = random(1000, maxWaitSeconds * 1000UL + 1);
  unsigned long waitStart = millis();

  while (millis() - waitStart < waitTime) {
    if (buttonPressed(player1Button)) {
      Serial.println("FALSE_START:P1");
      beepFalseStart();
      awardPoint(2, 0);
      return;
    }
    if (buttonPressed(player2Button)) {
      Serial.println("FALSE_START:P2");
      beepFalseStart();
      awardPoint(1, 0); 
      return;
    }
  }

  tone(buzzerPin, 1000);
  unsigned long reactionStart = millis();

  while (true) {
    bool p1 = buttonPressed(player1Button);
    bool p2 = buttonPressed(player2Button);

    if (p1 && p2) {
      noTone(buzzerPin);
      Serial.println("ROUND_DRAW");
      return;
    }
    if (p1) {
      unsigned long rTime = millis() - reactionStart;
      noTone(buzzerPin);
      Serial.print("REACTION:P1:");
      Serial.println(rTime);
      awardPoint(1, rTime);
      return;
    }
    if (p2) {
      unsigned long rTime = millis() - reactionStart;
      noTone(buzzerPin);
      Serial.print("REACTION:P2:");
      Serial.println(rTime);
      awardPoint(2, rTime);
      return;
    }
  }
}

void awardPoint(int winner, long reactionTime) {
  if (winner == 1) {
    player1Score++;
    winnerServo.write(140);
    digitalWrite(ledPlayer1, HIGH);
    gameStepper.step(tugSteps);
  } else {
    player2Score++;
    winnerServo.write(20);
    digitalWrite(ledPlayer2, HIGH);
    gameStepper.step(-tugSteps);
  }
}

void victorySpin(int winner) {
  beepVictory();
  for(int i = 0; i < 10; i++) {
    int targetLed = (winner == 1) ? ledPlayer1 : ledPlayer2;
    digitalWrite(targetLed, HIGH);
    gameStepper.step(winner == 1 ? 200 : -200);
    digitalWrite(targetLed, LOW);
    delay(50);
  }
}

void flashLeds(int times, int speed) {
  for (int i = 0; i < times; i++) {
    digitalWrite(ledPlayer1, HIGH); digitalWrite(ledPlayer2, HIGH);
    delay(speed);
    digitalWrite(ledPlayer1, LOW); digitalWrite(ledPlayer2, LOW);
    delay(speed);
  }
}

void beepFalseStart() { tone(buzzerPin, 300, 400); }

void beepVictory() {
  tone(buzzerPin, 800, 200); delay(250);
  tone(buzzerPin, 1200, 300);
}

bool buttonPressed(int buttonPin) {
  if (digitalRead(buttonPin) == LOW) {
    delay(30); 
    return (digitalRead(buttonPin) == LOW);
  }
  return false;
}

void waitForButtonsReleased() {
  while (digitalRead(player1Button) == LOW || digitalRead(player2Button) == LOW) {
    delay(10);
  }

  victorySpin();
}