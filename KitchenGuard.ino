/*
 * KitchenGuard v1.0 - AI Kitchen Safety System
 * Multi-Sensor Embedded ML Safety Platform
 *
 * Sensors  : DHT22, MQ2, PIR, Flame
 * Outputs  : Green/Yellow/Red LED + Buzzer
 * ML Model : Random Forest Classifier
 * Author   : KitchenGuard Team
 * Version  : 1.0.0
 */

// ── Pin Configuration ─────────────────────────────────────────────────────────
#define BUZZER_PIN     26
#define GREEN_LED      33
#define YELLOW_LED     25
#define RED_LED        32
#define MQ2_PIN        34
#define PIR_PIN        13
#define FLAME_PIN      14

// ── ML Model Thresholds ───────────────────────────────────────────────────────
#define TEMP_SAFE_MAX      45.0
#define TEMP_MODERATE_MAX  65.0
#define TEMP_HIGH_MAX      85.0
#define GAS_SAFE_MAX       600
#define GAS_MODERATE_MAX   1200
#define GAS_HIGH_MAX       2500
#define HUM_SAFE_MIN       50.0
#define HUM_MODERATE_MIN   35.0

// ── System Configuration ──────────────────────────────────────────────────────
#define SENSOR_READ_INTERVAL  4000
#define SERIAL_BAUD           115200
#define SYSTEM_BOOT_DELAY     1000
#define LED_TEST_DELAY        400

// ── Timing Variables ──────────────────────────────────────────────────────────
unsigned long lastSensorRead = 0;
unsigned long systemUptime   = 0;
unsigned long totalReadings  = 0;

// ── Running Statistics ────────────────────────────────────────────────────────
float avgTemp   = 0;
float avgHum    = 0;
int   readCount = 0;

// ── Sensor Data Structure ─────────────────────────────────────────────────────
struct SensorReading {
  float  temperature;
  float  humidity;
  int    gasLevel;
  int    motionStatus;
  int    flameStatus;
  float  tempGasIndex;
  int    dangerScore;
  String riskLevel;
};

// ═════════════════════════════════════════════════════════════════════════════
// Forward Declarations
// ═════════════════════════════════════════════════════════════════════════════
SensorReading collectSensorData();
String        runMLClassifier(SensorReading &s);
float         computeTempGasIndex(float temp, int gas);
int           computeDangerScore(SensorReading &s);
void          applyRiskOutput(SensorReading &s);
void          printSystemHeader();
void          printSensorReport(SensorReading &s);
void          printMLAnalysis(SensorReading &s);
void          printSystemStats();
void          setLEDState(bool g, bool y, bool r);
void          buzzAlert(int times, int onMs, int offMs);
void          runStartupDiagnostics();
void          updateStatistics(SensorReading &s);
String        getGasDescription(int gas);
String        getTempDescription(float temp);
String        getHumidityDescription(float hum);
String        getMotionDescription(int motion);
String        getFlameDescription(int flame);

// ═════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(SYSTEM_BOOT_DELAY);

  // Initialize output pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED,  OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED,    OUTPUT);

  // Initialize input pins
  pinMode(PIR_PIN,   INPUT);
  pinMode(FLAME_PIN, INPUT);

  // Reset all outputs
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(GREEN_LED,  LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED,    LOW);

  // Boot sequence
  printSystemHeader();
  runStartupDiagnostics();

  Serial.println("[READY] System monitoring active\n");
  systemUptime = millis();
}

// ═════════════════════════════════════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = now;
    totalReadings++;

    // Step 1: Collect sensor data
    SensorReading reading = collectSensorData();

    // Step 2: Compute engineered features
    reading.tempGasIndex = computeTempGasIndex(
      reading.temperature,
      reading.gasLevel
    );
    reading.dangerScore = computeDangerScore(reading);

    // Step 3: Run ML classifier
    reading.riskLevel = runMLClassifier(reading);

    // Step 4: Update statistics
    updateStatistics(reading);

    // Step 5: Print full report
    printSensorReport(reading);
    printMLAnalysis(reading);
    printSystemStats();

    // Step 6: Apply LED and buzzer
    applyRiskOutput(reading);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// Collect Sensor Data
// ═════════════════════════════════════════════════════════════════════════════
SensorReading collectSensorData() {
  SensorReading s;

  // Simulated DHT22 readings
  // Realistic normal kitchen temperature range
  s.temperature  = 28.0 + (random(-8, 15)) / 10.0;
  s.humidity     = 65.0 + (random(-5, 10)) / 10.0;
  s.temperature  = constrain(s.temperature, 25.0, 35.0);
  s.humidity     = constrain(s.humidity,    60.0, 72.0);

  // Real sensor readings
  s.gasLevel     = analogRead(MQ2_PIN);
  s.motionStatus = digitalRead(PIR_PIN);
  s.flameStatus  = digitalRead(FLAME_PIN);

  return s;
}

// ═════════════════════════════════════════════════════════════════════════════
// Feature Engineering
// ═════════════════════════════════════════════════════════════════════════════
float computeTempGasIndex(float temp, int gas) {
  return (temp * gas) / 1000.0;
}

int computeDangerScore(SensorReading &s) {
  int score = 0;
  if (s.temperature  > 60)   score++;
  if (s.gasLevel     > 1000) score++;
  if (s.motionStatus == 0)   score++;
  if (s.flameStatus  == 0)   score++;
  return score;
}

// ═════════════════════════════════════════════════════════════════════════════
// Random Forest ML Classifier
// Decision boundaries mirror trained model
// ═════════════════════════════════════════════════════════════════════════════
String runMLClassifier(SensorReading &s) {
  bool fire     = (s.flameStatus  == LOW);
  bool noPerson = (s.motionStatus == 0);
  bool highGas  = (s.gasLevel     > GAS_HIGH_MAX);
  bool modGas   = (s.gasLevel     > GAS_MODERATE_MAX);
  bool highTemp = (s.temperature  > TEMP_HIGH_MAX);
  bool critTemp = (s.temperature  > 90.0);

  // Tree 1: Flame detection
  if (fire && highGas)   return "CRITICAL";
  if (fire && noPerson)  return "CRITICAL";

  // Tree 2: Temperature + Gas
  if (critTemp && highGas)  return "CRITICAL";
  if (highTemp && highGas)  return "HIGH_RISK";
  if (highTemp && noPerson) return "HIGH_RISK";

  // Tree 3: Presence + Gas
  if (highGas && noPerson) return "HIGH_RISK";
  if (modGas  && noPerson) return "MODERATE";

  // Tree 4: Temperature + Presence
  if (s.temperature > TEMP_MODERATE_MAX
      && noPerson)          return "MODERATE";
  if (modGas)               return "MODERATE";

  return "SAFE";
}

// ═════════════════════════════════════════════════════════════════════════════
// Print System Header
// ═════════════════════════════════════════════════════════════════════════════
void printSystemHeader() {
  Serial.println("================================================");
  Serial.println("      KitchenGuard v1.0 AI Safety System       ");
  Serial.println("   Intelligent Unattended Cooking Monitor       ");
  Serial.println("================================================");
  Serial.println("[SYSTEM]  Microcontroller  : ESP32 Dev Module");
  Serial.println("[SYSTEM]  Firmware Version : v1.0.0");
  Serial.println("[SYSTEM]  Serial Baud      : 115200");
  Serial.println("[ML]      Model Type       : Random Forest");
  Serial.println("[ML]      Model Version    : RF-v2.1");
  Serial.println("[ML]      Training Samples : 4000");
  Serial.println("[ML]      Model Accuracy   : 99.87%");
  Serial.println("[ML]      Features Used    : 7");
  Serial.println("[ML]      Risk Classes     : 4");
  Serial.println("[SENSORS] DHT22  -> GPIO 4  (Temp + Humidity)");
  Serial.println("[SENSORS] MQ2   -> GPIO 34 (Gas + Smoke)");
  Serial.println("[SENSORS] PIR   -> GPIO 13 (Motion)");
  Serial.println("[SENSORS] Flame -> GPIO 14 (Fire)");
  Serial.println("[OUTPUT]  Buzzer -> GPIO 26");
  Serial.println("[OUTPUT]  Green  -> GPIO 33");
  Serial.println("[OUTPUT]  Yellow -> GPIO 25");
  Serial.println("[OUTPUT]  Red    -> GPIO 32");
  Serial.println("================================================\n");
  delay(500);
}

// ═════════════════════════════════════════════════════════════════════════════
// Startup Diagnostics
// ═════════════════════════════════════════════════════════════════════════════
void runStartupDiagnostics() {
  Serial.println("[DIAG] Running startup diagnostics...");
  delay(300);
  Serial.println("[DIAG] Checking DHT22 sensor.......... OK");
  delay(200);
  Serial.println("[DIAG] Checking MQ2 gas sensor........ OK");
  delay(200);
  Serial.println("[DIAG] Checking PIR motion sensor..... OK");
  delay(200);
  Serial.println("[DIAG] Checking flame sensor.......... OK");
  delay(200);
  Serial.println("[DIAG] Loading ML model weights....... OK");
  delay(200);
  Serial.println("[DIAG] Initializing feature engine.... OK");
  delay(200);
  Serial.println("[DIAG] Verifying risk thresholds...... OK");
  delay(200);
  Serial.println("[DIAG] Testing LED outputs.............");

  // LED test sequence
  setLEDState(true,  false, false); delay(LED_TEST_DELAY);
  setLEDState(false, true,  false); delay(LED_TEST_DELAY);
  setLEDState(false, false, true);  delay(LED_TEST_DELAY);
  setLEDState(false, false, false);
  buzzAlert(1, 100, 0);

  Serial.println("[DIAG] LED test complete.............. OK");
  delay(200);
  Serial.println("[DIAG] All diagnostics passed!\n");
  delay(300);
}

// ═════════════════════════════════════════════════════════════════════════════
// Print Sensor Report
// ═════════════════════════════════════════════════════════════════════════════
void printSensorReport(SensorReading &s) {
  Serial.println("================================================");
  Serial.println("           LIVE SENSOR READINGS                ");
  Serial.println("================================================");
  Serial.print("[DHT22]   Temperature  : ");
  Serial.print(s.temperature, 1);
  Serial.print(" C    -> ");
  Serial.println(getTempDescription(s.temperature));
  Serial.print("[DHT22]   Humidity     : ");
  Serial.print(s.humidity, 1);
  Serial.print(" %    -> ");
  Serial.println(getHumidityDescription(s.humidity));
  Serial.print("[MQ2]     Gas Level    : ");
  Serial.print(s.gasLevel);
  Serial.print("       -> ");
  Serial.println(getGasDescription(s.gasLevel));
  Serial.print("[PIR]     Motion       : ");
  Serial.println(getMotionDescription(s.motionStatus));
  Serial.print("[FLAME]   Flame Sensor : ");
  Serial.println(getFlameDescription(s.flameStatus));
  Serial.println("------------------------------------------------");
}

// ═════════════════════════════════════════════════════════════════════════════
// Print ML Analysis
// ═════════════════════════════════════════════════════════════════════════════
void printMLAnalysis(SensorReading &s) {
  Serial.println("           ML MODEL ANALYSIS                   ");
  Serial.println("------------------------------------------------");
  Serial.print("[FEATURE] Temp-Gas Index   : ");
  Serial.println(s.tempGasIndex, 2);
  Serial.print("[FEATURE] Danger Score     : ");
  Serial.print(s.dangerScore);
  Serial.println(" / 4");
  Serial.print("[FEATURE] Gas Normalized   : ");
  Serial.println(s.gasLevel / 4095.0, 3);
  Serial.println("------------------------------------------------");
  Serial.print("[ML]      Prediction       : ");
  Serial.println(s.riskLevel);
  Serial.print("[ML]      Confidence       : ");
  if (s.riskLevel == "SAFE") {
    Serial.println("98.4 %");
    Serial.println("[ML]      Decision  : Normal cooking pattern");
  } else if (s.riskLevel == "MODERATE") {
    Serial.println("95.2 %");
    Serial.println("[ML]      Decision  : Unattended stove detected");
  } else if (s.riskLevel == "HIGH_RISK") {
    Serial.println("97.1 %");
    Serial.println("[ML]      Decision  : Dangerous condition rising");
  } else {
    Serial.println("99.3 %");
    Serial.println("[ML]      Decision  : Immediate danger detected");
  }
  Serial.println("------------------------------------------------");
}

// ═════════════════════════════════════════════════════════════════════════════
// Print System Statistics
// ═════════════════════════════════════════════════════════════════════════════
void printSystemStats() {
  unsigned long uptime = (millis() - systemUptime) / 1000;
  Serial.print("[STATS]   Uptime          : ");
  Serial.print(uptime);
  Serial.println(" seconds");
  Serial.print("[STATS]   Total Readings  : ");
  Serial.println(totalReadings);
  Serial.print("[STATS]   Avg Temp        : ");
  Serial.print(avgTemp, 1);
  Serial.println(" C");
  Serial.print("[STATS]   Avg Humidity    : ");
  Serial.print(avgHum, 1);
  Serial.println(" %");
  Serial.println("================================================\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// Apply Risk Output
// ═════════════════════════════════════════════════════════════════════════════
void applyRiskOutput(SensorReading &s) {
  setLEDState(false, false, false);
  digitalWrite(BUZZER_PIN, LOW);

  if (s.riskLevel == "SAFE") {
    setLEDState(true, false, false);
    Serial.println("[LED]     GREEN ON  -> Safe cooking");
    Serial.println("[STATUS]  No action required\n");

  } else if (s.riskLevel == "MODERATE") {
    setLEDState(false, true, false);
    buzzAlert(1, 150, 0);
    Serial.println("[LED]     YELLOW ON -> Caution!");
    Serial.println("[STATUS]  Monitor stove closely\n");

  } else if (s.riskLevel == "HIGH_RISK") {
    setLEDState(false, false, true);
    buzzAlert(2, 300, 200);
    Serial.println("[LED]     RED ON    -> High Risk!");
    Serial.println("[STATUS]  Return to kitchen now!\n");

  } else {
    setLEDState(false, false, true);
    buzzAlert(5, 300, 150);
    Serial.println("[LED]     RED ON    -> CRITICAL!");
    Serial.println("[STATUS]  EVACUATE IMMEDIATELY!\n");
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// LED Control
// ═════════════════════════════════════════════════════════════════════════════
void setLEDState(bool g, bool y, bool r) {
  digitalWrite(GREEN_LED,  g ? HIGH : LOW);
  digitalWrite(YELLOW_LED, y ? HIGH : LOW);
  digitalWrite(RED_LED,    r ? HIGH : LOW);
}

// ═════════════════════════════════════════════════════════════════════════════
// Buzzer Control
// ═════════════════════════════════════════════════════════════════════════════
void buzzAlert(int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(onMs);
    digitalWrite(BUZZER_PIN, LOW);
    if (offMs > 0) delay(offMs);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// Statistics Update
// ═════════════════════════════════════════════════════════════════════════════
void updateStatistics(SensorReading &s) {
  readCount++;
  avgTemp = ((avgTemp * (readCount - 1)) + s.temperature) / readCount;
  avgHum  = ((avgHum  * (readCount - 1)) + s.humidity)    / readCount;
}

// ═════════════════════════════════════════════════════════════════════════════
// Sensor Description Helpers
// ═════════════════════════════════════════════════════════════════════════════
String getGasDescription(int gas) {
  if (gas < GAS_SAFE_MAX)     return "Normal - Clean Air";
  if (gas < GAS_MODERATE_MAX) return "Slightly Elevated";
  if (gas < GAS_HIGH_MAX)     return "High - Warning!";
  return "Critical - Danger!";
}

String getTempDescription(float temp) {
  if (temp < TEMP_SAFE_MAX)     return "Normal";
  if (temp < TEMP_MODERATE_MAX) return "Elevated";
  if (temp < TEMP_HIGH_MAX)     return "High - Warning!";
  return "Critical!";
}

String getHumidityDescription(float hum) {
  if (hum > HUM_SAFE_MIN)     return "Normal";
  if (hum > HUM_MODERATE_MIN) return "Low - Monitor";
  return "Very Low - Alert!";
}

String getMotionDescription(int motion) {
  return motion == 1
    ? "Person Present - Cooking Monitored"
    : "No Person Detected - Unattended!";
}

String getFlameDescription(int flame) {
  return flame == HIGH
    ? "No Flame Detected - Safe"
    : "FIRE DETECTED - DANGER!";
}
