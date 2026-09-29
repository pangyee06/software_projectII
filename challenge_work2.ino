const int LED_PIN = 7;

int pwmPeriod = 1000;
int pwmDuty = 0;

void set_period(int period) {
  if (period < 100) {
    period = 100;
  }

  if (period > 10000) {
    period = 10000;
  }

  pwmPeriod = period;
}

void set_duty(int duty) {
  if (duty < 0) {
    duty = 0;
  }

  if (duty > 100) {
    duty = 100;
  }

  pwmDuty = duty;
}

void pwm_once() {
  int onTime = (long)pwmPeriod * pwmDuty / 100;
  int offTime = pwmPeriod - onTime;

  if (pwmDuty == 0) {
    digitalWrite(LED_PIN, HIGH);
    delayMicroseconds(pwmPeriod);
  }
  else if (pwmDuty == 100) {
    digitalWrite(LED_PIN, LOW);
    delayMicroseconds(pwmPeriod);
  }
  else {
    digitalWrite(LED_PIN, LOW);
    delayMicroseconds(onTime);

    digitalWrite(LED_PIN, HIGH);
    delayMicroseconds(offTime);
  }
}

void triangle_fade() {
  unsigned long startTime = micros();

  while (micros() - startTime < 1000000UL) {
    unsigned long elapsedTime = micros() - startTime;

    if (elapsedTime < 500000UL) {
      int duty = elapsedTime * 100 / 500000UL;
