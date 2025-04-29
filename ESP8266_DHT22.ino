// ESP8266 DHT22 Sensor with Web UI, NTP (Germany + DST), WiFiManager, Persistent Config, and MQTT Home Assistant Integration by Jim (MiMusik.com)
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <time.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

AsyncWebServer server(80);
DNSServer dns;
Ticker resetTicker;
AsyncMqttClient mqttClient;

struct Config {
  char dhtPin[4] = "0";
  char deviceName[32] = "esp8266_sensor";
  int offsetMinute = 0;
  char mqttServer[64] = "";
  char mqttUser[32] = "";
  char mqttPass[32] = "";
} config;

DHT* dht = nullptr;
float temperature = NAN;
float humidity = NAN;
float heatIndex = NAN;
time_t lastSensorReadTime = 0;
unsigned long lastMinute = 0;

const char* configPath = "/config.json";

void saveConfig() {
  File f = LittleFS.open(configPath, "w");
  if (!f) return;
  StaticJsonDocument<512> doc;
  doc["dhtPin"] = config.dhtPin; 
  doc["deviceName"] = config.deviceName;
  doc["offsetMinute"] = config.offsetMinute; // reads sensor at HH:+offset (hourly reading of data, to be adjustable in a later version)
  doc["mqttServer"] = config.mqttServer;
  doc["mqttUser"] = config.mqttUser;
  doc["mqttPass"] = config.mqttPass;
  serializeJson(doc, f);
  f.close();
}

void loadConfig() {
  if (!LittleFS.exists(configPath)) return;
  File f = LittleFS.open(configPath, "r");
  if (!f) return;
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, f)) return;
  strlcpy(config.dhtPin, doc["dhtPin"] | "0", sizeof(config.dhtPin));
  strlcpy(config.deviceName, doc["deviceName"] | "esp8266_sensor", sizeof(config.deviceName));
  config.offsetMinute = doc["offsetMinute"] | 0;
  strlcpy(config.mqttServer, doc["mqttServer"] | "", sizeof(config.mqttServer));
  strlcpy(config.mqttUser, doc["mqttUser"] | "", sizeof(config.mqttUser));
  strlcpy(config.mqttPass, doc["mqttPass"] | "", sizeof(config.mqttPass));
  f.close();
}

void connectToMqtt() {
  if (strlen(config.mqttServer) > 0) {
    mqttClient.setServer(config.mqttServer, 1883);
    mqttClient.setKeepAlive(4000);
    if (strlen(config.mqttUser) > 0) {
      mqttClient.setCredentials(config.mqttUser, config.mqttPass);
    }
    mqttClient.connect();
  }
}

void publishSensorData() {
  if (!mqttClient.connected()) return;
  String base = "homeassistant/sensor/" + String(config.deviceName);
  String dev = "\"name\":\"" + String(config.deviceName) + "\",\"identifiers\":[\"" + config.deviceName + "\"]";

  auto pub = [&](const char* name, float value, const char* unit, const char* devclass) {
    String topic = base + "/" + name + "/config";
    String value_topic = base + "/" + name + "/state";
    String payload = "{\"device_class\":\"" + String(devclass) +
      "\",\"unit_of_measurement\":\"" + unit +
      "\",\"name\":\"" + name +
      "\",\"state_topic\":\"" + value_topic +
      "\",\"unique_id\":\"" + config.deviceName + "_" + name +
      "\",\"device\":{" + dev + "}}";
    mqttClient.publish(topic.c_str(), 1, true, payload.c_str());
    mqttClient.publish(value_topic.c_str(), 1, true, String(value, 1).c_str()); // <-- 1 decimal
  };

  if (!isnan(temperature)) pub("temperature", temperature, "°C", "temperature");
  if (!isnan(humidity)) pub("humidity", humidity, "%", "humidity");
  if (!isnan(heatIndex)) pub("heat_index", heatIndex, "°C", "temperature");
}

void setupTime() {
  //ntp servers (to be moved to the config at a later date)
  configTime(3600, 3600, "0.de.pool.ntp.org", "1.de.pool.ntp.org");
  //ntp timezone (to be moved to the config at a later date)
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
}

String getFormattedTime() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buf[32];
  strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y", timeinfo);
  return String(buf);
}

String getLastReadTimeString() {
  if (lastSensorReadTime == 0) return "Never";
  struct tm* timeinfo = localtime(&lastSensorReadTime);
  char buf[32];
  strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y", timeinfo);
  return String(buf);
}

int getCurrentMinute() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  return timeinfo->tm_min;
}

void readSensor() {
  if (!dht) return;
  humidity = dht->readHumidity();
  temperature = dht->readTemperature();
  if (!isnan(temperature) && !isnan(humidity)) {
    heatIndex = dht->computeHeatIndex(temperature, humidity, false);
    lastSensorReadTime = time(nullptr);
    publishSensorData();
  }
}

void doReset() {
  LittleFS.remove(configPath);
  ESP.eraseConfig();
  ESP.restart();
}

void deepReset() {
  resetTicker.once(1.5, doReset);
}

void setupWeb() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>" + String(config.deviceName) + " Climate Monitor</title></head><body><h2>ESP8266 " + String(config.deviceName) + " Sensor Status</h2>";
    html += "<p>Device: " + String(config.deviceName) + "</p>";
    html += "<p>WiFi: " + WiFi.localIP().toString() + "</p>";
    html += "<p>NTP Time: " + getFormattedTime() + "</p>";
    html += "<p>DHT GPIO: " + String(config.dhtPin) + "</p>";
    html += "<p>Offset Minute: " + String(config.offsetMinute) + "</p>";
    html += "<p>Temperature: " + (isnan(temperature) ? "Waiting..." : String(temperature, 1) + " °C") + "</p>"; // <-- 1 decimal
    html += "<p>Humidity: " + (isnan(humidity) ? "Waiting..." : String(humidity, 1) + " %") + "</p>"; // <-- 1 decimal
    html += "<p>Heat Index: " + (isnan(heatIndex) ? "Waiting..." : String(heatIndex, 1) + " °C") + "</p>"; // <-- 1 decimal
    html += "<p>Last Sensor Read: " + getLastReadTimeString() + "</p>";
    html += "<form action=\"/deepreset\" method=\"POST\"><button type=\"submit\">Deep Reset</button></form>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/deepreset", HTTP_POST, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Device is resetting...");
    deepReset();
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  system_update_cpu_freq(SYS_CPU_80MHZ);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  LittleFS.begin();
  loadConfig();

  AsyncWiFiManager wm(&server, &dns);
  AsyncWiFiManagerParameter custom_device("device", "Device Name", config.deviceName, sizeof(config.deviceName));
  AsyncWiFiManagerParameter custom_dhtpin("dht", "DHT GPIO", config.dhtPin, sizeof(config.dhtPin));
  char offsetBuf[4];
  snprintf(offsetBuf, sizeof(offsetBuf), "%d", config.offsetMinute);
  AsyncWiFiManagerParameter custom_offset("offset", "Offset Minute", offsetBuf, sizeof(offsetBuf));
  AsyncWiFiManagerParameter custom_mqtt("mqtt", "MQTT Server", config.mqttServer, sizeof(config.mqttServer));
  AsyncWiFiManagerParameter custom_user("user", "MQTT Username", config.mqttUser, sizeof(config.mqttUser));
  AsyncWiFiManagerParameter custom_pass("pass", "MQTT Password", config.mqttPass, sizeof(config.mqttPass));

  wm.addParameter(&custom_device);
  wm.addParameter(&custom_dhtpin);
  wm.addParameter(&custom_offset);
  wm.addParameter(&custom_mqtt);
  wm.addParameter(&custom_user);
  wm.addParameter(&custom_pass);

  if (!wm.autoConnect("ESP8266_Config")) {
    ESP.restart();
  }

  strlcpy(config.deviceName, custom_device.getValue(), sizeof(config.deviceName));
  strlcpy(config.dhtPin, custom_dhtpin.getValue(), sizeof(config.dhtPin));
  config.offsetMinute = atoi(custom_offset.getValue());
  strlcpy(config.mqttServer, custom_mqtt.getValue(), sizeof(config.mqttServer));
  strlcpy(config.mqttUser, custom_user.getValue(), sizeof(config.mqttUser));
  strlcpy(config.mqttPass, custom_pass.getValue(), sizeof(config.mqttPass));
  saveConfig();

  int dhtGpio = atoi(config.dhtPin);
  dht = new DHT(dhtGpio, DHT22);
  dht->begin();

  setupTime();
  setupWeb();
  connectToMqtt();
  delay(60000);
  readSensor();
}

unsigned long lastReconnectAttempt = 0;
unsigned long mqttReconnectDelay = 5000; // Start at 5s

void loop() {
  int nowMin = getCurrentMinute();
  if (nowMin != lastMinute) {
    lastMinute = nowMin;
    if (nowMin == config.offsetMinute) {
      readSensor();
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, rebooting...");
    delay(3000);
    ESP.restart();
  }

  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > mqttReconnectDelay) {
      lastReconnectAttempt = now;
      connectToMqtt();
      mqttReconnectDelay = min(mqttReconnectDelay * 2, 60000UL); // Exponential backoff to max 1 min
    }
  } else {
    mqttReconnectDelay = 5000; // Reset backoff if connected
  }
}
