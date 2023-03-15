#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "defines.h"
#include "settings.h"

void mqttDataCallback(char* topic, byte* payload, unsigned int length);

int red = 0;
int green = 0;
int blue = 0;

WiFiClient wifiClient;
PubSubClient mqtt;

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println("\n\r=== Setup ===");

  Serial.println("Configuring WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.hostname(WIFI_HOSTNAME);
  Serial.printf("  MAC:         %s\n\r", WiFi.macAddress().c_str());
  Serial.printf("  SSID:        %s\n\r", WIFI_SSID);
  Serial.printf("  Hostname:    %s\n\r", WiFi.hostname());
  Serial.print("Trying to connect to WiFi");
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n\rWiFi Connected!");
  Serial.printf("  IP address: %s\n\r", WiFi.localIP().toString().c_str());
  Serial.printf("  RRSI:       %d\n\r", WiFi.RSSI());

  Serial.println("Configuring MQTT");
  mqtt.setClient(wifiClient);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttDataCallback);
  Serial.printf("  Host:        %s\n\r", MQTT_HOST);
  Serial.printf("  Port:        %d\n\r", MQTT_PORT);
  Serial.printf("  Client ID:   %s\n\r", MQTT_CLIENT_ID);
  Serial.printf("  Username:    %s\n\r", MQTT_USERNAME);
  Serial.printf("  Buffer Size: %d\n\r", mqtt.getBufferSize());

  Serial.println("Configuring OTA");
  ArduinoOTA.setHostname(WIFI_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    Serial.printf("Start updating %s\n\r", type);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\n\rEnd");
  });
  ArduinoOTA.begin();
  Serial.println("Setup Completed");
}

void loop() {
  ArduinoOTA.handle();

  if (WiFi.status() == WL_CONNECTED) {
    if (mqtt.connected()) {
      mqtt.loop();
    } else {
      if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD, MQTT_WILL_TOPIC, 1, 0, MQTT_WILL_MESSAGE)) {
        Serial.println("MQTT Connected");
        Serial.println("Subscribing to topics");
        Serial.printf("  %s\n\r", MQTT_COMMAND_TOPIC);
        mqtt.subscribe(MQTT_COMMAND_TOPIC);
        mqtt.publish(MQTT_WILL_TOPIC, "online");
      }
    }
  }
}

void mqttDataCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String strPayload = String((char*)payload);
  Serial.println("=== MQTT Message Received! ===");
  Serial.printf("  Topic:       %s\n\r", topic);
  Serial.println("  Payload:");
  Serial.println(strPayload);

  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("deserializeJson() failed with code %s\r\n", err.f_str());
    return;
  }
  if(doc.containsKey(RED_KEY)) {
    red = String(doc[RED_KEY]).toInt();
    Serial.printf("Red value received: %d\n\r", red);
    analogWrite(RED_PIN, MAX_PWM - red);
  }
  if(doc.containsKey(GREEN_KEY)) {
    green = String(doc[GREEN_KEY]).toInt();
    Serial.printf("Green value received: %d\n\r", green);
    analogWrite(GREEN_PIN, MAX_PWM - green);
  }
  if(doc.containsKey(BLUE_KEY)) {
    blue = String(doc[BLUE_KEY]).toInt();
    Serial.printf("Blue value received: %d\n\r", blue);
    analogWrite(BLUE_PIN, MAX_PWM - blue);
  }
}
