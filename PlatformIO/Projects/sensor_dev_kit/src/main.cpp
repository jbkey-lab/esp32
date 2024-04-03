// Created by Jacob Lamkey

#include <Arduino.h>
#include "LidarHelper.h" // Include the LiDAR helper file
#include "WifiHelper.h"

void setup() {

    // Start the serial communication:
    Serial.begin(115200);
    Serial2.begin(230400, SERIAL_8N1, 16, 17); // Baud rate and UART2 pins

    // Connect to Wi-Fi
    // manualWifiSetup();
    setupAccessPointAndServer();
    //Serial.print(lidarDataBuffer);

    autoConnectWifi();

    pinMode(relayPin, OUTPUT); // Set the relay pin as an output
    digitalWrite(relayPin, LOW); // Start with the relay off
    server.on("/toggleRelay", HTTP_GET, handleRelayToggle);
}

void loop() {

    server.handleClient();
   // handleLidarData();
  // if (WiFi.status() == WL_CONNECTED) {
        //delay(10000);
    // if (WiFi.status() != WL_CONNECTED) {
    //     Serial.println("WiFi disconnected. Reconnecting...");
    //     autoConnectWifi(); // Implement your WiFi connection logic here
    // }
    if (state == "on") {
        handleLidarData(); 
    }
        //Serial.println("Connecting to WiFi...");
    //}
    //delay(500);  // Delay of 1 second

}
