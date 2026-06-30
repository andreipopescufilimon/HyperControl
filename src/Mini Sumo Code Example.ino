#include <Arduino.h>
#include <Servo.h>

// ====================== PINS ======================
#define PWMA 5
#define DIRA 3
#define PWMB 6
#define DIRB 4

#define DIP1 2  // flag enable
#define DIP2 7  // speed mode
#define LED3 13

#define RightSensor 9
#define RightDiagonalSensor A0
#define MiddleSensor A1
#define LeftDiagonalSensor A3
#define LeftSensor A5

#define LeftLine A4
#define RightLine 10

#define FLAG A2
#define Start 12

Servo flagServo;


bool justStarted = false;


// ====================== VARIABLES ======================
int lastSeen = 0;  // -1 left, 1 right, 0 center
bool escaping = false;
unsigned long escapeTimer = 0;

bool ledState = false;
unsigned long ledTimer = 0;

// ====================== MOTOR CONTROL ======================
void setMotorA(int s) {
  analogWrite(PWMA, abs(s));
  digitalWrite(DIRA, s < 0 ? LOW : HIGH);
}
void setMotorB(int s) {
  analogWrite(PWMB, abs(s));
  digitalWrite(DIRB, s < 0 ? LOW : HIGH);
}
void drive(int L, int R) {
  setMotorA(L);
  setMotorB(R);
}
void stopMotors() {
  drive(0, 0);
}

// ====================== SAFE EDGE ESCAPE ======================
void beginEscape() {
  escaping = true;
  escapeTimer = millis();
}

void handleEscape(bool highSpeed) {
  unsigned long now = millis();

  // HALF SPEED VERSION OF ESCAPE
  int revL = highSpeed ? -150 : -55;
  int revR = highSpeed ? -150 : -70;

  // Reverse for 150mss

  drive(revL, revR);
  delay(200);

  static int escapeDir = 0;
  if (escapeDir == 0) {
    if (digitalRead(LeftLine) == LOW) escapeDir = 1;
    else if (digitalRead(RightLine) == LOW) escapeDir = -1;
    else escapeDir = (random(0, 2) == 0 ? 1 : -1);
  }


  int t = highSpeed ? 255 : 255;
  if (escapeDir == 1) drive(-t, t);
  else drive(t, -t);

  delay(150);


  escaping = false;
  escapeDir = 0;
  //stopMotors();
}

// ====================== SETUP ======================
void setup() {
  pinMode(PWMA, OUTPUT);
  pinMode(DIRA, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(DIRB, OUTPUT);

  pinMode(DIP1, INPUT_PULLUP);
  pinMode(DIP2, INPUT_PULLUP);
  pinMode(LED3, OUTPUT);

  pinMode(RightSensor, INPUT);
  pinMode(RightDiagonalSensor, INPUT);
  pinMode(MiddleSensor, INPUT);
  pinMode(LeftDiagonalSensor, INPUT);
  pinMode(LeftSensor, INPUT);
  pinMode(LeftLine, INPUT);
  pinMode(RightLine, INPUT);
  pinMode(Start, INPUT);

  flagServo.attach(FLAG);
  flagServo.write(120);
}

// ====================== MAIN LOOP ======================
void loop() {

  // STOP MODE
  if (digitalRead(Start) == LOW) {
    flagServo.write(120);
    stopMotors();
    if (millis() - ledTimer > 500) {
      ledState = !ledState;
      digitalWrite(LED3, ledState);
      ledTimer = millis();
    }
    return;
  }
  
  if (digitalRead(Start) == HIGH && justStarted == false) {
    drive(0, 0);
    delay(4800);
    justStarted = true;
    flagServo.write(15);
    drive(120, 120);
    delay(600);
  }

  digitalWrite(LED3, HIGH);

  bool flagEnabled = (digitalRead(DIP1) == HIGH);
  bool highSpeedMode = (digitalRead(DIP2) == HIGH);

  // HALF SPEEDS ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
  int forwardSpeed = highSpeedMode ? 110 : 45;
  int curveFast = highSpeedMode ? 120 : 100;
  int curveSlow = highSpeedMode ? 40 : 20;
  int searchTurn = highSpeedMode ? 75 : 35;

  // Flag always down if enabled
  if (flagEnabled) flagServo.write(15);
  else flagServo.write(120);

  // Edge detection
  if (digitalRead(LeftLine) == LOW || digitalRead(RightLine) == LOW) {
    beginEscape();
  }

  if (escaping) {
    handleEscape(highSpeedMode);
    return;
  }

  // ====================== ENEMY LOCK (with SIDE SENSORS) ======================

  bool enemy = false;

  // ---- LEFT DIAGONAL ----
  if (digitalRead(LeftDiagonalSensor) == HIGH) {
    drive(curveSlow, curveFast);
    lastSeen = -1;
    enemy = true;
  }

  // ---- RIGHT DIAGONAL ----
  else if (digitalRead(RightDiagonalSensor) == HIGH) {
    drive(curveFast, curveSlow);
    lastSeen = 1;
    enemy = true;
  }

  // ---- MIDDLE FRONT LOCK ----
  else if (digitalRead(MiddleSensor) == HIGH) {
    drive(forwardSpeed, forwardSpeed);
    lastSeen = 0;
    enemy = true;
  }

  // ---- LEFT SIDE sees opponent (strong curve LEFT) ----
  else if (digitalRead(LeftSensor) == HIGH) {
    drive(-50, 200);
    lastSeen = -1;
    enemy = true;
  }

  // ---- RIGHT SIDE sees opponent (strong curve RIGHT) ----
  else if (digitalRead(RightSensor) == HIGH) {
    drive(200, -50);
    lastSeen = 1;
    enemy = true;
  }

  // ====================== SEARCH / ANTI-BACKTAKE ======================
  if (!enemy) {

    // Last seen LEFT → rotate left
    if (lastSeen == -1) {
      drive(100, 200);
    }
    // Last seen RIGHT → rotate right
    else if (lastSeen == 1) {
      drive(200, 100);
    }
    // No memory → default clockwise patrol
    else {
      drive(200, 100);
    }
  }
}
