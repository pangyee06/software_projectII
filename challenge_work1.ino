#include <Servo.h>

// Pin assignment
#define PIN_SERVO 10
#define PIN_TRIG 12
#define PIN_ECHO 13

// Ultrasonic sensor
#define SND_VEL 346.0
#define INTERVAL 25
#define PULSE_DURATION 10

#define _DIST_MIN 50.0
#define _DIST_MAX 360.0

#define TIMEOUT ((INTERVAL / 2) * 1000UL)
#define SCALE (0.001 * 0.5 * SND_VEL)

// EMA filter
#define _EMA_ALPHA 0.3

// Gate conditions
#define OPEN_DISTANCE 180.0
#define CLOSE_DISTANCE 250.0

// Servo control
#define SERVO_CLOSED 0
#define SERVO_OPEN 90
#define MOVING_TIME 1500

Servo myServo;

float dist_ema = _DIST_MAX;
float dist_prev = _DIST_MAX;

unsigned long last_sampling_time = 0;
unsigned long moveStartTime = 0;

float startAngle = SERVO_CLOSED;
float targetAngle = SERVO_CLOSED;
float currentAngle = SERVO_CLOSED;

bool gateOpen = false;
bool isMoving = false;


// -------------------------------
// 1. Sigmoid function
// -------------------------------
float sigmoid(float t) {

  t = constrain(t, 0.0, 1.0);

  float k = 6.0;

  float y = 1.0 / (1.0 + exp(-k * (2.0 * t - 1.0)));

  // Normalize to [0, 1]
  float minVal = 1.0 / (1.0 + exp(k));
  float maxVal = 1.0 / (1.0 + exp(-k));

  return (y - minVal) / (maxVal - minVal);
}


// -------------------------------
// 2. Smoothstep function
// -------------------------------
float smoothstep(float t) {

  t = constrain(t, 0.0, 1.0);

  return 3.0 * t * t - 2.0 * t * t * t;
}


// -------------------------------
// Ultrasonic sensor measurement
// -------------------------------
float USS_measure() {

  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);

  digitalWrite(PIN_TRIG, LOW);

  unsigned long duration =
      pulseIn(PIN_ECHO, HIGH, TIMEOUT);

  if (duration == 0) {
    return 0.0;
  }

  return duration * SCALE;
}


// -------------------------------
// Start gate movement
// -------------------------------
void startMovement(float newTarget) {

  startAngle = currentAngle;
  targetAngle = newTarget;

  moveStartTime = millis();
  isMoving = true;
}


// -------------------------------
// Update servo position
// -------------------------------
void updateServo() {

  if (!isMoving) {
    return;
  }

  unsigned long progress = millis() - moveStartTime;

  float t = (float)progress / MOVING_TIME;

  if (t >= 1.0) {
    t = 1.0;
    isMoving = false;
  }

  // -------------------------------
  // Select easing function
  // -------------------------------

  //float eased = sigmoid(t);       // Sigmoid
  float eased = smoothstep(t); // Smoothstep

  // -------------------------------

  currentAngle =
      startAngle + (targetAngle - startAngle) * eased;

  myServo.write((int)currentAngle);
}


// -------------------------------
// Setup
// -------------------------------
void setup() {

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  digitalWrite(PIN_TRIG, LOW);

  myServo.attach(PIN_SERVO);
  myServo.write(SERVO_CLOSED);

  Serial.begin(57600);
}


// -------------------------------
// Main loop
// -------------------------------
void loop() {

  unsigned long now = millis();

  updateServo();

  if (now - last_sampling_time < INTERVAL) {
    return;
  }

  last_sampling_time = now;

  float dist_raw = USS_measure();
  float dist_filtered;

  // Reject invalid measurements
  if (dist_raw == 0.0 ||
      dist_raw < _DIST_MIN ||
      dist_raw > _DIST_MAX) {

    dist_filtered = dist_prev;

  } else {

    dist_filtered = dist_raw;
    dist_prev = dist_raw;
  }

  // EMA filter
  dist_ema =
      _EMA_ALPHA * dist_filtered
      + (1.0 - _EMA_ALPHA) * dist_ema;


  // Open gate
  if (!gateOpen && dist_ema <= OPEN_DISTANCE) {

    gateOpen = true;
    startMovement(SERVO_OPEN);
  }

  // Close gate
  else if (gateOpen && dist_ema >= CLOSE_DISTANCE) {

    gateOpen = false;
    startMovement(SERVO_CLOSED);
  }

  // Serial monitor
  Serial.print("Raw: ");
  Serial.print(dist_raw);

  Serial.print(" / EMA: ");
  Serial.print(dist_ema);

  Serial.print(" / Angle: ");
  Serial.println(currentAngle);
}