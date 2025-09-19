# Obstacle-Avoiding-Robot-3-Ultrasonic-Sensors
An Arduino robot that avoids obstacles using three HC-SR04 ultrasonic sensors (Left, Center, Right) and four DC motors driven by the Adafruit Motor Shield (AFMotor library). The sketch uses a simple state machine to stop → back up → choose the freer side → pivot → and resume forward motion.

Highlights
1. Three sensors (L/C/R) for wide forward coverage
2. Deterministic state machine for smooth behavior (no jittery decisions)
3. Easy tuning of speed, thresholds, and timing from constants
4. Uses AFMotor.h (Adafruit Motor Shield v1) — no extra PWM wiring needed
5. Clear pin map and wiring table included

Libraries of Arduino IDE
1. Adafruit Motor Shield (by Adafruit)
#You can install it in Arduino IDE via:1.	Sketch → Include Library → Manage Libraries → Search “Adafruit Motor Shield” → Install “Adafruit Motor Shield Library” (by Adafruit).

Hardware & Libraries
1. DC gear motors ×4
2. Arduino Uno
3. Adafruit Motor Shield v1 
4. Sensors [Ultrasonic sensors (HC-SR04)] ×3
5. Wheel ×4
   
Power
recommended 6–12 V to motor shield

Pin Mapping
1. Ultrasonic (Trig/Echo on analog pins)
     Position  	TRIG	ECHO
      Left	     A0  	A3
      Center	   A1 	 A4
      Right	     A2 	 A5
2. Motors (Adafruit Motor Shield ports)
     motor1	Front-Left	M1
     motor2	Back-Left 	M2
     motor3	Front-Right	M3
     motor4	Back-Right	M4

Tunables (from the uploaded code)
// Speeds (0–255)
const int  SPEED_FWD    = 160;
const int  SPEED_TURN   = 160;
const int  SPEED_BACK   = 140;

// Detection & timing
const int  THRESH_CM    = 32;   // object considered "hit" if <= THRESH_CM
const int  CLEAR_MARGIN = 6;    // extra clearance to resume forward
const int  SENSOR_GAP   = 40;   // ms between sensor pings

const unsigned long MIN_BACK_MS   = 300;
const unsigned long STOP_DELAY_MS = 1000;
const unsigned long TURN_KICK_MS  = 260;
const unsigned long TURN_STEP_MS  = 120;
const unsigned long TURN_MAX_MS   = 1600;


Tip: If the robot over-turns, lower TURN_MS values. If it hesitates, increase CLEAR_MARGIN slightly.
