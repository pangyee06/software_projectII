// Arduino pin assignment
#define PIN_LED  9
#define PIN_TRIG 12   // sonar sensor TRIGGER
#define PIN_ECHO 13   // sonar sensor ECHO

// configurable parameters
#define SND_VEL 346.0     // sound velocity at 24 celsius degree (unit: m/sec)
#define INTERVAL 25       // sampling interval (unit: msec)
#define PULSE_DURATION 10 // ultra-sound Pulse Duration (unit: usec)

#define _DIST_MIN 100.0   // minimum distance (unit: mm)
#define _DIST_MID 200.0   // maximum brightness distance (unit: mm)
#define _DIST_MAX 300.0   // maximum distance (unit: mm)

#define TIMEOUT ((INTERVAL / 2) * 1000.0) // maximum echo waiting time (unit: usec)
#define SCALE (0.001 * 0.5 * SND_VEL)     // convert duration to distance

unsigned long last_sampling_time;   // unit: msec


void setup() {
  // initialize GPIO pins
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);  // sonar TRIGGER
  pinMode(PIN_ECHO, INPUT);   // sonar ECHO

  digitalWrite(PIN_TRIG, LOW);  // turn-off Sonar

  // LED is active low
  // 255 = OFF
  // 0   = maximum brightness
  analogWrite(PIN_LED, 255);

  // initialize serial port
  Serial.begin(57600);

  last_sampling_time = 0;
}


void loop() {
  float distance;
  int pwmValue;

  // wait until next sampling time
  if (millis() < (last_sampling_time + INTERVAL))
    return;

  // read distance
  distance = USS_measure(PIN_TRIG, PIN_ECHO);


  // ----------------------------------------
  // LED brightness control
  // ----------------------------------------

  // measurement failed or distance > 300mm
  if ((distance == 0.0) || (distance > _DIST_MAX)) {

    distance = _DIST_MAX + 10.0;

    // LED OFF
    pwmValue = 255;
  }

  // distance < 100mm
  else if (distance < _DIST_MIN) {

    distance = _DIST_MIN - 10.0;

    // LED OFF
    pwmValue = 255;
  }

  // 100mm ~ 200mm
  else if (distance <= _DIST_MID) {

    /*
      Active Low LED

      distance = 100mm  -> PWM = 255 -> OFF
      distance = 150mm  -> PWM ≈ 128 -> 50%
      distance = 200mm  -> PWM = 0   -> MAX brightness
    */

    pwmValue =
      255.0 * (_DIST_MID - distance)
      / (_DIST_MID - _DIST_MIN);
  }

  // 200mm ~ 300mm
  else {

    /*
      Active Low LED

      distance = 200mm  -> PWM = 0   -> MAX brightness
      distance = 250mm  -> PWM ≈ 128 -> 50%
      distance = 300mm  -> PWM = 255 -> OFF
    */

    pwmValue =
      255.0 * (distance - _DIST_MID)
      / (_DIST_MAX - _DIST_MID);
  }


  // control LED brightness
  analogWrite(PIN_LED, pwmValue);


  // ----------------------------------------
  // Serial output
  // ----------------------------------------

  Serial.print("Min:");
  Serial.print(_DIST_MIN);

  Serial.print(",distance:");
  Serial.print(distance);

  Serial.print(",Max:");
  Serial.print(_DIST_MAX);

  Serial.print(",PWM:");
  Serial.print(pwmValue);

  Serial.println("");


  // delay(50); 삭제
  // sampling interval is 25ms

  // update last sampling time
  last_sampling_time += INTERVAL;
}


// get a distance reading from USS
// return value is in millimeter
float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);

  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE;
}