// ================================================================
//  ESP32 Water Quality Monitor — Complete System
//  Sensors : pH (35) | TDS (34) | DS18B20 Temp (32) | Ultrasonic (5,18)
//  Outputs : Relay/Pump (26) | Digital In (27)
//  Features: WiFi GUI, Graphs, Auto Pump, Calibration, Suggestions
// ================================================================

#include <DallasTemperature.h>
#include <OneWire.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

const char *ssid = "GNXS-2.4G-BE7E20";
const char *password = "abcd1234";

// ---- PINS ----
#define TDS_PIN 34
#define PH_PIN 35
#define TEMP_PIN 32 // DS18B20 "To" from pH module
#define DO_PIN 27
#define TRIG_PIN 5   // Ultrasonic trigger
#define ECHO_PIN 18  // Ultrasonic echo
#define RELAY_PIN 26 // Relay → pump motor

#define VREF 3.3
#define ADC_RES 4095.0
#define SCOUNT 30

// ---- CALIBRATION (loaded from NVS) ----
float waterTemperature = 30.0;
float voltageAtPH7 = 1.65;
float voltageAtPH4 = 2.20;
float tdsCalFactor = 0.5;
float emptyDist = 30.0; // cm  distance when tank empty (sensor→bottom)
float fullDist = 4.0;   // cm  distance when tank full  (sensor→water surface)
float lowThreshold = 20.0;  // %   pump ON below this
float highThreshold = 80.0; // %   pump OFF above this

// ---- BUFFERS ----
int tdsBuffer[SCOUNT], phBuffer[SCOUNT];
int bufIndex = 0;

// ---- LIVE VALUES ----
float currentPH = 7.0, currentTDS = 0.0, currentTemp = 30.0;
float lastPhVoltage = 0.0, lastTdsVoltage = 0.0;
float waterLevel = 0.0, waterDistance = 0.0;
int digitalState = 0;

// ---- PUMP ----
bool pumpOn = false;
bool pumpAutoMode = true;

// ---- HISTORY (60 pts × 10 s = 10 min) ----
#define HIST 60
float hPH[HIST], hTDS[HIST], hTemp[HIST], hLvl[HIST];
int hIdx = 0, hCnt = 0;
unsigned long lastHist = 0;

// ---- OBJECTS ----
Preferences prefs;
WebServer server(80);
OneWire oneWire(TEMP_PIN);
DallasTemperature tempSensor(&oneWire);

// =============== MEDIAN FILTER ===============
int getMedianNum(int bArray[], int n) {
  int b[n];
  for (int i = 0; i < n; i++)
    b[i] = bArray[i];
  for (int j = 0; j < n - 1; j++)
    for (int i = 0; i < n - j - 1; i++)
      if (b[i] > b[i + 1]) {
        int t = b[i];
        b[i] = b[i + 1];
        b[i + 1] = t;
      }
  return (n & 1) ? b[(n - 1) / 2] : (b[n / 2] + b[n / 2 - 1]) / 2;
}

// =============== ULTRASONIC (multi-sample median) ===============
float singlePing() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long d = pulseIn(ECHO_PIN, HIGH, 30000);
  return (d == 0) ? -1.0 : d * 0.0343 / 2.0;
}
float readUltrasonic() {
  float r[7];
  int v = 0;
  for (int i = 0; i < 7; i++) {
    float d = singlePing();
    if (d > 0)
      r[v++] = d;
    delayMicroseconds(500);
  }
  if (v == 0)
    return -1;
  // bubble sort for median
  for (int i = 0; i < v - 1; i++)
    for (int j = i + 1; j < v; j++)
      if (r[i] > r[j]) {
        float t = r[i];
        r[i] = r[j];
        r[j] = t;
      }
  return r[v / 2];
}

// =============== HTML ===============
#include "page.h"

// =============== WEB HANDLERS ===============
void handleRoot() { server.send(200, "text/html", MAIN_PAGE); }

void handleData() {
  String j = "{\"temp\":" + String(currentTemp, 1) +
             ",\"ph\":" + String(currentPH, 2) +
             ",\"tds\":" + String(currentTDS, 0) +
             ",\"phV\":" + String(lastPhVoltage, 3) +
             ",\"tdsV\":" + String(lastTdsVoltage, 3) +
             ",\"lvl\":" + String(waterLevel, 1) +
             ",\"dist\":" + String(waterDistance, 1) +
             ",\"pump\":" + String(pumpOn ? "true" : "false") +
             ",\"pAuto\":" + String(pumpAutoMode ? "true" : "false") +
             ",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
  server.send(200, "application/json", j);
}

void handleHistory() {
  String j = "{\"d\":[";
  int s = (hCnt >= HIST) ? hIdx : 0;
  int c = min(hCnt, HIST);
  for (int i = 0; i < c; i++) {
    int x = (s + i) % HIST;
    if (i)
      j += ",";
    j += "[" + String(hPH[x], 1) + "," + String(hTDS[x], 0) + "," +
         String(hTemp[x], 1) + "," + String(hLvl[x], 0) + "]";
  }
  j += "]}";
  server.send(200, "application/json", j);
}

void handleCalGet() {
  String j = "{\"temp\":" + String(waterTemperature, 1) +
             ",\"ph7v\":" + String(voltageAtPH7, 3) +
             ",\"ph4v\":" + String(voltageAtPH4, 3) +
             ",\"tdscf\":" + String(tdsCalFactor, 2) +
             ",\"emD\":" + String(emptyDist, 1) +
             ",\"fuD\":" + String(fullDist, 1) +
             ",\"lowTh\":" + String(lowThreshold, 0) +
             ",\"highTh\":" + String(highThreshold, 0) + "}";
  server.send(200, "application/json", j);
}

void handleMeasure() {
  float d = readUltrasonic();
  server.send(200, "application/json", "{\"dist\":" + String(d, 1) + "}");
}

void handleCalSave() {
  if (server.hasArg("temp"))
    waterTemperature = server.arg("temp").toFloat();
  if (server.hasArg("ph7v"))
    voltageAtPH7 = server.arg("ph7v").toFloat();
  if (server.hasArg("ph4v"))
    voltageAtPH4 = server.arg("ph4v").toFloat();
  if (server.hasArg("tdscf"))
    tdsCalFactor = server.arg("tdscf").toFloat();
  if (server.hasArg("emD"))
    emptyDist = server.arg("emD").toFloat();
  if (server.hasArg("fuD"))
    fullDist = server.arg("fuD").toFloat();
  if (server.hasArg("lowTh"))
    lowThreshold = server.arg("lowTh").toFloat();
  if (server.hasArg("highTh"))
    highThreshold = server.arg("highTh").toFloat();
  prefs.putFloat("temp", waterTemperature);
  prefs.putFloat("ph7v", voltageAtPH7);
  prefs.putFloat("ph4v", voltageAtPH4);
  prefs.putFloat("tdscf", tdsCalFactor);
  prefs.putFloat("emD", emptyDist);
  prefs.putFloat("fuD", fullDist);
  prefs.putFloat("lowTh", lowThreshold);
  prefs.putFloat("highTh", highThreshold);
  server.send(200, "application/json", "{\"ok\":true}");
}

void handlePump() {
  if (server.hasArg("mode"))
    pumpAutoMode = (server.arg("mode") == "auto");
  if (server.hasArg("state") && !pumpAutoMode) {
    pumpOn = (server.arg("state") == "on");
    digitalWrite(RELAY_PIN, pumpOn ? HIGH : LOW);
  }
  String j = "{\"on\":" + String(pumpOn ? "true" : "false") +
             ",\"auto\":" + String(pumpAutoMode ? "true" : "false") + "}";
  server.send(200, "application/json", j);
}

// =============== SETUP ===============
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(TDS_PIN, INPUT);
  pinMode(PH_PIN, INPUT);
  pinMode(DO_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  tempSensor.begin();
  tempSensor.setResolution(12);
  tempSensor.setWaitForConversion(false);
  Serial.printf("DS18B20 found: %d\n", tempSensor.getDeviceCount());

  for (int i = 0; i < SCOUNT; i++) {
    tdsBuffer[i] = 0;
    phBuffer[i] = 0;
  }
  for (int i = 0; i < HIST; i++) {
    hPH[i] = 0;
    hTDS[i] = 0;
    hTemp[i] = 0;
    hLvl[i] = 0;
  }

  prefs.begin("wqm", false);
  waterTemperature = prefs.getFloat("temp", 30.0);
  voltageAtPH7 = prefs.getFloat("ph7v", 1.65);
  voltageAtPH4 = prefs.getFloat("ph4v", 2.20);
  tdsCalFactor = prefs.getFloat("tdscf", 0.5);
  emptyDist = prefs.getFloat("emD", 30.0);
  fullDist = prefs.getFloat("fuD", 4.0);
  lowThreshold = prefs.getFloat("lowTh", 20.0);
  highThreshold = prefs.getFloat("highTh", 80.0);
  currentTemp = waterTemperature;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("WiFi");
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 40) {
    delay(500);
    Serial.print(".");
    t++;
  }
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("\nIP: " + WiFi.localIP().toString());
  else
    Serial.println("\nWiFi failed");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/history", HTTP_GET, handleHistory);
  server.on("/calibrate", HTTP_GET, handleCalGet);
  server.on("/calibrate", HTTP_POST, handleCalSave);
  server.on("/pump", HTTP_POST, handlePump);
  server.on("/measure", HTTP_GET, handleMeasure);
  server.begin();
  Serial.println("Server OK\n");
}

// =============== LOOP ===============
void loop() {
  server.handleClient();

  static unsigned long st = millis();
  if (millis() - st > 40U) {
    st = millis();
    tdsBuffer[bufIndex] = analogRead(TDS_PIN);
    phBuffer[bufIndex] = analogRead(PH_PIN);
    bufIndex = (bufIndex + 1) % SCOUNT;
  }

  static unsigned long ct = millis();
  if (millis() - ct > 1000U) {
    ct = millis();

    // TDS
    int tb[SCOUNT];
    for (int i = 0; i < SCOUNT; i++)
      tb[i] = tdsBuffer[i];
    lastTdsVoltage = getMedianNum(tb, SCOUNT) * VREF / ADC_RES;
    float cc = 1.0 + 0.02 * (waterTemperature - 25.0);
    float cv = lastTdsVoltage / cc;
    currentTDS = (133.42 * pow(cv, 3) - 255.86 * pow(cv, 2) + 857.39 * cv) *
                 tdsCalFactor;

    // pH
    int pb[SCOUNT];
    for (int i = 0; i < SCOUNT; i++)
      pb[i] = phBuffer[i];
    lastPhVoltage = getMedianNum(pb, SCOUNT) * VREF / ADC_RES;
    float sl = (7.0 - 4.0) / (voltageAtPH7 - voltageAtPH4);
    currentPH = 7.0 + (lastPhVoltage - voltageAtPH7) * sl;
    currentPH = constrain(currentPH, 0.0f, 14.0f);

    // Temp
    tempSensor.requestTemperatures();
    float tv = tempSensor.getTempCByIndex(0);
    if (tv != DEVICE_DISCONNECTED_C && tv > -50 && tv < 100) {
      currentTemp = tv;
      waterTemperature = tv;
    }

    // Water level (two-point calibration)
    waterDistance = readUltrasonic();
    if (waterDistance > 0 && emptyDist > fullDist) {
      waterLevel =
          ((emptyDist - waterDistance) / (emptyDist - fullDist)) * 100.0;
      waterLevel = constrain(waterLevel, 0.0f, 100.0f);
    }

    digitalState = digitalRead(DO_PIN);

    // Auto pump
    if (pumpAutoMode) {
      if (waterLevel < lowThreshold && !pumpOn) {
        pumpOn = true;
        digitalWrite(RELAY_PIN, HIGH);
        Serial.println("PUMP ON");
      } else if (waterLevel > highThreshold && pumpOn) {
        pumpOn = false;
        digitalWrite(RELAY_PIN, LOW);
        Serial.println("PUMP OFF");
      }
    }

    Serial.printf("pH=%.2f TDS=%.0f T=%.1f Lvl=%.0f%% Pump=%s\n", currentPH,
                  currentTDS, currentTemp, waterLevel, pumpOn ? "ON" : "OFF");
  }

  // History every 10s
  if (millis() - lastHist > 10000U) {
    lastHist = millis();
    hPH[hIdx] = currentPH;
    hTDS[hIdx] = currentTDS;
    hTemp[hIdx] = currentTemp;
    hLvl[hIdx] = waterLevel;
    hIdx = (hIdx + 1) % HIST;
    if (hCnt < HIST)
      hCnt++;
  }
}
