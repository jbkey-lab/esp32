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


    //autoConnectWifi();

    pinMode(relayPin, OUTPUT); // Set the relay pin as an output
    digitalWrite(relayPin, LOW); // Start with the relay off
    //server.on("/toggleRelay", HTTP_GET, handleRelayToggle);
    //server.begin();
}

void loop() {

    server.handleClient();
   // if (state == "on") {
        handleLidarData();    
    // if (isConnected && WiFi.status() == WL_CONNECTED) {
    //     Serial.println("Connected to new Wi-Fi");
    //     Serial.print("IP Address: ");
    //     Serial.println(WiFi.localIP());
    //     WiFi.softAPdisconnect(true);
    //     Serial.println("Access Point Stopped");
    //     isConnected = false; // Reset the flag
    // }

    if (delayRunning && (millis() - delayStart >= 100000)) {
        delayRunning = false;
        // Delay has finished, perform any actions that should happen after the delay
        if (WiFi.status() == WL_CONNECTED) {

            WiFi.softAPdisconnect(true);
            Serial.println("Access Point Stopped");
        };

    }

        //delay(500);
   // } 
}


   // handleLidarData();
  // if (WiFi.status() == WL_CONNECTED) {
        //delay(10000);
    // if (WiFi.status() != WL_CONNECTED) {
    //     Serial.println("WiFi disconnected. Reconnecting...");
    //     autoConnectWifi(); // Implement your WiFi connection logic here
    // }
            //Serial.println("Connecting to WiFi...");
    //}
    //delay(500);  // Delay of 1 second
