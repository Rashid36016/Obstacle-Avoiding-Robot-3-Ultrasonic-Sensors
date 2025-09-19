#include <AFMotor.h>

/* =================== Pin Mapping =================== */
// Left
#define TRIG1 A0
#define ECHO1 A3
// Middle
#define TRIG2 A1
#define ECHO2 A4
// Right
#define TRIG3 A2
#define ECHO3 A5

/* =================== Motors =================== */
AF_DCMotor motor1(1);  // Front-Left
AF_DCMotor motor2(2);  // Back-Left
AF_DCMotor motor3(3);  // Front-Right
AF_DCMotor motor4(4);  // Back-Right

/* =================== Tunables =================== */
const int  SPEED_FWD    = 160;   // 0–255
const int  SPEED_TURN   = 160;
const int  SPEED_BACK   = 140;

const int  THRESH_CM    = 32;    // অবজেক্ট রেঞ্জ (<= হলে হিট)
const int  CLEAR_MARGIN = 6;     // ক্লিয়ার ধরার জন্য বাড়তি মার্জিন
const int  SENSOR_GAP   = 40;    // ক্রস-টক কমাতে সামান্য বড় গ্যাপ (ms)

// সিকোয়েন্স টাইমিং
const unsigned long MIN_BACK_MS   = 300;   // কমপক্ষে এতক্ষণ ব্যাক
const unsigned long STOP_DELAY_MS = 1000;  // প্রতিটা Stop-এর মিনিমাম সময়
const unsigned long TURN_KICK_MS  = 260;   // টার্ন শুরুর কিক
const unsigned long TURN_STEP_MS  = 120;   // কিকের পর ছোট টার্ন স্টেপ
const unsigned long TURN_MAX_MS   = 1600;  // সর্বোচ্চ টার্ন উইন্ডো

/* =================== State Machine =================== */
enum Phase { P_FWD, P_STOP1, P_BACK, P_STOP2, P_TURN, P_STOP3 };
Phase         phase          = P_FWD;
unsigned long phaseStart     = 0;
unsigned long turnAccum      = 0;
bool          turnLeftChoice = true;

inline unsigned long inPhaseMs() { return millis() - phaseStart; }
void goPhase(Phase p);  // forward decl

/* =================== Helpers =================== */
long readCM(uint8_t trig, uint8_t echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(4);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);

  unsigned long t = pulseIn(echo, HIGH, 30000UL);
  if (t == 0) return 999;
  return (long)(t / 29 / 2);
}

void setSpeedAll(int s) {
  motor1.setSpeed(s); motor2.setSpeed(s);
  motor3.setSpeed(s); motor4.setSpeed(s);
}

void stopAll() {
  motor1.run(RELEASE); motor2.run(RELEASE);
  motor3.run(RELEASE); motor4.run(RELEASE);
}

void forward() {
  setSpeedAll(SPEED_FWD);
  motor1.run(FORWARD); motor2.run(FORWARD);
  motor3.run(FORWARD); motor4.run(FORWARD);
}

void backward() {
  setSpeedAll(SPEED_BACK);
  motor1.run(BACKWARD); motor2.run(BACKWARD);
  motor3.run(BACKWARD); motor4.run(BACKWARD);
}

void leftTurn() {
  setSpeedAll(SPEED_TURN);
  motor1.run(BACKWARD); motor2.run(BACKWARD);
  motor3.run(FORWARD ); motor4.run(FORWARD );
}

void rightTurn() {
  setSpeedAll(SPEED_TURN);
  motor1.run(FORWARD ); motor2.run(FORWARD );
  motor3.run(BACKWARD); motor4.run(BACKWARD);
}

/* ========== State switcher ========== */
void goPhase(Phase p) {
  phase = p;
  phaseStart = millis();
  switch (p) {
    case P_FWD:   forward(); break;
    case P_STOP1:
    case P_STOP2:
    case P_STOP3: stopAll(); break;
    case P_BACK:  backward(); break;
    case P_TURN:  (turnLeftChoice ? leftTurn() : rightTurn()); break;
  }
}

/* =================== Arduino Core =================== */
void setup() {
  pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);

  // ট্রিগার পিনগুলোকে LOW দিয়ে শুরু
  digitalWrite(TRIG1, LOW);
  digitalWrite(TRIG2, LOW);
  digitalWrite(TRIG3, LOW);

  goPhase(P_FWD);
}

void loop() {
  // ---- সেন্সর রিড ----
  int distL = (int)readCM(TRIG1, ECHO1); delay(SENSOR_GAP);
  int distC = (int)readCM(TRIG2, ECHO2); delay(SENSOR_GAP);
  int distR = (int)readCM(TRIG3, ECHO3); delay(SENSOR_GAP);

  bool hitL = (distL <= THRESH_CM);
  bool hitC = (distC <= THRESH_CM);
  bool hitR = (distR <= THRESH_CM);
  bool anyHit = (hitL || hitC || hitR);

  // ===== Global Kill-Switch =====
  if (anyHit && (phase == P_FWD || phase == P_TURN || phase == P_STOP3)) {
    goPhase(P_STOP1);
    return;
  }

  switch (phase) {
    case P_FWD: {
      forward();
    } break;

    case P_STOP1: {
      if (inPhaseMs() >= STOP_DELAY_MS) goPhase(P_BACK);
    } break;

    case P_BACK: {
      if (anyHit) {
        backward(); // continuous back
      } else {
        if (inPhaseMs() >= MIN_BACK_MS) goPhase(P_STOP2);
      }
    } break;

    case P_STOP2: {
      if (inPhaseMs() >= STOP_DELAY_MS) {
        if (hitL && !hitR) {
          turnLeftChoice = false; // Right turn
        } else if (hitR && !hitL) {
          turnLeftChoice = true;  // Left turn
        } else {
          int gapL = (distL == 999) ? 999 : distL;
          int gapR = (distR == 999) ? 999 : distR;
          turnLeftChoice = (gapL > gapR);
        }
        turnAccum = 0;
        goPhase(P_TURN);
      }
    } break;

    case P_TURN: {
      if ((distC > THRESH_CM + CLEAR_MARGIN) && (inPhaseMs() >= TURN_KICK_MS)) {
        goPhase(P_STOP3);
      } else {
        if (inPhaseMs() >= TURN_KICK_MS + TURN_STEP_MS) {
          turnAccum += TURN_STEP_MS;
          phaseStart = millis();
          (turnLeftChoice ? leftTurn() : rightTurn());
        }
        if (turnAccum >= TURN_MAX_MS) {
          goPhase(P_BACK);
        }
      }
    } break;

    case P_STOP3: {
      if (inPhaseMs() >= STOP_DELAY_MS) goPhase(P_FWD);
    } break;
  }

  delay(20);
}
