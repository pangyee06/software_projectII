// Arduino pin assignment (same as 09_example_1.ino)
#define PIN_LED  9
#define PIN_TRIG 12
#define PIN_ECHO 13

// configurable parameters
#define SND_VEL 346.0      // sound velocity (m/sec)
#define INTERVAL 25        // sampling interval (msec)
#define PULSE_DURATION 10  // trigger pulse duration (usec)
#define _DIST_MIN 100      // reference distance (mm)
#define _DIST_MAX 300      // reference distance (mm)

#define TIMEOUT ((INTERVAL / 2) * 1000.0)
#define SCALE (0.001 * 0.5 * SND_VEL)

#define _MEDIAN_N 3        // change to 3, 10, or 30 for each experiment

#if _MEDIAN_N < 1
#error "_MEDIAN_N must be at least 1"
#endif

// global variables
unsigned long last_sampling_time = 0;
float dist_ema;            // raw passthrough: EMA is disabled for this task
float dist_median;

float dist_samples[_MEDIAN_N];  // circular buffer: recent raw samples
int sample_index = 0;           // next position to overwrite
int sample_count = 0;           // number of samples actually collected

// function declarations
float USS_measure(int TRIG, int ECHO);
float median_filter(float new_sample);

void setup() {
  // initialize GPIO pins
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_LED, HIGH);   // LED OFF (active-low, as in the example)
  digitalWrite(PIN_TRIG, LOW);

  // initialize serial port
  Serial.begin(57600);
}

void loop() {
  float dist_raw;
  unsigned long current_time = millis();

  // unsigned subtraction also works across millis() rollover
  if (current_time - last_sampling_time < INTERVAL)
    return;
  last_sampling_time = current_time;

  // Keep ALL raw readings, including timeout (= 0) and out-of-range values.
  // Do not replace errors with the previous valid distance.
  dist_raw = USS_measure(PIN_TRIG, PIN_ECHO);
  dist_median = median_filter(dist_raw);

  // Keep the slide's output format, but apply only the median filter.
  // This passthrough is equivalent to EMA with alpha = 1.
  dist_ema = dist_raw;

  // output the distance to the serial port (no clipping)
  Serial.print("Min:");     Serial.print(_DIST_MIN);
  Serial.print(",raw:");    Serial.print(dist_raw);
  Serial.print(",ema:");    Serial.print(dist_ema);
  Serial.print(",median:"); Serial.print(dist_median);
  Serial.print(",Max:");    Serial.print(_DIST_MAX);
  Serial.println("");

  // Keep the original LED behavior: indicate the RAW distance range.
  if ((dist_raw < _DIST_MIN) || (dist_raw > _DIST_MAX))
    digitalWrite(PIN_LED, HIGH);  // LED OFF
  else
    digitalWrite(PIN_LED, LOW);   // LED ON
}

// get a distance reading from USS; return value is in millimeters
float USS_measure(int TRIG, int ECHO) {
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);

  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE;
}

// Store the new raw sample and return the median of the recent samples.
float median_filter(float new_sample) {
  float sorted[_MEDIAN_N];

  dist_samples[sample_index] = new_sample;
  sample_index = (sample_index + 1) % _MEDIAN_N;

  if (sample_count < _MEDIAN_N)
    sample_count++;

  // Sort a COPY so the circular buffer keeps its overwrite order.
  for (int i = 0; i < sample_count; i++)
    sorted[i] = dist_samples[i];

  // insertion sort, ascending order
  for (int i = 1; i < sample_count; i++) {
    float key = sorted[i];
    int j = i - 1;

    while ((j >= 0) && (sorted[j] > key)) {
      sorted[j + 1] = sorted[j];
      j--;
    }
    sorted[j + 1] = key;
  }

  int middle = sample_count / 2;

  if (sample_count % 2 == 1)
    return sorted[middle];
  else
    return (sorted[middle - 1] + sorted[middle]) / 2.0;
}
