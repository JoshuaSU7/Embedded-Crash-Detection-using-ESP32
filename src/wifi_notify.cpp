#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// your network credentials
const char *SSID = "yourwifinetwork";
const char *PASSWORD = "yourwifipassword";

// your laptop's local IP and Flask port
// find your laptop IP by running `ifconfig` on Mac or `ipconfig` on Windows
const char *SERVER_URL = "yourip/crash";

void setupWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); // ensure clean state
}

void notifyCrash(float col_llh) {
    Serial.println("Crash detected — connecting to WiFi...");

    WiFi.begin(SSID, PASSWORD);

    // wait for connection, timeout after 10 seconds
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection failed, could not send crash notification");
        WiFi.disconnect(true);
        return;
    }

    Serial.println("WiFi connected, sending crash notification...");
    // Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    // Serial.printf("ESP32 IP: %s\n", WiFi.localIP().toString().c_str());

    HTTPClient http;
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");

    // build JSON payload
    StaticJsonDocument<64> doc;
    doc["col_llh"] = col_llh;
    String payload;
    serializeJson(doc, payload);

    int responseCode = http.POST(payload);

    if (responseCode > 0) {
        Serial.printf("Server responded with code: %d\n", responseCode);
    } else {
        Serial.printf("HTTP POST failed: %s\n", http.errorToString(responseCode).c_str());
    }

    http.end();
    WiFi.disconnect(true); // disconnect to save power
    Serial.println("WiFi disconnected");
}