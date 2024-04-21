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
String state = "";
WiFiUDP udp;
//AsyncUDP udp;

LidarPacket latestPacket;

WebServer server(80);
bool isConnected = false; // Flag to check connection status

uint8_t globalByte;
String lidarDataBuffer = "";
String ipAddressStr = "";
IPAddress ipAddress; // For storing the converted IP address

unsigned long delayStart = 0;
bool delayRunning = false;

const int SOME_MAX_SIZE = 46 * 2; // 7200 bytes * 2 characters per byte
const int PACKET_SIZE = sizeof(LidarPacket);
unsigned int udpPort = 8042; // Example port number
const char* data = "Hello, UDP!";
char incomingPacket[255];  // Buffer for incoming packets
const int rele = 23;

void handleRelayToggle() {
    if (server.hasArg("state")) {
        state = server.arg("state");
        if (state == "on") {
            digitalWrite(relayPin, HIGH); // Turn on the relay
        } else if (state == "off") {
            digitalWrite(relayPin, LOW); // Turn off the relay
        }
        server.send(200, "text/plain", "Relay state set to " + state);
        Serial.println("State is set to " + state);
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
const char* message = "hello world";

void handleLidarData() {
    if (state == "on") {
        //Serial.println("state is currently: " + state);

        while (Serial2.available() > 0 && packetIndex < PACKET_SIZE) {
            if (state == "off") {
                    break; // Stop processing if state changes to 'off'
                }
            if ((batchIndex + PACKET_SIZE) <= BATCH_SIZE * PACKET_SIZE) { // Ensure there's room for another packet
                for (int i = 0; i < PACKET_SIZE; ++i) {
                    if (Serial2.available() > 0) {
                        uint8_t byte = Serial2.read();
                        batchBuffer[batchIndex++] = byte;
                    }
                }
            }
            if (batchIndex == BATCH_SIZE * PACKET_SIZE) {
                // Convert batchBuffer to a string
                String batchString = "";
                for (int i = 0; i < BATCH_SIZE * PACKET_SIZE; ++i) {
                    batchString += String(batchBuffer[i], HEX); // Convert byte to hexadecimal string
                    batchString += " "; // Add space between bytes
                }

                // Send the batch data as a byte array
                udp.beginPacket(ipAddress, udpPort);
                udp.write((const uint8_t*)batchString.c_str(), batchString.length()); // Convert the string to a byte array and send
                udp.endPacket();
                Serial.println("Sent batch data: " + batchString);

                // Clear batchBuffer
                memset(batchBuffer, 0, BATCH_SIZE * PACKET_SIZE);
            }
        }
    }
   // handleRelayToggle(); 

}

void handleReceiveIP() {
    if (server.hasArg("ip")) {
        ipAddressStr = server.arg("ip");
        if (ipAddress.fromString(ipAddressStr)) { // Convert String to IPAddress
            Serial.println("Received and converted IP: " + ipAddressStr);
            // Now, you can use ipAddress with udp.begin
            delay(1000);
            //Serial.println("UDP server started");
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


void setupAccessPointAndServer() {
    Preferences preferences; // Create a Preferences object
    preferences.begin("wifi", false); // Open the NVS namespace "wifi"

   String ssid = preferences.getString("ssid", ""); // Get saved SSID, default to empty string if not found
   String password = preferences.getString("password", ""); // Get saved password, default to empty string if not found

    // Start ESP32 as an Access Point
    WiFi.softAP(SSID, Password);
    Serial.println("Access Point Started");
    Serial.println("IP Address: ");
    Serial.println(WiFi.softAPIP());

    // Setup server route for form submission
    server.on("/setup", HTTP_POST, [&]() { // Capture preferences by reference
        Serial.println("Received a request on /setup");

        // Check for incoming SSID and password or use stored ones if present
        String newSSID = server.hasArg("ssid") ? server.arg("ssid") : ssid;
        String newPassword = server.hasArg("password") ? server.arg("password") : password;

        if (!newSSID.isEmpty() && !newPassword.isEmpty()) {
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

            WiFi.disconnect();

            IPAddress staticIP(WiFi.localIP()); // Example static IP
            IPAddress gateway(WiFi.gatewayIP());    // Gateway (your router IP)
            IPAddress subnet(WiFi.subnetMask());   // Subnet mask
            IPAddress primaryDNS(8, 8, 8, 8);     // Primary DNS (optional)
            IPAddress secondaryDNS(8, 8, 4, 4);   // Secondary DNS (optional)
            
            if (!WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
                Serial.println("STA Failed to configure");
            }

            WiFi.begin(ssidChar, passwordChar);
            Serial.println("Static IP set to...");
            IPAddress staticIPString = WiFi.localIP();
            Serial.println(staticIPString);

            //server.begin();
            // Serial.println("Connected to new Wi-Fi");
            // Serial.print("IP Address: ");
            // Serial.println(WiFi.localIP());
            server.send(200, "text/plain", "Connected to new Wi-Fi successfully");
        } else {
            Serial.println("Missing SSID or Password in the request");
            server.send(400, "text/plain", "Missing SSID or Password");
        }

    });
    //delay(10000);
    server.on("/getIP", HTTP_GET, []() {
        if (WiFi.status() == WL_CONNECTED) {
            server.send(200, "text/plain", WiFi.localIP().toString());
            Serial.println("Send localIP to swift");
        } else {
            server.send(404, "text/plain", "Not connected to WiFi");
        }
    });     

    server.on("/receiveIP", HTTP_GET, handleReceiveIP);

    server.on("/wifiConnected", handleRoot);

    preferences.end(); // Close the NVS namespace

    server.on("/toggleRelay", HTTP_GET,  handleRelayToggle);

    delay(1000);
    server.begin();
    //delay(100000);
    //isConnected = true;
   // WiFi.softAPdisconnect(true);
    //Serial.println("Access Point Stopped");
    delayStart = millis();
    delayRunning = true;
}


#endif

// void autoConnectWifi() {
//   preferences.begin("wifi", false); // Open NVS namespace "wifi" in read-only mode

//   // Attempt to connect using saved credentials
//   String ssid = preferences.getString("ssid", ""); // Get saved SSID, default to empty string if not found
//   String password = preferences.getString("password", ""); // Get saved password, default to empty string if not found

//   if (!ssid.isEmpty() && !password.isEmpty()) {
//     WiFi.begin(ssid.c_str(), password.c_str());

//     Serial.print("Connecting to ");
//     Serial.print(ssid);
//     Serial.println("...");

//     unsigned long startTime = millis();
//     while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) { // 30-second timeout
//       delay(500);
//       Serial.print(".");
//     }

//     if (WiFi.status() == WL_CONNECTED) {
//       Serial.println("\nConnected to Wi-Fi.");
//       server.on("/status", HTTP_GET, []() {
//                 if (WiFi.status() == WL_CONNECTED) {
//                     server.send(200, "text/plain", WiFi.localIP().toString());
//                 } else {
//                     server.send(404, "text/plain", "Not connected to WiFi");
//                 }
//      });

//       delay(10000);
  
//       WiFi.softAPdisconnect(true);
//       Serial.println("Access Point Stopped");


//       delay(1000);

//       Serial.print("IP Address: ");
//       Serial.println(WiFi.localIP());
//       server.on("/receiveIP", HTTP_GET, handleReceiveIP);
//       server.on("/wifiConnected", handleRoot);

//       preferences.end(); // Close the NVS namespace
//       return;
//     } else {
//       Serial.println("\nFailed to connect to saved Wi-Fi. Starting Access Point...");
//     }
//   } else {
//     Serial.println("No saved Wi-Fi credentials. Starting Access Point...");
//     setupAccessPointAndServer();
//   }

//   preferences.end(); // Ensure NVS is closed if not connected

//   // Call function to start Access Point here if auto-connect fails
//   //setupAccessPointAndServer();
//     //server.on("/lidar", HTTP_GET, handleLidarData);

//     server.on("/toggleRelay", HTTP_GET, handleRelayToggle);


//     server.begin();
// }


 // uint8_t buffer[50] = "hello world";

  // This initializes UDP and transfers buffer
//   udp.beginPacket(ipAddress, udpPort);
//   udp.write(buffer, 11); // send the message
//   udp.endPacket();

  // Short delay to allow server time to respond - adjust as necessary
//   delay(100);

//   // Check if there's an incoming packet
//   int packetSize = udp.parsePacket();
//   if (packetSize) {
//     // If there's data, read the packet
//     int len = udp.read(buffer, 100); // Read the packet into buffer, up to 50 bytes
//     if (len > 0) {
//       buffer[len] = 0; // Null-terminate the string
//       Serial.print("Server to client: ");
//       Serial.println((char *)buffer);
//     }
//   } else {
//     Serial.println("No packet received");
//   }

  // Clear the buffer after processing to avoid reading stale data
//   memset(buffer, 0, 50);

  //delay(2000); // Send every 2 seconds as an example

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
// void handleLidarData() {
//     while (Serial2.available() > 0 && packetIndex < PACKET_SIZE) {
//         if ((batchIndex + PACKET_SIZE) <= BATCH_SIZE * PACKET_SIZE) { // Ensure there's room for another packet
//             for (int i = 0; i < PACKET_SIZE; ++i) {
//                 if (Serial2.available() > 0) {
//                     uint8_t byte = Serial2.read();
//                     batchBuffer[batchIndex++] = byte;
//                 }
//             }
//         }
//         if (batchIndex == BATCH_SIZE * PACKET_SIZE) {
//             // data will be sent to server
//             udp.beginPacket(ipAddress, udpPort);
//             udp.write(batchBuffer, BATCH_SIZE * PACKET_SIZE);
//             udp.endPacket();
//             // Print the contents of batchBuffer in hexadecimal format
//             for (int i = 0; i < BATCH_SIZE * PACKET_SIZE; ++i) {
//                 Serial.print(batchBuffer[i], HEX);
//                 Serial.print(" ");
//             }
//             Serial.println(); // Print a newline
//         }
//        // delay(50);
//         memset(batchBuffer, 0, BATCH_SIZE * PACKET_SIZE);
//     }
// }

// void handleLidarData() {

//     while (Serial2.available() > 0 && packetIndex < PACKET_SIZE) {

//         if ((batchIndex + PACKET_SIZE) <= BATCH_SIZE * PACKET_SIZE) { // Ensure there's room for another packet
//                 for (int i = 0; i < PACKET_SIZE; ++i) {
//                     if (Serial2.available() > 0) {
//                         uint8_t byte = Serial2.read();
//                         batchBuffer[batchIndex++] = byte;
//                     }
//                 }
//             }
//         if (batchIndex == BATCH_SIZE * PACKET_SIZE) {
//            // data will be sent to server
//             //uint8_t buffer[50] = "hello world";
//             //This initializes udp and transfer buffer
//             udp.beginPacket(ipAddress, udpPort);
//             //udp.write(buffer, 11);
//             udp.write(batchBuffer, BATCH_SIZE * PACKET_SIZE);
//             Serial.print(batchBuffer)
//             udp.endPacket();
//             // memset(buffer, 0, 50);
//             // //processing incoming packet, must be called before reading the buffer
//             //  udp.parsePacket();
//             // //receive response from server, it will be HELLO WORLD
//             // if(udp.read(buffer, 50) > 0){
//             //     Serial.print("Server to client: ");
//             //     Serial.println((char *)buffer);
//             // }
//             // Serial.println("Batch sent");
//             // batchIndex = 0;
//             //Wait for 1 second
//             memset(batchBuffer, 0, BATCH_SIZE * PACKET_SIZE);
//         }
//         delay(50);
//      }
// }