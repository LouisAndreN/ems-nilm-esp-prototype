#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Preferences.h>

/* 
ESP32 + ADS1115 + SCT-013
Calibration automatique basée sur:
    I_known = P / V
Tu modifies juste knownPower_W au début.
*/

// ========= CALIBRATION SETTINGS ==========
float knownPower_W = 60.0f;      
float mainsVoltage_V = 100.0f;   // Japan -> 100V
// ==============================================

Adafruit_ADS1115 ads;
Preferences prefs;

const float ADS_LSB = 2.048f / 32768.0f; 
const int SAMPLES = 1600;
const int SAMPLE_DELAY_US = 1150;

float calibrationFactor = 5.0f;
const char* PREF_KEY = "sct_factor";
const char* PREF_NAMESPACE = "sct_conf";
const float IGNORE_THRESHOLD_A = 0.001f;

float measure_vrms(int samples);
float measure_irms();
void autoCalibration();


void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("\n=== ESP32 SCT-013 RMS Monitor (Auto Calibration) ===");

  if (!ads.begin()) {
    Serial.println("ERROR: ADS1115 undetected !");
    while (1) delay(1000);
  }
  ads.setGain(GAIN_ONE);

  prefs.begin(PREF_NAMESPACE, false);

  // Load factor if exists
  if (prefs.isKey(PREF_KEY)) {
    calibrationFactor = prefs.getFloat(PREF_KEY, calibrationFactor);
    Serial.print("Loaded calibration factor = ");
    Serial.println(calibrationFactor, 6);
  } else {
    Serial.println("No saved calibration: running AUTO calibration...");
    autoCalibration();
  }

  Serial.println("System ready.");
}

void loop() {
  static uint32_t t0 = millis();
  if (millis() - t0 >= 2000) {
    float i = measure_irms();
    Serial.print("I_rms = ");
    Serial.print(i, 5);
    Serial.println(" A");
    t0 = millis();
  }
}


// ---- RMS CALC ----
float measure_vrms(int samples) {
  double sumsq = 0;
  for (int i = 0; i < samples; i++) {
    int16_t raw = ads.readADC_Differential_0_1();
    float v = raw * ADS_LSB;
    sumsq += v * v;
    delayMicroseconds(SAMPLE_DELAY_US);
  }
  return sqrt(sumsq / samples);
}


// ---- IRMS ----
float measure_irms() {
  float vrms = measure_vrms(SAMPLES);
  return vrms * calibrationFactor;
}


// ---- AUTO CALIBRATION ----
void autoCalibration() {
  Serial.println("=== Auto Calibration Started ===");

  float I_known = knownPower_W / mainsVoltage_V;
  Serial.print("Known load power = ");
  Serial.print(knownPower_W);
  Serial.println(" W");

  Serial.print("Line voltage = ");
  Serial.print(mainsVoltage_V);
  Serial.println(" V");

  Serial.print("Expected I_known = ");
  Serial.print(I_known, 6);
  Serial.println(" A");

  Serial.println("Measuring Vrms...");
  float vrms = measure_vrms(SAMPLES);

  Serial.print("Measured Vrms = ");
  Serial.print(vrms * 1000.0f, 4);
  Serial.println(" mV");

  float newFactor = I_known / vrms;

  calibrationFactor = newFactor;
  prefs.putFloat(PREF_KEY, calibrationFactor);

  Serial.print("New calibration factor = ");
  Serial.println(calibrationFactor, 6);

  Serial.println("Saved in NVS.");
  Serial.println("=== Auto Calibration Done ===");
}
