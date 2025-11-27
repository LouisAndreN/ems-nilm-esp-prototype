#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Preferences.h>

/* 
ESP32 + ADS1115 + SCT-013
   - Measure RMS continuously
   - Interactive calibration via Serial
   - Save calibration factor in NVS (Preferences)
*/

Adafruit_ADS1115 ads;
Preferences prefs;

const float ADS_LSB = 2.048f / 32768.0f; // 62.5 uV (GAIN_ONE)
const int SAMPLES = 1600;    
const int SAMPLE_DELAY_US = 1150; // ~870 SPS

// Configuration
float calibrationFactor = 5.0f;   // 5A/1V
const char* PREF_KEY = "sct_factor";
const char* PREF_NAMESPACE = "sct_conf";
const float IGNORE_THRESHOLD_A = 0.001f; // ignore < 1 mA


float measure_vrms(int samples);
float measure_irms();
void interactiveCalibration();

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("=== ESP32 SCT-013 RMS Monitor ===");

  if (!ads.begin()) {
    Serial.println("ERROR: ADS1115 undetected !");
    while (1) delay(1000);
  }
  ads.setGain(GAIN_ONE); // ±2.048 V

  // Load saved calibration factor if exists
  prefs.begin(PREF_NAMESPACE, false);
  if (prefs.isKey(PREF_KEY)) {
    calibrationFactor = prefs.getFloat(PREF_KEY, calibrationFactor);
    Serial.print("Calibration factor = ");
    Serial.println(calibrationFactor, 6);
  } else {
    Serial.print("No saved calibration. Default factor = ");
    Serial.println(calibrationFactor, 6);
  }

  Serial.println("Serial Commands: c = calibrate, r = measure, d = plot factor, x = erase factor");
  Serial.println("----------------------------------------");
}

void loop() {
  // Receive command on Serial
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'c') { interactiveCalibration(); }
    else if (cmd == 'r') {
      float i = measure_irms();
      Serial.print("I_rms (forced) = "); 
      Serial.print(i, 6); 
      Serial.println(" A");
    }
    else if (cmd == 'd') {
      Serial.print("Calibration factor = "); 
      Serial.println(calibrationFactor, 6);
    }
    else if (cmd == 'x') {
      prefs.remove(PREF_KEY);
      Serial.println("Calibration erased.");
    }
  }

  // Periodically measure IRMS
  static uint32_t t0 = millis();
  if (millis() - t0 >= 2000) { // every 2s
    float i = measure_irms();
    if (i < IGNORE_THRESHOLD_A) {
      Serial.print("I_rms <"); 
      Serial.print(IGNORE_THRESHOLD_A, 3); 
      Serial.println(" A (noise)");
    } else {
      Serial.print("I_rms: "); 
      Serial.print(i, 4); 
      Serial.println(" A");
    }
    t0 = millis();
  }
}

// Measure VRMS with differential A0-A1 in volts
float measure_vrms(int samples) {
  double sumsq = 0.0;
  for (int i = 0; i < samples; ++i) {
    int16_t raw = ads.readADC_Differential_0_1();
    float v = raw * ADS_LSB;
    sumsq += (double)v * (double)v;
    delayMicroseconds(SAMPLE_DELAY_US);
  }
  float meanSq = (float)(sumsq / (double)samples);
  return sqrt(meanSq);
}

// Measure corrected IRMS
float measure_irms() {
  // Measure VRMS
  float vrms = measure_vrms(SAMPLES);

  // Simple correction baseline : measure baseline fast (not ideal but useful)
  // No quadratic substraction, clean calibration.
  float irms = vrms * calibrationFactor;
  return irms;
}

// Routine interactive de calibration:
// - measure VRMS on N samples
// - ask known I to user
// - calculate factor = I_known / VRMS
// - ask to save
void interactiveCalibration() {
  Serial.println("=== Interactive calibration ===");
  Serial.println("Place known load (resistive). Press ENTER to start measure.");
  while (!Serial.available()) 
  { 
    delay(10); 
  }
  while (Serial.available()) 
    Serial.read(); // clear buffer

  Serial.println("Measure in progress...");
  float vrms = measure_vrms(SAMPLES);
  Serial.print("Vrms mesure = "); 
  Serial.print(vrms * 1000.0f, 4); 
  Serial.println(" mV");

  Serial.println("Put known current value in A :");
  // read user input
  String s = "";
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\r') continue;
      if (c == '\n') break;
      s += c;
    }
    delay(10);
  }
  float iknown = s.toFloat();
  if (iknown <= 0.0f) {
    Serial.println("Invalid value.");
    return;
  }

  float newFactor = iknown / vrms;
  Serial.print("New factor calculated = "); 
  Serial.println(newFactor, 6);
  Serial.println("Save ? (y/n)");

  // read yes/no
  while (!Serial.available()) 
  { 
    delay(10); 
  }
  char r = Serial.read();
  if (r == 'y' || r == 'Y') {
    calibrationFactor = newFactor;
    prefs.putFloat(PREF_KEY, calibrationFactor);
    Serial.println("Factor saved in NVS.");
  } else {
    Serial.println("Calibration unsaved.");
    calibrationFactor = newFactor;
  }
  Serial.println("=== Calibration finished ===");
}

