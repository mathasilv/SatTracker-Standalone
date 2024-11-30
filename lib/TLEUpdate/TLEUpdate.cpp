// File: lib/TLEUpdate/TLEUpdate.cpp

#include "TLEUpdate.h"
#include <SPIFFS.h>
#include <WiFi.h>

std::vector<SatelliteData> satellites;
int numSatellites = 0;

void loadTLEs() {
    File file = SPIFFS.open("/data/tle.txt", "w");
   if (file.size() == 0) {
    Serial.println("TLE file is empty");
    file.close();
    return;
}


    satellites.clear();
    while (file.available()) {
        String name = file.readStringUntil('\n');
        String line1 = file.readStringUntil('\n');
        String line2 = file.readStringUntil('\n');

        // Remove any carriage return characters
        name.trim();
        line1.trim();
        line2.trim();

        if (name.length() > 0 && line1.length() > 0 && line2.length() > 0) {
            SatelliteData satData = {name, line1, line2};
            satellites.push_back(satData);
        }
    }
    file.close();
    numSatellites = satellites.size();
    Serial.println("TLEs loaded successfully");
}

void updateTLEs(const char* host, const char* path) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi not connected");
        return;
    }

    WiFiClient client;

    Serial.print("Connecting to ");
    Serial.println(host);

    if (!client.connect(host, 80)) {
        Serial.println("Connection failed");
        return;
    }

    // Send HTTP GET request
    client.print(String("GET ") + path + " HTTP/1.0\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

    // Wait for response
    while (!client.available()) {
        delay(100);
    }

    // Read headers and skip them
    while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            break; // Headers ended
        }
    }

    // Open file for writing
    File file = SPIFFS.open("/tle.txt", "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    // Read the response and write to file
    while (client.available()) {
        String line = client.readStringUntil('\n');
        file.println(line);
    }

    file.close();
    client.stop();
    Serial.println("TLEs updated successfully");

    // Reload TLEs into memory
    loadTLEs();
}
