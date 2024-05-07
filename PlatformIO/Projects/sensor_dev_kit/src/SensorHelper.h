// WifiHelper.h
#ifndef SENSOR_HELPER_H
#define SENSOR_HELPER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "LidarHelper.h"
#include <Preferences.h>
#include <WiFiUdp.h>
#include <AsyncUDP.h>
#include "WifiHelper.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "LidarHelper.h"

WebServer server(80);

// ------------------- Temp Sensor ----------------------------
#define ONE_WIRE_BUS 4 // Example pin; adjust as needed

// Create an instance of the OneWire library
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to the DallasTemperature library
DallasTemperature sensors(&oneWire);

void tempSensor() {
  sensors.requestTemperatures(); // Request temperature readings

  float temperatureC = sensors.getTempCByIndex(0); // Get the first sensor's temperature in Celsius
  float temperatureF = sensors.getTempFByIndex(0); // Get the first sensor's temperature in Fahrenheit

  if (temperatureC != DEVICE_DISCONNECTED_C) { // Check for valid reading
    Serial.print("Temperature (C): ");
    Serial.print(temperatureC);
    Serial.print(" °C, ");
    Serial.print("Temperature (F): ");
    Serial.print(temperatureF);
    Serial.println(" °F");
  } else {
    Serial.println("Error: Device disconnected");
  }

  delay(2000); // Wait 2 seconds before the next reading

  String response = "temperatureF " + String(temperatureF) + 
                    ", temperatureC " + String(temperatureC);

  server.send(200, "text/plain", response);

}
// ------------------- Water Drop Sensor ------------------------
const int analogPin = 34;  // Connect A0 to GPIO 34
const int digitalPin = 32; // Connect D0 to GPIO 32

void waterDropSensor() {
    // Read the digital output from the rain sensor
    int digitalValue = digitalRead(digitalPin);
    // Read the analog output from the rain sensor
    int analogValue = analogRead(analogPin);

    // Print the values to the Serial Monitor
    Serial.print("Digital Output: ");
    Serial.print(digitalValue);
    Serial.print(", Analog Output: ");
    Serial.println(analogValue);

    // Delay a bit before the next read
    delay(500);
}

void handleWaterDropSensor() {
  // Read the digital output from the rain sensor
  int digitalValue = digitalRead(digitalPin);

  // Read the analog output from the rain sensor
  int analogValue = analogRead(analogPin);

  // Create a response string with the sensor values
  String response = "Digital Output: " + String(digitalValue) + 
                    ", Analog Output: " + String(analogValue);

  // Send the response back to the client
  server.send(200, "text/plain", response);
}

// ------------------- Hunter sprinkler valve ------------------------
const int hunterPin = 5; // GPIO pin connected to the relay module
String valveState = "off"; // Default state is 'off'

void handleToggleValve() {
  if (server.hasArg("state")) {
    String valveState = server.arg("state");

    if (valveState == "on") {
      digitalWrite(hunterPin, HIGH); // Activate the relay to open the valve
      valveState = "on";
      Serial.println("Valve is open");
      server.send(200, "text/plain", "Valve state set to open");
    } else if (valveState == "off") {
      digitalWrite(hunterPin, LOW); // Deactivate the relay to close the valve
      valveState = "off";
      Serial.println("Valve is closed");
      server.send(200, "text/plain", "Valve state set to closed");
    } else {
      server.send(400, "text/plain", "Invalid state parameter. Use 'on' or 'off'.");
    }
  } else {
    server.send(400, "text/plain", "Missing 'state' parameter. Use '?state=on' or '?state=off'.");
  }
}

// ------------------- Water Flow ------------------------

const int flowPin = 4; // GPIO pin connected to the flow sensor output
volatile int flowPulseCount; // Counts pulses from the flow meter

float flowRate;
unsigned long flowTimer;
unsigned long flowMillis;
const float calibrationFactor = 4.5; // Calibration factor for flow sensor (adjust as necessary)

void IRAM_ATTR pulseCounter() {
  flowPulseCount++;
}

void waterFlowSensor() {
    if (millis() - flowMillis > 1000) { // Update every second
    // Disable the interrupt while calculating flow rate and sending to serial
    detachInterrupt(digitalPinToInterrupt(flowPin));
    
    flowRate = ((1000.0 / (millis() - flowMillis)) * flowPulseCount) / calibrationFactor;
    flowMillis = millis();
    flowPulseCount = 0; // Reset pulse counter

    Serial.print("Flow rate: ");
    Serial.print(flowRate); // Flow rate in L/min
    Serial.println(" L/min");

    // Re-enable the interrupt
    attachInterrupt(digitalPinToInterrupt(flowPin), pulseCounter, RISING);
  }
}

void handleFlowRate() {
  // Respond with the current flow rate
  server.send(200, "text/plain", "Current flow rate: " + String(flowRate) + " L/min");
}
// ------------------- lidar ------------------------

const int BATCH_SIZE = 1;  // Adjust this number based on your needs
//const IPAddress multicastAddress(239, 255, 0, 1);  // Multicast group address
const int PACKET_SIZE = sizeof(LidarPacket);
static uint8_t batchBuffer[BATCH_SIZE * PACKET_SIZE];  // Buffer to accumulate batches of packet data
static uint8_t packetBuffer[PACKET_SIZE];  // Buffer for a single packet
static int packetIndex = 0;
static int batchIndex = 0;
const char* message = "hello world";
unsigned int udpPort = 8042; // Example port number
IPAddress ipAddress; // For storing the converted IP address
WiFiUDP udp;
String state = "";

void sendBatchData() {
    String batchString = "";
    for (int i = 0; i < BATCH_SIZE * PACKET_SIZE; i++) {
        batchString += String(batchBuffer[i], HEX);
        batchString += " ";
    }
    
    Serial.println("Sending batch data: " + batchString);
    
    if (udp.beginPacket(ipAddress, udpPort) == 1) {
        udp.write((const uint8_t*)batchString.c_str(), batchString.length());
        if (udp.endPacket() != 1) {
            Serial.println("Error sending UDP packet");
        }
    } else {
        Serial.println("Error beginning UDP packet");
    }
    
    memset(batchBuffer, 0, BATCH_SIZE * PACKET_SIZE); // Clear the buffer
}

void handleLidarData() {
    if (state == "on") {
        while (Serial2.available() > 0 && packetIndex < PACKET_SIZE) {
            if (state == "off") {
                break; // Stop processing if state changes to 'off'
            }
            
            // Reset batchIndex if it reaches the limit
            if (batchIndex >= BATCH_SIZE * PACKET_SIZE) {
                batchIndex = 0;
            }
            
            for (int i = 0; i < PACKET_SIZE; i++) {
                if (Serial2.available() > 0) {
                    uint8_t byte = Serial2.read();
                    batchBuffer[batchIndex++] = byte;
                }
            }

            // Send the batch if full
            if (batchIndex == BATCH_SIZE * PACKET_SIZE) {
                sendBatchData();
                batchIndex = 0; // Reset the batch index after sending
            }
        }
    }
}


#endif


// void handleLidarData() {
//     if (state == "on") {
//         //Serial.println("state is currently: " + state);

//         while (Serial2.available() > 0 && packetIndex < PACKET_SIZE) {
//             delay(100);
//             if (state == "off") {
//                     break; // Stop processing if state changes to 'off'
//                 }
//             if ((batchIndex + PACKET_SIZE) <= BATCH_SIZE * PACKET_SIZE) { // Ensure there's room for another packet
//                 for (int i = 0; i < PACKET_SIZE; ++i) {
//                     if (Serial2.available() > 0) {
//                         uint8_t byte = Serial2.read();
//                         batchBuffer[batchIndex++] = byte;
//                     }
//                 }
//             }
//             if (batchIndex == BATCH_SIZE * PACKET_SIZE) {
//                 // Convert batchBuffer to a string
//                 String batchString = "";
//                 for (int i = 0; i < BATCH_SIZE * PACKET_SIZE; ++i) {
//                     batchString += String(batchBuffer[i], HEX); // Convert byte to hexadecimal string
//                     batchString += " "; // Add space between bytes
//                     Serial.println("Sent batch data: " + batchString);
//                     Serial.println("Batch String length: " + batchString.length());

//                 }

//                 // Send the batch data as a byte array
//                 udp.beginPacket(ipAddress, udpPort);
//                 udp.write((const uint8_t*)batchString.c_str(), batchString.length()); // Convert the string to a byte array and send
//                 udp.endPacket();

//                 // Clear batchBuffer
//                 memset(batchBuffer, 0, BATCH_SIZE * PACKET_SIZE);
//             }
//         }
//     }