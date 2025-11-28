#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Preferences.h>

/* 
ESP32 + ADS1115 + SCT-013
Calibration automatique basée sur:
    I_known = P / V
Modifié pour recalibrer à chaque démarrage et afficher IRMS
*/

// ========= CALIBRATION SETTINGS ==========
float knownCurrent_I = 0.35f;      // courant de référence
float mainsVoltage_V = 100.0f;     // tension secteur
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

unsigned long t0_start;  // temps de début du programme


void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("\n=== ESP32 SCT-013 RMS Monitor (Auto Calibration Always) ===");

  if (!ads.begin()) {
    Serial.println("ERROR: ADS1115 undetected !");
    while (1) delay(1000);
  }
  ads.setGain(GAIN_ONE);

  prefs.begin(PREF_NAMESPACE, false);

  // Forcer recalibration à chaque démarrage
  autoCalibration();

  // Mesure finale avec le nouveau facteur
  float measuredCurrent = measure_irms();
  Serial.print("Final calibration factor = ");
  Serial.println(calibrationFactor, 6);
  Serial.print("Measured IRMS = ");
  Serial.print(measuredCurrent, 6);
  Serial.println(" A");

  Serial.println("System ready.");

  t0_start = millis();  // enregistrer le temps zéro
}

float measure_irms_with_time(unsigned long &time_ms) {
    unsigned long t_start = millis();   // début mesure
    float irms = measure_irms();
    unsigned long t_end = millis();     // fin mesure
    time_ms = ((t_start + t_end) / 2) - t0_start;  // temps relatif depuis début
    return irms;
}

void loop() {
    static unsigned long lastMillis = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - lastMillis >= 2000) { // toutes les 2 secondes
        unsigned long measureTime;
        float i = measure_irms_with_time(measureTime);

        Serial.print("t = ");
        Serial.print(measureTime / 1000.0, 3); // secondes
        Serial.print(" s; ");
        Serial.print("I_rms = ");
        Serial.print(i, 5);
        Serial.println(" A");

        lastMillis = currentMillis;
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

  Serial.print("Line voltage = ");
  Serial.print(mainsVoltage_V);
  Serial.println(" V");

  Serial.print("Expected I_known = ");
  Serial.print(knownCurrent_I, 6);
  Serial.println(" A");

  Serial.println("Measuring Vrms...");
  float vrms = measure_vrms(SAMPLES);

  Serial.print("Measured Vrms = ");
  Serial.print(vrms * 1000.0f, 4);
  Serial.println(" mV");

  calibrationFactor = knownCurrent_I / vrms;

  // Sauvegarde quand même dans NVS si tu veux réutiliser
  prefs.putFloat(PREF_KEY, calibrationFactor);

  Serial.print("New calibration factor = ");
  Serial.println(calibrationFactor, 6);
  Serial.println("=== Auto Calibration Done ===");
}
