# WiFi

## Description of ESP32-S3 Wi-Fi
The ESP32-S3 is a highly integrated, low-power Wi-Fi and Bluetooth MCU by Espressif. Its Wi-Fi subsystem is a significant upgrade over the original ESP32, featuring:
- Standard Support: 2.4 GHz 802.11 b/g/n.
- Speed: Up to 150 Mbps data rate.
- Security: Supports WPA/WPA2/WPA3, and enterprise security.
- Features: Fast scanning, dedicated Wi-Fi MAC/Baseband, and support for Wi-Fi Easy Connect™ (DPP) and Wi-Fi Enhanced Open™ (OWE).
- Hardware: It features a dual-core Xtensa® 32-bit LX7 processor and native USB OTG, making it ideal for AIoT (AI + IoT) applications.

## Wi-Fi Operation Modes     
The ESP32-S3 can operate in several distinct Wi-Fi modes, which can be used individually or combined:
1. **Station (STA) Mode**: The ESP32-S3 connects to an existing Wi-Fi router/Access Point (like your phone or laptop does). It gets an IP address from the router.    
   Use Case: Fetching web data, sending sensor readings to MQTT/Cloud.
2. **Access Point (AP / SoftAP) Mode**: The ESP32-S3 acts as a router itself. It broadcasts its own SSID, and other devices (phones, laptops) can connect to it. It assigns IP addresses to connected clients.     
   Use Case: Initial device provision, direct phone-to-ESP control, captive portals.
3. **AP + STA Mode (Mixed Mode)**: The ESP32-S3 operates as a Station (connected to your home router) AND as an Access Point (broadcasting its own network) simultaneously. This is highly useful for configuring devices or creating a local control network while maintaining internet access.    
   Use Case: Wi-Fi repeaters, range extenders, local web fallback while online.
4. **Wi-Fi Sniffer (Promiscuous) Mode**: The ESP32-S3 can capture raw 802.11 Wi-Fi packets in the air without connecting to any network. Used for packet analysis and security research.     
   Use Case: Network analysis, packet sniffing, presence detection.
5. **ESP-NOW**: Espressif’s proprietary, connectionless peer-to-peer protocol using raw Wi-Fi frames.     
   Use Case: Low-power sensor networks, ultra-fast board-to-board communication.

## Core Wi-Fi API Reference     
Include the Wi-Fi library at the top of your sketch: ```#include <WiFi.h>```     
https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFi.h     

**Station (STA) Mode APIs**     
|	Function	|	Description	|
|	-	|	-	|
|  ```WiFi.mode(WIFI_STA)``` | Sets mode to Station. Type: ```WIFI_OFF```, ```WIFI_STA```, ```WIFI_AP```, ```WIFI_AP_STA``` |
|	```WiFi.begin(ssid, password)```	|	Connects to a Wi-Fi network.	|
|	```WiFi.status()```	|	Returns connection status (e.g., WL_CONNECTED, WL_DISCONNECTED).	|
|	```WiFi.isConnected()```	|	Returns true if connected to an AP.	|
|	```WiFi.localIP()```	|	Returns the IP address (IPv4) assigned to the ESP32.	|
|	```WiFi.macAddress()```	|	Returns the MAC address of the ESP32.	|
|	```WiFi.RSSI()```	|	Returns the signal strength in dBm of the connected network.	|
|	```WiFi.disconnect()```	|	Disconnects from the current Wi-Fi network.	|
|	```WiFi.setAutoReconnect(bool)```	|	Enables/disables automatic reconnection.	|

**Access Point (AP) Mode APIs**    
|	Function	|	Description	|
|	-	|	-	|
|  ```WiFi.mode(WIFI_AP)``` | Sets mode to Access Point.  |
|	```WiFi.softAP(ssid, pass, ch, [hide], [max])```	|	Starts an Access Point. ch=channel, hide=hidden SSID, max=max clients.	|
|	```WiFi.softAPIP()```	|	Returns the IP address of the ESP32 in AP mode (usually 192.168.4.1).	|
|	```WiFi.softAPmacAddress()```	|	Returns the MAC address of the SoftAP.	|
|	```WiFi.softAPgetStationNum()```	|	Returns the number of devices currently connected to the SoftAP.	|
|  ```WiFi.softAPgetConnectedStations()``` |  Returns count of connected clients. |
|	```WiFi.softAPdisconnect(wifi_off)```	|	Disconnects all clients and shuts down the SoftAP.	|

**Network Scanning APIs**
|	Function	|	Description	|
|	-	|	-	|
|	```WiFi.scanNetworks()```	|	Scans for available networks. Returns the number of networks found.	|
|	```WiFi.SSID(index)```	|	Returns the SSID of the network at the scanned index.	|
|	```WiFi.RSSI(index)```	|	Returns the signal strength in dBm of the network at the scanned index.	|      

**Note on WiFi.mode()**      
In the ESP32 Arduino core, WiFi.mode() is optional for basic STA or AP setups because the library sets it automatically behind the scenes.      
While optional for basic setups, there are specific scenarios where you must use WiFi.mode():        



1. Mixed Mode (AP + STA)      
If you want the ESP32 to act as an Access Point and connect to a router at the same time, the auto-magic doesn't know which one you want to prioritize. You must explicitly tell it to use both:     
```
WiFi.mode(WIFI_AP_STA); // Mandatory for mixed mode!
WiFi.softAP(ap_ssid, ap_pass);
WiFi.begin(sta_ssid, sta_pass);
```
2. Turning Wi-Fi Off to Save Power     
If you are running on a battery and want to completely shut down the Wi-Fi radio to save power, you use:     
```
WiFi.mode(WIFI_OFF); 
// or simply WiFi.disconnect(true) which also turns off the radio
```
3. Sniffer / Promiscuous Mode      
If you want to capture raw Wi-Fi packets without connecting to a network, you must set the mode to null/station and then enable promiscuous:
```
WiFi.mode(WIFI_STA);
WiFi.promiscuousEnable(true);
```

## Application Examples

**Station STA Mode**     
```
#include <WiFi.h>

// Your home router credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== ESP32-S3 in STATION (STA) MODE ===");

 // Explicitly set the mode before connecting
  WiFi.mode(WIFI_STA); 
  // 1. Connect to the existing router
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to router");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to Home Router!");
  
  // 2. Print the IP address assigned by your router
  Serial.print("ESP32 IP Address on your network: ");
  Serial.println(WiFi.localIP()); 
  Serial.println("(You can ping this IP from your PC)");
}

void loop() {
  // Just keep the connection alive and print signal strength every 5 seconds
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected! Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("Lost connection! Trying to reconnect...");
    WiFi.begin(ssid, password);
  }
  delay(5000); 
}
```
**Station STA Mode with Simple Web Server**  
The ESP32 connects to your home router. To see the web page, your phone must be connected to your home Wi-Fi, and you type the ESP32's IP address into your browser.     
```
#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80); // Create a web server on port 80

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>ESP32-S3 STA Mode</h1>";
  html += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";
  html += "<p>RSSI: " + String(WiFi.RSSI()) + " dBm</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Connecting to Wi-Fi...");
  // Explicitly set the mode before connecting
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Define routes
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP Server started");
}

void loop() {
  server.handleClient(); // Handle incoming web requests
}
```
**Station STA Mode with Web Server to control LED**      
This code hosts a simple web page on the ESP32-S3 that displays uptime and allows toggling the onboard LED.    
```
#include <WiFi.h>
#include <WebServer.h>

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);
const int LED_PIN = 2; // Built-in LED on most S3 dev boards
bool ledState = LOW;

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ESP32-S3 Web Control</title></head>";
  html += "<body style='font-family:sans-serif; text-align:center; margin-top:50px;'>";
  html += "<h2>ESP32-S3 Wi-Fi Web Server</h2>";
  html += "<p>LED Status: <b>" + String(ledState ? "ON" : "OFF") + "</b></p>";
  html += "<a href='/toggle'><button style='padding:10px 20px; font-size:16px;'>Toggle LED</button></a>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleToggle() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
  server.sendHeader("Location", "/");
  server.send(303); // Redirect back to root
  Serial.print("LED State: ");
  Serial.println(ledState);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected!");
  Serial.print("Open in Browser: http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();
}

void loop() {
  server.handleClient();
}
```
**Station STA Mode with Web Server and Authentication**      
```
/*  
  Rui Santos & Sara Santos - Random Nerd Tutorials
  https://RandomNerdTutorials.com/esp32-web-server-beginners-guide/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Username and password for web page access
const char* http_username = "admin";
const char* http_password = "admin";

// Assign output variables to GPIO pins
const int output26 = 48;
const int output27 = 47;
String output26State = "off";
String output27State = "off";

// Create a web server object
WebServer server(80);

// Function to authenticate user
bool isAuthenticated() {
  if (!server.authenticate(http_username, http_password)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

// Function to handle turning GPIO 26 on
void handleGPIO26On() {
  if (!isAuthenticated()) return;
  output26State = "on";
  digitalWrite(output26, HIGH);
  handleRoot();
}

// Function to handle turning GPIO 26 off
void handleGPIO26Off() {
  if (!isAuthenticated()) return;
  output26State = "off";
  digitalWrite(output26, LOW);
  handleRoot();
}

// Function to handle turning GPIO 27 on
void handleGPIO27On() {
  if (!isAuthenticated()) return;
  output27State = "on";
  digitalWrite(output27, HIGH);
  handleRoot();
}

// Function to handle turning GPIO 27 off
void handleGPIO27Off() {
  if (!isAuthenticated()) return;
  output27State = "off";
  digitalWrite(output27, LOW);
  handleRoot();
}

// Function to handle the root URL and show the current states
void handleRoot() {
  if (!isAuthenticated()) return;

  String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<link rel=\"icon\" href=\"data:,\">";
  html += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}";
  html += ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}";
  html += ".button2 { background-color: #555555; }</style></head>";
  html += "<body><h1>ESP32 Web Server</h1>";

  // Display GPIO 26 controls
  html += "<p>GPIO 26 - State " + output26State + "</p>";
  if (output26State == "off") {
    html += "<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>";
  } else {
    html += "<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>";
  }

  // Display GPIO 27 controls
  html += "<p>GPIO 27 - State " + output27State + "</p>";
  if (output27State == "off") {
    html += "<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>";
  } else {
    html += "<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>";
  }

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  // Initialize the output variables as outputs
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output26, LOW);
  digitalWrite(output27, LOW);

  // Connect to Wi-Fi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up the web server to handle different routes with authentication
  server.on("/", handleRoot);
  server.on("/26/on", handleGPIO26On);
  server.on("/26/off", handleGPIO26Off);
  server.on("/27/on", handleGPIO27On);
  server.on("/27/off", handleGPIO27Off);

  // Start the web server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle incoming client requests
  server.handleClient();
}
```

**Access Point (AP) Mode**     
```
#include <WiFi.h>

// Credentials for the ESP32's own hotspot
const char* ap_ssid = "ESP32-S3-Hotspot";
const char* ap_password = "12345678";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== ESP32-S3 in ACCESS POINT (AP) MODE ===");
  
  // 1. Create the Wi-Fi Hotspot (No router needed!)
  // Explicitly set the mode before starting the AP
  WiFi.mode(WIFI_AP); 
  // Parameters: SSID, Password, Channel, Hide SSID, Max Clients
  WiFi.softAP(ap_ssid, ap_password, 1, 0, 4);
  
  // 2. Print the IP address of the hotspot (Usually 192.168.4.1)
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Access Point Started! SSID: ");
  Serial.println(ap_ssid);
  Serial.print("Hotspot created! IP Address: ");
  Serial.println(myIP);
  Serial.println("Go to your phone's Wi-Fi settings and connect to 'ESP32-S3-Hotspot'");
}

void loop() {
  // Count how many devices are connected to our ESP32 hotspot
  int stationCount = WiFi.softAPgetStationNum();
  
  Serial.print("Devices currently connected to my Hotspot: ");
  Serial.println(stationCount);
  
  if (stationCount > 0) {
    Serial.println(" -> Someone is connected to me!");
  } else {
    Serial.println(" -> Waiting for someone to connect...");
  }
  
  delay(3000);
}
```
**Access Point (AP) Mode with simple Web Server**     
```
#include <WiFi.h>
#include <WebServer.h>

const char* ap_ssid = "ESP32-S3-AP";
const char* ap_password = "12345678";  // Must be at least 8 characters

WebServer server(80);

void handleRoot() {
  String html = "<h1>Welcome to ESP32-S3 SoftAP!</h1>";
  html += "<p>You are connected to the ESP32-S3 directly.</p>";
  html += "<p>Connected Clients: " + String(WiFi.softAPgetStationNum()) + "</p>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  Serial.println("Configuring Access Point...");

  // Start SoftAP
  // Parameters: SSID, Password, Channel, Hidden, Max Connections
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password, 1, 0, 4);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP SSID: ");
  Serial.println(ap_ssid);
  Serial.print("AP IP address: ");
  Serial.println(myIP);  // Usually 192.168.4.1

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP Server started on AP");

}

void loop() {
  server.handleClient();
    // Count how many devices are connected to our ESP32 hotspot
  int stationCount = WiFi.softAPgetStationNum();
  
  Serial.print("Devices currently connected to my Hotspot: ");
  Serial.println(stationCount);
  delay(5000);
}
```
**Summary of the Core Difference**      

|	Feature	|	Station (STA) Mode	|	Access Point (AP) Mode	|
|	-	|	-	|	-	|
|	What it does	|	Joins an existing Wi-Fi network.	|	Creates its own Wi-Fi network.	|
|	Code Command	|	```WiFi.begin(ssid, pass)```	|	```WiFi.softAP(ssid, pass)```	|
|	IP Address	|	Assigned by your home router (e.g., ```192.168.1.50```).	|	Assigned by itself (usually ```192.168.4.1```).	|
|	Internet Access	|	Yes (if the home router has internet).	|	No (it's an isolated local network).	|
|	Real-world analogy	|	Your laptop connecting to your home Wi-Fi.	|	Your phone turning on its "Personal Hotspot".	|




**AP + STA Mode (Mixed Mode)**      

This is highly practical. The ESP32 connects to your home Wi-Fi (STA) to get internet, while simultaneously broadcasting its own Wi-Fi (AP) for local device connections.     
```
#include <WiFi.h>
#include <WebServer.h>

// STA Credentials (Your Home Router)
const char* sta_ssid = "YOUR_HOME_WIFI";
const char* sta_password = "YOUR_HOME_PASSWORD";

// AP Credentials (ESP32's own network)
const char* ap_ssid = "ESP32-S3-Mixed";
const char* ap_password = "12345678";

WebServer server(80);

void handleRoot() {
  String html = "<h1>ESP32-S3 AP+STA Mode</h1>";
  html += "<h3>Station (STA) Info:</h3>";
  html += "<p>Connected to: " + String(WiFi.SSID()) + "</p>";
  html += "<p>Router IP: " + WiFi.localIP().toString() + "</p>";
  
  html += "<h3>Access Point (AP) Info:</h3>";
  html += "<p>AP SSID: " + String(ap_ssid) + "</p>";
  html += "<p>AP IP: " + WiFi.softAPIP().toString() + "</p>";
  html += "<p>Clients connected to AP: " + String(WiFi.softAPgetStationNum()) + "</p>";
  
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  
  // 1. Start Access Point FIRST
  WiFi.mode(WIFI_AP_STA); // Mandatory for mixed mode!
  Serial.println("Starting AP...");
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // 2. Connect to Home Router (STA)
  Serial.println("Connecting to STA...");
  WiFi.begin(sta_ssid, sta_password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Router!");
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Router. AP is still running.");
  }

  // 3. Start Web Server
  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();
}
```     
Note: To access the Web Client.
- Method 1: Connect your computer/handphone WiFi to AP_SSID. Go to browser enter AP_IP.
- Method 2: Connect to same router WiFi. Go to browser enter STA_IP.

**Wi-Fi Sniffing**     

The ESP32-S3 (like the original ESP32) features a built-in Wi-Fi radio that supports Promiscuous Mode. When enabled, the Wi-Fi driver bypasses the standard MAC address filtering and passes all captured 802.11 frames to the application layer.     

In the Arduino IDE, we access this feature using the underlying ESP-IDF (Espressif IoT Development Framework) functions.     

To keep this educational and safe, the following code specifically filters for 802.11 Management Frames (Type 0). It ignores Data frames (which contain the actual encrypted user traffic) and Control frames. It specifically looks for:     
- Beacon Frames (Subtype 8): Broadcasted by routers to announce their SSID.
- Probe Request Frames (Subtype 4): Broadcasted by client devices (phones, laptops) looking for known networks.     

```
#include <WiFi.h>
#include <esp_wifi.h>

// --- Configuration ---
const unsigned long CHANNEL_SWITCH_INTERVAL = 3000;  // Time in ms to stay on one channel
const int MAX_CHANNEL = 13;                          // Max 2.4GHz channel (adjust to 11 for US)

// --- Global Variables ---
int current_channel = 1;
unsigned long last_channel_switch = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Allow time for Serial monitor to connect
  Serial.println("\n\n========================================");
  Serial.println(" ESP32-S3 Wi-Fi Sniffer (Management Frames)");
  Serial.println("========================================\n");

  // 1. Initialize Wi-Fi in Station mode, but DO NOT connect to any AP
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // 2. Enable Promiscuous Mode
  esp_wifi_set_promiscuous(true);

  // 3. Register the callback function to handle incoming packets
  esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous_callback);

  // 4. Set the initial Wi-Fi channel
  esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);

  Serial.printf("[*] Sniffer started. Listening on Channel %d...\n", current_channel);
  Serial.println("[*] Filtering for Beacon and Probe Request frames only.\n");
}

void loop() {
  // Simple Channel Hopping Logic
  // Wi-Fi networks operate on different channels; a sniffer must hop to see everything.
  if (millis() - last_channel_switch > CHANNEL_SWITCH_INTERVAL) {
    current_channel = (current_channel % MAX_CHANNEL) + 1;
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);

    Serial.printf("\n--- Hopped to Channel %02d ---\n", current_channel);
    last_channel_switch = millis();
  }

  // Yield to the watchdog timer
  delay(10);
}

/**
 * @brief Callback function triggered when a Wi-Fi packet is received in promiscuous mode.
 * 
 * @param buf Pointer to the received packet buffer
 * @param type The type of packet (mgmt, data, ctrl, etc.)
 */

void wifi_promiscuous_callback(void *buf, wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  const uint8_t *payload = ppkt->payload;
  uint16_t len = ppkt->rx_ctrl.sig_len;

  if (len < 24) return;

  uint8_t frame_control = payload[0];
  uint8_t frame_type = (frame_control >> 2) & 0x03;
  uint8_t frame_subtype = (frame_control >> 4) & 0x0F;

  if (frame_type != 0) return;
  if (frame_subtype != 8 && frame_subtype != 4) return;

  const uint8_t *transmitter_mac = payload + 10;

  // --- Extract SSID (Information Elements) ---
  // The start of Information Elements depends on the frame subtype.
  // The MAC header is always 24 bytes for management frames.
  uint16_t ie_offset = 24;

  if (frame_subtype == 8) {
    // BEACON frames have 12 bytes of fixed parameters before the IEs:
    // Timestamp (8 bytes) + Beacon Interval (2 bytes) + Capability Info (2 bytes)
    ie_offset += 12;
  }
  // PROBE REQUEST (subtype 4) has 0 bytes of fixed parameters, so ie_offset remains 24.

  String ssid = "";
  if (len > ie_offset) {
    // Iterate through the Information Elements
    while (ie_offset + 2 <= len) {
      uint8_t ie_id = payload[ie_offset];
      uint8_t ie_len = payload[ie_offset + 1];

      // Prevent buffer over-read
      if (ie_offset + 2 + ie_len > len) break;

      // The SSID is always Information Element ID 0
      if (ie_id == 0) {
        if (ie_len > 0) {
          char ssid_buf[33] = { 0 };  // Max SSID length is 32 bytes + null terminator
          uint8_t copy_len = (ie_len > 32) ? 32 : ie_len;
          memcpy(ssid_buf, payload + ie_offset + 2, copy_len);

          // SANITIZE: Replace non-printable ASCII characters with '?'
          // This prevents Serial Monitor garbage from high-bit UTF-8 or control chars
          for (uint8_t i = 0; i < copy_len; i++) {
            if (ssid_buf[i] < 32 || ssid_buf[i] > 126) {
              ssid_buf[i] = '?';
            }
          }
          ssid = String(ssid_buf);
        } else {
          // If length is 0, it's a hidden network broadcasting an empty SSID
          ssid = "Hidden Network";
        }
        break;  // Found the SSID, stop searching
      }
      ie_offset += 2 + ie_len;
    }
  }

  // Format and print the output
  const char *frame_type_str = (frame_subtype == 8) ? "BEACON " : "PROBE  ";

  if (ssid.isEmpty()) {
    ssid = (frame_subtype == 4) ? "Broadcast (Wildcard)" : "Hidden Network";
  }

  Serial.printf("[%s] MAC: %02X:%02X:%02X:%02X:%02X:%02X | SSID: %s\n",
                frame_type_str,
                transmitter_mac[0], transmitter_mac[1], transmitter_mac[2],
                transmitter_mac[3], transmitter_mac[4], transmitter_mac[5],
                ssid.c_str());
  //if (ssid == "Nightingale_IoT")
  //  Serial.println("+++++++++++++++++++++++++++++++");
}
```
How the Code Works     
- Promiscuous Initialization:
  ```WiFi.mode(WIFI_STA)``` initializes the Wi-Fi driver without actually connecting to a router.
  ```esp_wifi_set_promiscuous(true)``` disables the hardware MAC filter, and ```esp_wifi_set_promiscuous_rx_cb``` tells the ESP32 to send all raw 802.11 frames to our ```wifi_promiscuous_callback``` function.
- Channel Hopping:
  Wi-Fi routers broadcast on specific channels (1-13 for 2.4GHz). If the ESP32 is stuck on Channel 1, it will only see routers on Channel 1. The ```loop()``` function uses a timer to change the radio channel every 3 seconds, allowing it to map the entire 2.4GHz spectrum.
- Frame Control Parsing:
  The first byte of an 802.11 frame is the "Frame Control" field. The code uses bitwise shifts to extract the Type and Subtype. It explicitly drops the packet if it is a Data frame (which contains encrypted user traffic) or a Control frame.
- Information Element (IE) Parsing:
  After the 24-byte MAC header, 802.11 management frames contain a list of "Information Elements". These are tagged parameters. The code loops through these tags looking for Tag ID 0, which is the standardized identifier for the SSID.     

Educational Project Ideas    
Because this code only captures unencrypted management frames, it is perfectly safe to use for learning and legitimate IoT projects. Here are a few ways to expand this base code:     
- Wi-Fi Presence Detector: Instead of just printing to Serial, log the transmitter_mac from Probe Requests to a database or send it to an MQTT broker. You can use this to detect when specific devices (like your phone) enter the building, triggering smart home automations.
- OLED Display Integration: Connect a small I2C OLED screen (like an SSD1306) to the ESP32-S3 and display the captured SSIDs and MAC addresses in real-time, creating a portable "Wi-Fi Scanner" tool.
- Deauthentication Detection: Modify the subtype filter to look for Subtype 12 (Deauthentication). A sudden spike in these frames usually indicates a jamming attack or someone trying to force devices to reconnect to capture the WPA2 handshake. You can build an alert system for this.


## HTTP servers    
When building HTTP servers on the ESP32 using #include <WebServer.h>, the primary class is WebServer. Below is the complete API command reference organized by usage category, including parameter signatures, return types, and operational details.        

1. Server Lifecycle Commands          
- ```WebServer(int port = 80)```     
  - Arguments:     
    - port (int, optional): Listening TCP port number (default is 80).     
  - Usage: Instantiates the web server object. Usually called globally.
  - Example: ```WebServer server(80);```     
- ```begin()``` / ```begin(uint16_t port)```   
  - Arguments:   
    - port (uint16_t, optional): Overrides the port set in constructor.    
  - Usage: Binds to port and starts listening for HTTP clients. Called inside setup().   
  - Example: ```server.begin()```;   
- ```handleClient()```   
  - Arguments: None.
  - Usage: Listens for incoming connections, parses HTTP packets, and triggers assigned route handlers. Must be called continuously in ```loop()```.   
- ```stop()``` / ```close()```   
  - Arguments: None.   
  - Usage: Stops the HTTP server and releases underlying sockets.   

2. Route Registration Commands      
- ```on(path, [method], handler, [uploadHandler])```
  - Arguments:
    - path (const Uri& / String): URL path to listen for (e.g., "/", "/api/status").
    - method (HTTPMethod, optional): HTTP verb (```HTTP_GET```, ```HTTP_POST```, ```HTTP_PUT```, ```HTTP_DELETE```, ```HTTP_PATCH```).
    - handler (THandlerFunction): Callback function executed when request matches.
    - uploadHandler (THandlerFunction, optional): Secondary callback triggered during file uploads.
  - Usage: Binds a specific URI path and HTTP method to a custom C++ function.
  - Example: ```server.on("/data", HTTP_POST, handlePostData);```

- ```onNotFound(handler)```
  - Arguments:
    - handler (THandlerFunction): Callback function executed when no registered route matches the client request.\
  - Usage: Renders custom 404 Error pages or processes dynamic wildcard routes.

- ```onFileUpload(uploadHandler)```
  - Arguments:
    - uploadHandler (THandlerFunction): Callback function invoked chunk-by-chunk during multi-part form uploads.
  - Usage: Global callback setup for processing incoming file uploads.

- ```serveStatic(uri, fs, path, cache_header)```
  - Arguments:
    - uri (const char)*: Web path prefix (e.g., "/static").
    - fs (FS&): File system instance (LittleFS, SPIFFS, or SD).
    - path (const char)*: Disk folder/file path in Flash (e.g., "/www").
    - cache_header (const char, optional)*: Cache-control string (e.g., "max-age=86400").
  - Usage: Automatically serves static web pages (HTML, CSS, JS, images) stored in Flash memory.

3. Response Generation Commands       
- ```send(code, content_type, content)```
  - Arguments:
    - code (int): Standard HTTP status code (200, 400, 404, 500). See next topic for details of the code.
    - content_type (const char)*: MIME type string (e.g., "text/html", "application/json", "text/plain").
    - content (String): Body content payload.
  - Usage: Sends a full HTTP response back to the connected client.
  - Example: ```server.send(200, "application/json", "{\"status\":\"ok\"}");```

- ```send_P(code, content_type, content, contentLength)```
  - Arguments:
    - code (int): HTTP status code.
    - content_type (const char)*: MIME type.
    - content (PGM_P): Pointer to text/data stored directly in Flash memory (PROGMEM).
    - contentLength (size_t, optional): Explicit byte length of Flash array.
  - Usage: Serves large static strings from Flash without taking up RAM.

- ```sendHeader(name, value, first)```
  - Arguments:
    - name (String): Header key (e.g., "Location", "Access-Control-Allow-Origin").
    - value (String): Header value.
    - first (bool, optional): Set true to insert header at the top of stack (default false).
  - Usage: Sets custom HTTP headers. Must be called before ```server.send()```.

- ```sendContent(content)``` / ```setContentLength(length)```
  - Arguments:
    - content (String / PGM_P): Data chunk.
    - length (size_t): Total length or CONTENT_LENGTH_UNKNOWN.
- Usage: Enables chunked streaming responses when serving dynamic or large streams of data.

4. Query Parameter & Form Field Commands     
When clients submit query parameters (```/api?temp=25&unit=C```) or URL-encoded form data (POST), the server parses them automatically.     

|	Command	|	Arguments	|	Return Type	|	Usage / Description	|
|	-	|	-	|	-	|	-	|
|	```args()```	|	None	|	size_t	|	Returns total number of query or form parameters passed.	|
|	```hasArg(name)```	|	name (String)	|	bool	|	Returns true if a parameter key exists.	|
|	```arg(name)```	|	name (String)	|	String	|	Returns value for parameter key name.	|
|	```arg(index)```	|	index (int)	|	String	|	Returns parameter value by 0-based position index.	|
|	```argName(index)```	|	index (int)	|	String	|	Returns parameter key name by 0-based position index.	|     

5. Request & Connection Context Commands

|	Command	|	Arguments	|	Return Type	|	Usage / Description	|
|	-	|	-	|	-	|	-	|
|	```uri()```	|	None	|	String	|	Returns path requested by client (e.g., "```/index.html```").	|
|	```method()```	|	None	|	HTTPMethod	|	Returns HTTP verb (```HTTP_GET```, ```HTTP_POST```, etc.).	|
|	```client()```	|	None	|	WiFiClient	|	Returns active raw WiFiClient TCP stream object.	|
|	```upload()v	|	None	|	HTTPUpload&	|	Returns file upload object containing .filename, .type, .totalSize, and .status.	|

6. HTTP Header Commands       
By default, request headers are ignored for performance. Call ```collectHeaders()``` before ```begin()``` to capture specific header keys.      
- ```collectHeaders(headerKeys[], count)```
  - Arguments: headerKeys[] (const char array)*
  - count (size_t).
  - Usage: Configures server to store matching headers like User-Agent or Authorization.      

Header Inspection API     
|	Command	|	Arguments	|	Return Type	|	Usage / Description	|
|	-	|	-	|	-	|	-	|
|	```headers()```	|	None	|	int	|	Returns count of collected request headers.	|
|	```hasHeader(name)```	|	name (String)	|	bool	|	Checks if header key was received.	|
|	```header(name)```	|	name (String)	|	String	|	Retrieves collected header value by name.	|
|	```header(index)```	|	index (int)	|	String	|	Retrieves collected header value by index.	|
|	```headerName(index)```	|	index (int)	|	String	|	Retrieves name of collected header by index.	|
|	```hostHeader()```	|	None	|	String	|	Returns value of HTTP Host: header.	|

7. HTTP Authentication Commands    
- ```authenticate(username, password)```
  - Arguments: username (const char), password (const char).
  - Return Type: bool
  - Usage: Checks incoming authorization header credentials against supplied values.

- ```requestAuthentication(mode, realm, authFailMsg)```
  - Arguments:
    - mode (HTTPAuthMethod): BASIC_AUTH or DIGEST_AUTH.
    - realm (const char, optional)*: Authentication realm prompt string.
    - authFailMsg (String, optional): HTML response body if prompt is canceled.
  - Usage: Sends HTTP 401 response triggering a browser pop-up credentials prompt.

**HTTP Status Code**    

In the ```server.send(code, content_type, content)``` method from the ESP32 ```WebServer.h``` library, the code argument represents an HTTP Status Code. It tells the browser or client (like a mobile app or JavaScript fetch() call) whether the request succeeded, failed, or requires further action.

Here is the complete list of HTTP status codes commonly used with ```server.send()```, grouped by category with ESP32-specific explanations and example usage.     
1. 2xx Success (Request Processed Successfully)     
These codes tell the client that the ESP32 understood and fulfilled the request.     

|	Code	|	Status Name	|	Explanation & ESP32 Use Case	|	Example server.send() Usage	|
|	-	|	-	|	-	|	-	|
|	200	|	OK	|	Standard success response. Use when returning HTML pages, JSON sensor data, or plain text responses.	|```server.send(200,""text/html"",""<h1>Hello</h1>"");```    ```server.send(200,""application/json"", ""{\""temp\"":23.5}"");```	|
|	201	|	Created	|	The request succeeded and created a new resource on the ESP32 (e.g., creating a new config file in Flash/LittleFS or saving a new user credential).	|	```server.send(201, "application/json", "{\"status\":\"created\"}");```	|
|	204	|	No Content	|	Processed successfully, but no body content is returned. Ideal for AJAX/Fetch API calls from web UI (e.g., toggling a relay or LED) to save ESP32 memory and bandwidth.	|	```server.send(204, "text/plain", "");```	|

2. 3xx Redirection (Client Action Needed)     
These codes tell the client to navigate to another URL.

|	Code	|	Status Name	|	Explanation & ESP32 Use Case	|	Example server.send() Usage	|
|	-	|	-	|	-	|	-	|
|301|	Moved Permanently	|	The URI has moved permanently. Often used when enforcing HTTPS or setting up permanent route aliases.	|	```server.sendHeader("Location", "/new-path");```   ```server.send(301, "text/plain", "");```	|
|302|	Found (Temporary Redirect)	|	Redirects the client to another page temporarily. Used after submitting an HTML <form> to reload the main dashboard.	|	```server.sendHeader("Location", "/"); ```   ```server.send(302, "text/plain", "");```	|
|303|	See Other	|	Forces a GET redirect after a POST request. Prevents users from accidentally resubmitting form data when refreshing the page.	|	```server.sendHeader("Location", "/");``` ```server.send(303, "text/plain", "");```	|
|304|	Not Modified	|	Used with browser caching. Tells the browser that a static file stored in ESP32 Flash (e.g., style.css) hasn't changed, so use the cached copy.	|	```server.send(304, "text/plain", "");```	|

3. 4xx Client Errors (Invalid Request from Client)     
These codes mean the client sent bad input, lacks permission, or requested a non-existent route.

|	Code	|	Status Name	|	Explanation & ESP32 Use Case	|	Example server.send() Usage	|
|	-	|	-	|	-	|	-	|
|	400	|	Bad Request	|	Missing or invalid parameters. Triggered when !server.hasArg("param") or when a JSON body is invalid.	|	```server.send(400, "text/plain", "Missing 'ssid' argument");```	|
|	401	|	Unauthorized	|	Client needs to authenticate. Sent when a user fails password checks or tries to access protected endpoints without credentials.	|	```server.send(401, "text/plain", "Access Denied");```	|
|	403	|	Forbidden	|	Request was valid, but the ESP32 refuses to fulfill it (e.g., attempting to write to a read-only GPIO or system file).	|	```server.send(403, "text/plain", "GPIO Pin Restricted");```	|
|	404	|	Not Found	|	The requested URI path does not exist. Used inside server.onNotFound().	|	```server.send(404, "text/plain", "404: Page Not Found");```	|
|	405	|	Method Not Allowed	|	Client used the wrong HTTP verb (e.g., sent a GET request to an endpoint that only allows POST).	|	```server.send(405, "text/plain", "Only POST allowed");```	|
|	429	|	Too Many Requests	|	Rate limiting. Sent if a client polls the ESP32 for sensor data faster than allowed (e.g., more than once per second).	|	```server.send(429, "text/plain", "Rate limit exceeded");```	|

4. 5xx Server Errors (ESP32 Internal Failure)     
These codes mean the request was valid, but the ESP32 encountered an internal problem processing it.    

|	Code	|	Status Name	|	Explanation & ESP32 Use Case	|	Example server.send() Usage	|
|	-	|	-	|	-	|	-	|
|	500	|	Internal Server Error	|	Generic hardware/software crash. Used when LittleFS fails to open a file, I2C sensor read fails, or memory allocation fails.	|	```server.send(500, "text/plain", "Failed to read sensor on I2C");```	|
|	501	|	Not Implemented	|	Feature or HTTP verb is not supported by current ESP32 firmware.	|	```server.send(501, "text/plain", "Feature not implemented");```	|
|	503	|	Service Unavailable	|	ESP32 is temporarily unable to process the request (e.g., during an Over-The-Air (OTA) firmware update or Wi-Fi reconnecting).	|	```server.send(503, "text/plain", "ESP32 Updating Firmware...");```	|

Summary of Common Content-Type Strings     
When calling ```server.send(code, content_type, content)```, match your HTTP code with the correct MIME content_type:
```
// HTML Pages
server.send(200, "text/html", "<h1>Dashboard</h1>");

// JSON REST APIs
server.send(200, "application/json", "{\"temperature\": 24.5, \"humidity\": 60}");

// Plain Text Error Messages
server.send(400, "text/plain", "Error: Missing required parameter");

// JavaScript or CSS files from Flash Memory
server.send(200, "application/javascript", jsCodeString);
server.send(200, "text/css", cssStyleString);
```
## Raw String Literal + .replace() vs Concatenation (html += ...)     

The fundamental difference between these two approaches comes down to string formatting and how memory and variables are managed in C++.     
Instead of building an HTML string line-by-line using concatenation (```html += ...```), the code uses a C++ Raw String Literal: ```R"rawliteral(...)rawliteral"```.     
  
1. What is a Raw String Literal?     
In standard C++, characters like double quotes (```"```) and newlines (```\n```) have special meaning inside string literals. To include them in standard strings, you have to escape them with backslashes (```\"``` or ```\n```).

A raw string literal ignores all escape sequences and line breaks. Everything between ```R"rawliteral(``` and ```)rawliteral"``` is saved exactly as typed.

```
// Standard String Concatenation (Requires escaping quotes & line-by-line syntax)
String html = "<button class=\"btn\">Turn ON</button>";

// Raw String Literal (Paste raw HTML/CSS/JS directly without modification)
const char* htmlPage = R"rawliteral(
  <button class="btn">Turn ON</button>
)rawliteral";
```

2. How to Manage Variables with Raw Strings      
Because ```const char* htmlPage``` is a read-only static text block, you cannot inline C++ variables directly inside it (e.g., you can't write ```<p>Count: + myVariable + </p>``` inside the raw block).

To inject dynamic data, it uses a Template & Placeholder Pattern:

Step 1: Define Unique Placeholders in HTML     
Insert custom, unique string tags in your raw HTML where dynamic content belongs (e.g., ```%STATE%```, ```%TEMP%```, ```%HUMIDITY%```).    
```
<p><strong>Status:</strong> <span id="ledStatus">%STATE%</span></p>
<p><strong>Temperature:</strong> %TEMP% °C</p>
```

Step 2: Make a Dynamic Copy in RAM     
Inside your route handler function (e.g., ```handleRoot()```), instantiate a C++ ```String``` object initialized with the raw template. This allocates dynamic memory on the heap so it can be edited.    

```
String html = htmlPage; // Copies static template into dynamic RAM
```
Step 3: Search and Replace Placeholders      
Use the ```.replace()``` method to replace each placeholder tag with live runtime values before sending the response:     
```
void handleRoot() {
  float currentTemp = 24.5;

  String html = htmlPage; // Copy template
  
  // Replace placeholders with real variables
  html.replace("%STATE%", ledState ? "ON" : "OFF");
  html.replace("%TEMP%", String(currentTemp, 1));
  
  server.send(200, "text/html", html); // Send finished HTML
}
```

3. Advantages & Disadvantages Comparison    

|	Feature	|	Raw String Literal + ```.replace()```	|	Concatenation (```html += ...```)	|
|	-	|	-	|	-	|
|	Code Readability	|	High. Looks like clean standard HTML/CSS/JS.	|	Low. Messy with backslashes, quotes, and ```+=``` on every line.	|
|	Copy-Pasting HTML	|	Easy. Copy directly from VS Code or a web designer.	|	Hard. Requires formatting every line as C++ string syntax.	|
|	Variable Injection	|	Requires placeholder tags (```%TAG%```) and ```.replace()```.	|	Inlined directly (```html += myVar;```).	|
|	Heap Memory Impact	|	Creating a dynamic copy via ```String html = htmlPage``` allocates memory in RAM temporarily during request.	|	Repeated ```+=``` calls cause heap fragmentation over time due to frequent allocations/reallocations.	|
|	Flash vs RAM	|	Can be easily stored in Flash memory using ```PROGMEM```.	|	Kept as multiple string literals or dynamic string allocations.	|

💡 Pro Tip: Optimize RAM with ```PROGMEM```     
In the code provided, ```const char* htmlPage``` resides in RAM by default. If your HTML block gets large, it will consume valuable ESP32 RAM even when no web client is connected.

You can store the raw HTML template directly in Flash Memory (SPI Flash) by adding ```PROGMEM```:
```
// Stored in Flash Memory (saves RAM)
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
...
</html>
)rawliteral";

void handleRoot() {
  // Reads from Flash into RAM only when a client visits the page
  String html = String(htmlPage); 
  html.replace("%STATE%", ledState ? "ON" : "OFF");
  server.send(200, "text/html", html);
}
```

Full code example: ESP32 Access Point Mode: Create a Wi-Fi Hotspot to Control an LED     
https://www.oceanlabz.in/getting-started-with-esp32-wi-fi/     
```
#include <WiFi.h>
#include <WebServer.h>
 
// Access Point credentials
const char* ssid = "ESP32_Hotspot";
const char* password = "12345678";
 
// Create web server on port 80
WebServer server(80);
 
// Define LED pin (GPIO 2 for onboard LED)
const int ledPin = 2;
bool ledState = LOW;
 
// HTML content to be served
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 LED Control</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 50px; }
    .button {
      padding: 16px 36px; margin: 20px; font-size: 20px;
      border: none; border-radius: 10px; cursor: pointer;
    }
    .on { background-color: #4CAF50; color: white; }
    .on:hover { background-color: #45a049; }
    .off { background-color: #f44336; color: white; }
    .off:hover { background-color: #da190b; }
  </style>
</head>
<body>
  <h1>ESP32 LED Control</h1>
  <p><strong>Status:</strong> <span id="ledStatus">%STATE%</span></p>
  <button class="button on" onclick="location.href='/led/on'">Turn ON</button>
  <button class="button off" onclick="location.href='/led/off'">Turn OFF</button>
</body>
</html>
)rawliteral";
 
void handleRoot() {
  String html = htmlPage;
  html.replace("%STATE%", ledState ? "ON" : "OFF");
  server.send(200, "text/html", html);
}
 
void handleLedOn() {
  digitalWrite(ledPin, HIGH);
  ledState = true;
  handleRoot();
}
 
void handleLedOff() {
  digitalWrite(ledPin, LOW);
  ledState = false;
  handleRoot();
}
 
void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);
 
  // Start Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
 
  // Define routes
  server.on("/", handleRoot);
  server.on("/led/on", handleLedOn);
  server.on("/led/off", handleLedOff);
  server.begin();
}
 
void loop() {
  server.handleClient();
}
```






## ESP32 ESP-NOW     

ESP-NOW is a fast, connectionless 2.4 GHz wireless communication protocol developed by Espressif. It enables raw board-to-board data transmission without requiring a Wi-Fi router, IP address assignment, or DHCP handshakes.

By operating directly on the Data Link Layer (Layer 2) using vendor-specific IEEE 802.11 Action Frames, ESP-NOW bypasses the overhead of the TCP/IP stack.     

<img width="50%" height="auto" alt="image" src="https://github.com/user-attachments/assets/bd4fec0a-db6d-488d-adfb-3e52e9612147" />       

*ESP-NOW Protocol Model vs OSI Model. Source: Espressif Systems*

**Technical Highlights**      
- Payload Capacity: Up to 250 bytes per packet.
- Latency: Extremely low, typically ~2ms to 10ms transmission time.
- Power Consumption: Ideal for deep-sleep battery devices. A sensor can wake up, transmit data, receive confirmation, and sleep in less than 15ms.
- Security: Supports CCMP (AES-128) encryption for paired peers using a Pre-shared Master Key (PMK) and Local Master Keys (LMK).
- Capacity: Supports up to 20 unencrypted peers and 10 encrypted peers simultaneously.
- Coexistence: Can operate alongside Wi-Fi Station or Access Point modes on the same channel.
- Range: Similar to standard Wi-Fi (up to 100m – 500m+ in open space, depending on the antenna and PCB design).      


**Application Topology Modes**    
ESP-NOW supports several flexible network topologies:     
|	Topology Mode	|	Description	|	Typical Use Case	|
|	-	|	-	|	-	|
|	Unicast: One-Way (1:1)	|	One Sender transmits directly to a specific Receiver's MAC address.	|	Wireless light switch, remote control.	|
|	Broadcast: One-to-Many (1:N)	|	One Master board broadcasts or sends data to multiple Slave boards.	|	Ideal for beaconing, time synchronization, or triggering simultaneous actions. Central controller updating multiple displays/actuators. 	|
|	Many-to-One (N:1)	|	Multiple battery-powered Nodes send readings to one central Receiver Hub.	|	Distributed soil moisture or temperature sensors.	|
|	Bi-Directional (2-Way)	|	Every node registers the other as a peer, sending and receiving data back and forth.	|	Mesh-like telemetry, acknowledgment feedback systems.	|

**ESP-NOW Header Source Code**     
The header file esp_now.h is part of the Espressif ESP-IDF framework integrated directly into the Arduino ESP32 core:     
Official GitHub Link: [espressif/esp-idf — esp_now.h](https://github.com/espressif/esp-idf/blob/master/components/esp_wifi/include/esp_now.h)


**ESP-NOW API Reference**    
To use these functions, include the header in your sketch: ```#include <esp_now.h>```     

**1. Core Lifecycle APIs**    
```
esp_err_t esp_now_init(void);
```
- Usage: Initializes the ESP-NOW protocol stack. Wi-Fi mode (```WiFi.mode(...)```) must be initialized before calling ```esp_now_init()```.
- Returns: ```ESP_OK``` on success, ```ESP_ERR_ESPNOW_NOT_INIT``` or other error codes on failure.

```
esp_err_t esp_now_deinit(void);
```
- Usage: De-initializes ESP-NOW and frees associated memory buffers.

```
esp_err_t esp_now_get_version(uint32_t *version)
```
- Usage: Gets the current ESP-NOW version.     


**2. Peer Management APIs**    
Before sending a targeted message to another board, it must be registered as a peer using the ```esp_now_peer_info_t``` struct.

```
esp_err_t esp_now_add_peer(const esp_now_peer_info_t *peer);
```
- Usage: Adds a peer to the peer list. The struct requires setting ```.peer_addr``` (6-byte MAC), ```.channel``` (0–14), and ```.encrypt``` (bool).

```
esp_err_t esp_now_del_peer(const uint8_t *peer_addr);
```
- Usage: Removes a peer from the registered peer list using its 6-byte MAC address.

```
esp_err_t esp_now_mod_peer(const esp_now_peer_info_t *peer);
```
- Usage: Modifies settings for an existing peer (e.g., updating Wi-Fi channel or encryption keys).

```
bool esp_now_is_peer_exist(const uint8_t *peer_addr);
```
- Usage: Checks if a board with the specified MAC address is already registered in the peer table.
```
esp_err_t esp_now_get_peer(const uint8_t *mac_addr, esp_now_peer_info_t *peer)
```
- Usage: Retrieves the configuration of a specific peer.

```
int esp_now_get_peer_num(void)
```
- Returns the current number of peers in the list.     


**3. Data Transmission APIs**    
```
esp_err_t esp_now_send(const uint8_t *peer_addr, const uint8_t *data, size_t len);
```     

- Arguments:    
  - ```peer_addr```: Target 6-byte MAC address array (or ```NULL``` to send to all registered peers).
  - ```data```: Pointer to byte buffer array or struct cast to ```const uint8_t*```.
  - ```len```: Number of bytes to transmit (max 250 bytes).
- Usage: Transmits data packet over raw 2.4 GHz frames.

**4. Callback Registration APIs**    
```
esp_err_t esp_now_register_send_cb(esp_now_send_cb_t cb);
```
- Usage: Registers a function executed automatically when a send operation completes. Confirms whether the packet was successfully delivered (```ESP_NOW_SEND_SUCCESS```) or failed (```ESP_NOW_SEND_FAIL```).

```
esp_err_t esp_now_unregister_send_cb(void)
```
- Usage: Unregisters the send callback.

```
esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t cb);
```
- Usage: Registers a function executed whenever an incoming packet is received by the ESP32.

```
esp_err_t esp_now_unregister_recv_cb(void)
```
- Usage: Unregisters the receive callback.     



**5. Security**     
```
esp_err_t esp_now_set_pmk(const uint8_t *pmk)
```
- Usage: Sets the Primary Master Key (PMK) for encrypting the Local Master Keys (LMK). If not set, a default PMK is used.     


**Step-by-Step ESP-NOW Tutorial for ESP32-S3: One-Way Unicast (1:1) configuration**     
**Step 1**: Find the Receiver Board's MAC Address    
Upload this short utility code to your Receiver ESP32-S3 board and open the Serial Monitor (115200 baud) to copy its MAC address.    
```
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  WiFi.mode(WIFI_MODE_STA);
  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // Nothing here
}
```

**Step 2**: Sender Code (ESP32-S3)     
Paste your receiver's MAC address into the ```receiverAddress``` array below.     
```
#include <WiFi.h>
#include <esp_now.h>

// REPLACE WITH YOUR RECEIVER'S MAC ADDRESS
uint8_t receiverAddress[] = {0x24, 0xEC, 0x4A, 0x36, 0x9B, 0x70};

// Data structure to send (Must match Receiver structure)
typedef struct struct_message {
  int counter;
  float temperature;
  bool state;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// Send Callback Function
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register send callback
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;  // 0 means current Wi-Fi channel
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Sender Ready!");
}

void loop() {
  // Populate message data
  myData.counter++;
  myData.temperature = 25.4;
  myData.state = true;

  // Send packet to peer
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.printf("Sent packet #%d\n", myData.counter);
  } else {
    Serial.println("Error sending the data");
  }

  delay(2000); // Send data every 2 seconds
}
```
**Step 3**: Receiver Code (ESP32-S3)    
Upload this code to the Receiver ESP32-S3 board.     
```
#include <WiFi.h>
#include <esp_now.h>

// Data structure to receive (Must match Sender structure)
typedef struct struct_message {
  int counter;
  float temperature;
  bool state;
} struct_message;

struct_message incomingData;

// Receive Callback Function (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  
  Serial.println("--- New Message Received ---");
  Serial.printf("Bytes received: %d\n", len);
  Serial.printf("Counter: %d\n", incomingData.counter);
  Serial.printf("Temperature: %.2f °C\n", incomingData.temperature);
  Serial.printf("State: %s\n", incomingData.state ? "TRUE" : "FALSE");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register receive callback
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Receiver Listening...");
}

void loop() {
  // Nothing needed here. Incoming packets trigger OnDataRecv automatically.
}
```
**Code Example for Bi-Directional (2-Way)**    


To make two ESP32-S3 boards communicate bi-directionally, both boards need to act as both Sender and Receiver.

The most practical and reliable way to implement 2-way communication is the Request–Response (Ping-Pong) pattern:
- Board A sends a command or query packet to Board B.
- Board B receives the packet, processes it, and immediately transmits a response packet back to Board A.

**1. Board A Code (Initiator / Controller)**    
This board sends a command to Board B every 2 seconds and listens for the incoming status response.    

*Note: Replace BOARD_B_MAC with the actual MAC address of your second ESP32-S3 board.*     

```
#include <WiFi.h>
#include <esp_now.h>

// ⚠️ REPLACE WITH BOARD B's MAC ADDRESS
uint8_t boardBMac[] = {0x24, 0xEC, 0x4A, 0x36, 0x9B, 0x70};

// Shared Data Structures
typedef struct struct_command {
  int command_id;    // e.g., 1 = Toggle LED, 2 = Read Sensor
  bool trigger_state;
} struct_command;

typedef struct struct_response {
  int response_from; // Board ID
  float temp_reading;
  bool led_status;
} struct_response;

struct_command outgoingCmd;
struct_response incomingData;
esp_now_peer_info_t peerInfo;

// Delivery Confirmation Callback
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Transmission to Board B: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivered" : "Failed");
}

// Data Received Callback
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));

  Serial.println("\n📥 [Board A] Received Reply from Board B:");
  Serial.printf("  Sender ID:    Board #%d\n", incomingData.response_from);
  Serial.printf("  Temp Sensor:  %.2f °C\n", incomingData.temp_reading);
  Serial.printf("  LED Status:   %s\n", incomingData.led_status ? "ON" : "OFF");
  Serial.println("----------------------------------------");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register BOTH callbacks
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Register Board B as a Peer
  memcpy(peerInfo.peer_addr, boardBMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Board B as peer");
    return;
  }

  Serial.println("Board A (Initiator) Ready!");
}

void loop() {
  static int cmdCount = 0;
  
  // Prepare command
  outgoingCmd.command_id = ++cmdCount;
  outgoingCmd.trigger_state = (cmdCount % 2 == 0);

  Serial.printf("\n📤 [Board A] Sending Command #%d to Board B...\n", outgoingCmd.command_id);

  // Transmit command to Board B
  esp_now_send(boardBMac, (uint8_t *) &outgoingCmd, sizeof(outgoingCmd));

  delay(2000); // Send request every 2 seconds
}
```

**2. Board B Code (Responder / Node)**     
This board continuously listens for commands from Board A. As soon as a command arrives, it executes the request and transmits back a status payload.

*Note: Replace BOARD_A_MAC with the actual MAC address of your first ESP32-S3 board.*     
```
#include <WiFi.h>
#include <esp_now.h>

// ⚠️ REPLACE WITH BOARD A's MAC ADDRESS
uint8_t boardAMac[] = {0x34, 0x85, 0x18, 0x7B, 0x12, 0x40};

// Shared Data Structures
typedef struct struct_command {
  int command_id;
  bool trigger_state;
} struct_command;

typedef struct struct_response {
  int response_from;
  float temp_reading;
  bool led_status;
} struct_response;

struct_command incomingCmd;
struct_response outgoingReply;
esp_now_peer_info_t peerInfo;

const int LED_PIN = 2; // Built-in LED

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Reply Delivery to Board A: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif
  memcpy(&incomingCmd, incomingDataPtr, sizeof(incomingCmd));

  Serial.println("\n📥 [Board B] Command Received from Board A!");
  Serial.printf("  Command ID: %d | Requested State: %s\n", 
                incomingCmd.command_id, 
                incomingCmd.trigger_state ? "HIGH" : "LOW");

  // 1. Execute hardware action based on command
  digitalWrite(LED_PIN, incomingCmd.trigger_state ? HIGH : LOW);

  // 2. Prepare reply payload
  outgoingReply.response_from = 2;
  outgoingReply.temp_reading = 24.0 + (random(-10, 10) / 10.0); // Simulated sensor
  outgoingReply.led_status = incomingCmd.trigger_state;

  // 3. Immediately reply back to Board A
  Serial.println("📤 [Board B] Sending status ACK back to Board A...");
  esp_now_send(boardAMac, (uint8_t *) &outgoingReply, sizeof(outgoingReply));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Register Board A as a Peer
  memcpy(peerInfo.peer_addr, boardAMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Board A as peer");
    return;
  }

  Serial.println("Board B (Responder) Listening...");
}

void loop() {
  // Loop remains empty; responses are event-triggered in OnDataRecv
}
```

**3. Best Practices for 2-Way Communication**    
- **Avoid Simultaneous Loops**: Having both boards continuously call ```esp_now_send()``` inside their ```loop()``` functions at arbitrary timers can cause RF packet collisions. Using the event-driven Request–Reply approach above eliminates radio congestion.
- **Wi-Fi Channel Synchronization**: If one board is also connected to a home router, both boards must be pinned to the exact same Wi-Fi channel (1–13) using ```WiFi.printDiag(Serial)``` or ```esp_wifi_set_channel()```.
- **Data Alignment**: Keep identical struct definitions across both sketches to prevent memory boundary alignment issues.



**AES-128 key encryption on 2-way ESP-NOW between two ESP32-S3 boards**     
To enable hardware-accelerated AES-128 (CCMP) encryption on ESP-NOW, Espressif uses a two-tier key system:     
- Primary Master Key (PMK): A 16-byte global key used to encrypt the local master keys. Set via ```esp_now_set_pmk()```.
- Local Master Key (LMK): A 16-byte device-to-device key stored in ```peerInfo.lmk``` for a specific peer.     

Both boards must share the exact same PMK and LMK, and both must operate on the same fixed Wi-Fi channel (channel 0/auto-channel cannot negotiate encrypted handshake frames).

**Encryption Configuration Steps**    
```
// 1. Both keys MUST be exactly 16 bytes (16 chars) long
static const char PMK_KEY[] = "PMK_123456789012"; // 16 bytes
static const char LMK_KEY[] = "LMK_123456789012"; // 16 bytes

// 2. Set PMK after esp_now_init()
esp_now_set_pmk((uint8_t *)PMK_KEY);

// 3. Configure Peer with LMK and encrypt flag
memcpy(peerInfo.peer_addr, targetMac, 6);
peerInfo.channel = 1; // Fixed channel required for encryption
peerInfo.encrypt = true;
memcpy(peerInfo.lmk, LMK_KEY, 16);

// 4. Register peer
esp_now_add_peer(&peerInfo);
```

**Complete Encrypted 2-Way Examples**     
1. Board A Code (Initiator)      
*Note: Replace ```BOARD_B_MAC`` with Board B's MAC address.*    
```
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ⚠️ REPLACE WITH BOARD B's MAC ADDRESS
uint8_t boardBMac[] = {0x24, 0xEC, 0x4A, 0x36, 0x9B, 0x70};

// 16-Byte Encryption Keys
static const char PMK_KEY[] = "16BytePMKKey1234";
static const char LMK_KEY[] = "16ByteLMKKey5678";

// Data Structures
typedef struct struct_command {
  int command_id;
  bool trigger_state;
} struct_command;

typedef struct struct_response {
  int response_from;
  float temp_reading;
  bool led_status;
} struct_response;

struct_command outgoingCmd;
struct_response incomingData;
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Encrypted Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success (ACK received)" : "Fail");
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));

  Serial.println("\n📥 [Board A] Decrypted Message Received:");
  Serial.printf("  Sender ID:   Board #%d\n", incomingData.response_from);
  Serial.printf("  Temp Sensor: %.2f °C\n", incomingData.temp_reading);
  Serial.printf("  LED Status:  %s\n", incomingData.led_status ? "ON" : "OFF");
  Serial.println("----------------------------------------");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Set Wi-Fi to Station mode and pin to Channel 1
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set Primary Master Key (PMK)
  esp_now_set_pmk((uint8_t *)PMK_KEY);

  // Register Callbacks
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Configure Peer with LMK and Encryption
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, boardBMac, 6);
  peerInfo.channel = 1;      // Must match explicit Wi-Fi channel
  peerInfo.encrypt = true;    // Enable AES-128
  memcpy(peerInfo.lmk, LMK_KEY, 16);

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add encrypted peer");
    return;
  }

  Serial.println("Board A (Encrypted Initiator) Ready!");
}

void loop() {
  static int cmdCount = 0;

  outgoingCmd.command_id = ++cmdCount;
  outgoingCmd.trigger_state = (cmdCount % 2 == 0);

  Serial.printf("\n📤 [Board A] Transmitting Encrypted Cmd #%d...\n", outgoingCmd.command_id);
  esp_now_send(boardBMac, (uint8_t *) &outgoingCmd, sizeof(outgoingCmd));

  delay(2000);
}
```

2. Board B Code (Responder)    
*Note: Replace ```BOARD_A_MAC``` with Board A's MAC address.*      
```
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ⚠️ REPLACE WITH BOARD A's MAC ADDRESS
uint8_t boardAMac[] = {0x34, 0x85, 0x18, 0x7B, 0x12, 0x40};

// 16-Byte Encryption Keys (MUST MATCH BOARD A EXACTLY)
static const char PMK_KEY[] = "16BytePMKKey1234";
static const char LMK_KEY[] = "16ByteLMKKey5678";

typedef struct struct_command {
  int command_id;
  bool trigger_state;
} struct_command;

typedef struct struct_response {
  int response_from;
  float temp_reading;
  bool led_status;
} struct_response;

struct_command incomingCmd;
struct_response outgoingReply;
esp_now_peer_info_t peerInfo;

const int LED_PIN = 2;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Encrypted Reply Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif
  memcpy(&incomingCmd, incomingDataPtr, sizeof(incomingCmd));

  Serial.println("\n📥 [Board B] Decrypted Command Received!");
  Serial.printf("  Cmd ID: %d | Requested State: %s\n", 
                incomingCmd.command_id, 
                incomingCmd.trigger_state ? "HIGH" : "LOW");

  digitalWrite(LED_PIN, incomingCmd.trigger_state ? HIGH : LOW);

  // Prepare response
  outgoingReply.response_from = 2;
  outgoingReply.temp_reading = 23.5 + (random(-10, 10) / 10.0);
  outgoingReply.led_status = incomingCmd.trigger_state;

  Serial.println("📤 [Board B] Sending Encrypted ACK Reply...");
  esp_now_send(boardAMac, (uint8_t *) &outgoingReply, sizeof(outgoingReply));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);

  // Set Wi-Fi to Station mode and pin to Channel 1
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set Primary Master Key
  esp_now_set_pmk((uint8_t *)PMK_KEY);

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Configure Peer with LMK and Encryption
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, boardAMac, 6);
  peerInfo.channel = 1;      // Must match explicit Wi-Fi channel
  peerInfo.encrypt = true;    // Enable AES-128
  memcpy(peerInfo.lmk, LMK_KEY, 16);

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add encrypted peer");
    return;
  }

  Serial.println("Board B (Encrypted Responder) Listening...");
}

void loop() {
  // Empty loop; replies triggered by OnDataRecv
}
```

**Crucial Encryption Rules & Troubleshooting**    

| Rule | Why It Matters |
|------|----------------|
| **Exact 16‑Byte Array Length** | PMK and LMK strings must be exactly 16 bytes (128 bits). Shorter or longer key strings cause `esp_now_add_peer()` to return `ESP_ERR_ESPNOW_KEY_MAX`. |
| **Explicit Wi‑Fi Channel Setting** | Call `esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE)` before setup. Setting `peerInfo.channel = 0` (auto channel) causes encryption key exchange to fail. |
| **Peer Capacity Limit** | Unencrypted ESP‑NOW supports up to 20 total peers. Encrypted peers are limited to 6 peers maximum due to hardware key storage registers. |
| **`memset(&peerInfo, 0, sizeof(peerInfo))`** | Always zero out the `peerInfo` structure before populating fields to avoid garbage memory in security parameters. |


https://www.luisllamas.es/que-es-esp-now-esp32/

https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/

## ESP32 MQTT     

https://www.luisllamas.es/como-usar-mqtt-en-el-esp8266-esp32/

https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/



## Further Reading     

[ESP32 WiFi Tutorial & Library Examples (Arduino IDE)](https://deepbluembedded.com/esp32-wifi-library-examples-tutorial-arduino/)

[Building an ESP32 Web Server: The Complete Guide for Beginners](https://randomnerdtutorials.com/esp32-web-server-beginners-guide/)
- https://github.com/RuiSantosdotme/build-web-servers-dl

https://deepbluembedded.com/esp32-wifi-library-examples-tutorial-arduino/

https://www.oceanlabz.in/getting-started-with-esp32-wi-fi/

https://www.oceanlabz.in/esp32-web-server-with-multiple-sliders-control-leds-brightness/

https://www.oceanlabz.in/esp32-web-server-control-outputs-with-timer/

## Reference

https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFi.h

https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi/examples

https://github.com/espressif/arduino-esp32/blob/master/libraries/WebServer/src/WebServer.h

https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer/examples






