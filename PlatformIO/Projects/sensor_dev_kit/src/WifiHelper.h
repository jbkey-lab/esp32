// WifiHelper.h
#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "LidarHelper.h"
#include <Preferences.h>
#include <WiFiUdp.h>
#include <AsyncUDP.h>

Preferences preferences;

//const char* globalSSID = "The LAN before Time";     // Replace with your network SSID
//const char* globalPassword = "Jbl?8042"; // Replace with your network password
const char* SSID = "lidar";     // Replace with your network SSID
const char* Password = "123456789"; // Replace with your network password

bool relayState = false; // Variable to hold the relay state
const int relayPin = 13;  // Relay connected to GPIO 13
WiFiUDP udp;
//AsyncUDP udp;

LidarPacket latestPacket;

WebServer server(80);

uint8_t globalByte;
String lidarDataBuffer = "";
String ipAddressStr = "";
IPAddress ipAddress; // For storing the converted IP address

const int SOME_MAX_SIZE = 46 * 2; // 7200 bytes * 2 characters per byte
const int PACKET_SIZE = sizeof(LidarPacket);
unsigned int udpPort = 8042; // Example port number
const char* data = "Hello, UDP!";
char incomingPacket[255];  // Buffer for incoming packets
const int rele = 23;

void handleRelayToggle() {
    if (server.hasArg("state")) {
        String state = server.arg("state");
        if (state == "on") {
            digitalWrite(relayPin, HIGH); // Turn on the relay
        } else if (state == "off") {
            digitalWrite(relayPin, LOW); // Turn off the relay
        }
        server.send(200, "text/plain", "Relay state set to " + state);
    } else {
        server.send(400, "text/plain", "Missing 'state' parameter");
    } 
}


const int BATCH_SIZE = 1;  // Adjust this number based on your needs
const IPAddress multicastAddress(239, 255, 0, 1);  // Multicast group address
static uint8_t batchBuffer[BATCH_SIZE * PACKET_SIZE];  // Buffer to accumulate batches of packet data
static uint8_t packetBuffer[PACKET_SIZE];  // Buffer for a single packet
static int packetIndex = 0;
static int batchIndex = 0;

void handleLidarData() {
    // while (Serial2.available() > 0 && packetIndex < PACKET_SIZE) {
    //     uint8_t byte = Serial2.read();
    //     packetBuffer[packetIndex++] = byte; // Store the byte in the packet buffer

    //     if (packetIndex == PACKET_SIZE) {
    //         // Check if there is enough space in the batch buffer
    //         if (batchIndex + PACKET_SIZE <= BATCH_SIZE * PACKET_SIZE) {
    //             // Copy the completed packet to the batch buffer
    //             memcpy(batchBuffer + batchIndex, packetBuffer, PACKET_SIZE);
    //             batchIndex += PACKET_SIZE;
    //             packetIndex = 0; // Reset packet index for next packet
    //         } else {
    //             Serial.println("Error: Batch buffer overflow");
    //             batchIndex = 0;
    //             packetIndex = 0;
    //             return; // Early return on error
    //         }
    //     }
    while (Serial2.available() > 0 && packetIndex < PACKET_SIZE) {

        if ((batchIndex + PACKET_SIZE) <= BATCH_SIZE * PACKET_SIZE) { // Ensure there's room for another packet
                for (int i = 0; i < PACKET_SIZE; ++i) {
                    if (Serial2.available() > 0) {
                        uint8_t byte = Serial2.read();
                        batchBuffer[batchIndex++] = byte;
                    }
                }
            }
        if (batchIndex == BATCH_SIZE * PACKET_SIZE) {
            //data will be sent to server
            uint8_t buffer[50] = "hello world";
            //This initializes udp and transfer buffer
            udp.beginPacket(ipAddress, udpPort);
            udp.write(buffer, 11);
            //udp.write(batchBuffer, BATCH_SIZE * PACKET_SIZE);

            udp.endPacket();
            //memset(buffer, 0, 50);
            //processing incoming packet, must be called before reading the buffer
             udp.parsePacket();
            //receive response from server, it will be HELLO WORLD
            if(udp.read(buffer, 50) > 0){
                Serial.print("Server to client: ");
                Serial.println((char *)buffer);
            }
            Serial.println("Batch sent");
            batchIndex = 0;
            //Wait for 1 second
            //memset(batchBuffer, 0, BATCH_SIZE * PACKET_SIZE);
        }
        delay(50);
    }
}


void handleReceiveIP() {
    if (server.hasArg("ip")) {
        ipAddressStr = server.arg("ip");
        if (ipAddress.fromString(ipAddressStr)) { // Convert String to IPAddress
            Serial.println("Received and converted IP: " + ipAddressStr);
            // Now, you can use ipAddress with udp.begin
            delay(1000);
            Serial.println("UDP server started");
            server.send(200, "text/plain", "IP Address received and UDP started: " + ipAddressStr);
        } else {
            Serial.println("Invalid IP address received");
            server.send(400, "text/plain", "Invalid IP address");
        }
    } else {
        server.send(400, "text/plain", "Missing IP address");
    }
}


void handleRoot() {
  server.send(200, "text/plain", "\nConnected to Wi-Fi.");
}

void autoConnectWifi() {
  preferences.begin("wifi", false); // Open NVS namespace "wifi" in read-only mode

  // Attempt to connect using saved credentials
  String ssid = preferences.getString("ssid", ""); // Get saved SSID, default to empty string if not found
  String password = preferences.getString("password", ""); // Get saved password, default to empty string if not found

  if (!ssid.isEmpty() && !password.isEmpty()) {
    WiFi.begin(ssid.c_str(), password.c_str());

    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) { // 30-second timeout
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to Wi-Fi.");
      WiFi.softAPdisconnect(true);
      Serial.println("Access Point Stopped");

      delay(10000);
      server.on("/wifiConnected", handleRoot);
      delay(1000);

      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      server.on("/receiveIP", HTTP_GET, handleReceiveIP);

      preferences.end(); // Close the NVS namespace
      return;
    } else {
      Serial.println("\nFailed to connect to saved Wi-Fi. Starting Access Point...");
    }
  } else {
    Serial.println("No saved Wi-Fi credentials. Starting Access Point...");
    //setupAccessPointAndServer()
  }

  preferences.end(); // Ensure NVS is closed if not connected

  // Call function to start Access Point here if auto-connect fails
  //setupAccessPointAndServer();
    //server.on("/lidar", HTTP_GET, handleLidarData);

    server.on("/toggleRelay", HTTP_GET, handleRelayToggle);


    server.begin();
}

void setupAccessPointAndServer() {
    Preferences preferences; // Create a Preferences object
    preferences.begin("wifi", false); // Open the NVS namespace "wifi"

    // Start ESP32 as an Access Point
        WiFi.softAP(SSID, Password);
        Serial.println("Access Point Started");
        Serial.println("IP Address: ");
        Serial.println(WiFi.softAPIP());


    // Setup server route for form submission
    server.on("/setup", HTTP_POST, [&]() { // Capture preferences by reference
        Serial.println("Received a request on /setup");

        if (server.hasArg("ssid") && server.hasArg("password")) {
            String newSSID = server.arg("ssid");
            String newPassword = server.arg("password");

            Serial.print("New SSID: ");
            Serial.println(newSSID);
            Serial.print("New Password: ");
            Serial.println(newPassword);

            // Save SSID and password to NVS
            preferences.putString("ssid", newSSID);
            preferences.putString("password", newPassword);

            // Convert String to char*
            char ssidChar[newSSID.length() + 1];
            newSSID.toCharArray(ssidChar, newSSID.length() + 1);
            char passwordChar[newPassword.length() + 1];
            newPassword.toCharArray(passwordChar, newPassword.length() + 1);

            // Attempt to connect to new Wi-Fi network
            WiFi.begin(ssidChar, passwordChar);
            unsigned long startTime = millis();
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.println("Attempting to connect to new Wi-Fi...");

                if (millis() - startTime > 30000) { // 30-second timeout
                    Serial.println("Failed to connect to new Wi-Fi.");
                    server.send(500, "text/plain", "Failed to connect to new Wi-Fi");
                    ESP.restart();
                    return;
                }
            }
            delay(10000);

            WiFi.softAPdisconnect(true);
            Serial.println("Access Point Stopped");
            server.on("/wifiConnected", handleRoot);
            delay(1000);
            Serial.println("Connected to new Wi-Fi");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            server.on("/receiveIP", HTTP_GET, handleReceiveIP);

            //udp.begin(ipAddress, udpPort);

            server.send(200, "text/plain", "Connected to new Wi-Fi successfully");
        } else {
            Serial.println("Missing SSID or Password in the request");
            server.send(400, "text/plain", "Missing SSID or Password");
        }
    });

    preferences.end(); // Close the NVS namespace

    //server.on("/lidar", HTTP_GET, handleLidarData);

    server.on("/toggleRelay", HTTP_GET, handleRelayToggle);

    server.begin();

}


#endif
            // if (udp.begin(ipAddress, udpPort)) {
            //     Serial.println("UDP started successfully.");
            //     //return true;
            //     handleLidarData();
            // } else {
            //     Serial.println("Failed to start UDP.");
            //    // return false;
            // }
            // if (udp.listen(udpPort)) {
            //     Serial.println("UDP server started");
            //     udp.onPacket([](AsyncUDPPacket packet) {
            //         udp.onPacket([](AsyncUDPPacket packet) {
            //         Serial.print("UDP Packet Type: ");
            //         Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
            //         Serial.print(", From: ");
            //         Serial.print(packet.remoteIP());
            //         Serial.print(":");
            //         Serial.print(packet.remotePort());
            //         Serial.print(", To: ");
            //         Serial.print(packet.localIP());
            //         Serial.print(":");
            //         Serial.print(packet.localPort());
            //         Serial.print(", Length: ");
            //         Serial.print(packet.length()); //dlzka packetu
            //         Serial.print(", Data: ");
            //         Serial.write(packet.data(), packet.length());
            //         Serial.println();
            //         String myString = (const char*)packet.data();
            //         if (myString == "ZAP") {
            //             Serial.println("Zapinam rele");
            //             //digitalWrite(rele, LOW);
            //         } else if (myString == "VYP") {
            //             Serial.println("Vypinam rele");
            //             //digitalWrite(rele, HIGH);
            //         }
            //         packet.printf("Got %u bytes of data", packet.length());
            //         });

            //         // Serial.print("Received data: ");
            //         // Serial.write(packet.data(), packet.length());
            //         // Serial.println();
            //     });
            // } else {
            //     Serial.println("Error: Failed to start UDP server");
            // }
                //udp.broadcast("Anyone here?");

    // Send batch if it's ready
    // if (batchIndex >= BATCH_SIZE * PACKET_SIZE) {
    //     // Broadcast the message to all devices on the network
    //     udp.broadcastTo(reinterpret_cast<const char*>(batchBuffer), batchIndex, udpPort);
    //     Serial.println("UDP packet broadcasted successfully");
    //     batchIndex = 0; // Reset the batch index for the next batch
    // }
    // if (batchIndex >= BATCH_SIZE * PACKET_SIZE) {
    //     if (udp.beginPacket(ipAddress, udpPort)) {
    //         udp.write(batchBuffer, batchIndex);
    //         if (udp.endPacket() == 0) {
    //             Serial.println("Error: Failed to send UDP packet");
    //         } else {
    //             Serial.println("UDP packet sent successfully");
    //         }
    //     } else {
    //         Serial.println("Error: Failed to begin UDP packet");
    //     }
    //     batchIndex = 0; // Reset the batch index for the next batch
    // }
    // const int packet_test = 60; // Define the size of one packet (adjust based on your data)

    // uint8_t testData[packet_test];
    // for (int i = 0; i < packet_test; i++) {
    //     testData[i] = i % 60; // Fill test data with incrementing values
    // }

    // // Send test UDP packet
    // if (udp.beginPacket(ipAddress, udpPort)) {
    //     udp.write(testData, packet_test);
    //     if (udp.endPacket() == 0) {
    //         Serial.println("Error: Failed to send UDP packet");
    //     } else {
    //         Serial.println("UDP packet sent successfully");
    //     }
    // } else {
    //     Serial.println("Error: Failed to begin UDP packet");
    // }
   // memset(batchBuffer, 0, BATCH_SIZE * packet_test);
