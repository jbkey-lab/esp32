// Created by Jacob Lamkey

#include <Arduino.h>
#include "LidarHelper.h" // Include the LiDAR helper file
#include "WifiHelper.h"
#include "SensorHelper.h"

void setup() {

    // Start the serial communication:
    Serial.begin(115200);
    Serial2.begin(230400, SERIAL_8N1, 16, 17); // Baud rate and UART2 pins
    sensors.begin(); 

    pinMode(flowPin, INPUT_PULLUP); // Set flow sensor pin as input
    pinMode(relayPin, OUTPUT); // Set the relay pin as an output
    pinMode(hunterPin, OUTPUT); // Set relay pin as output
    pinMode(digitalPin, INPUT);

    attachInterrupt(digitalPinToInterrupt(flowPin), pulseCounter, RISING); // Interrupt on rising edge
    flowMillis = millis();
    // Connect to Wi-Fi
    setupAccessPointAndServer();

    digitalWrite(relayPin, LOW); // Start with the relay off
}

void loop() {

    server.handleClient();
    handleLidarData();    
    waterFlowSensor();

    if (delayRunning && (millis() - delayStart >= 100000)) {
        delayRunning = false;
        // Delay has finished, perform any actions that should happen after the delay
        if (WiFi.status() == WL_CONNECTED) {

            WiFi.softAPdisconnect(true);
            Serial.println("Access Point Stopped");
        };
    }
}
