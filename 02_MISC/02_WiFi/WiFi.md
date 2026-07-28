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
|  ```WiFi.mode(WIFI_STA)``` | Sets mode to Station. |
|	WiFi.begin(ssid, password)	|	Connects to a Wi-Fi network.	|
|	WiFi.status()	|	Returns connection status (e.g., WL_CONNECTED, WL_DISCONNECTED).	|
|	WiFi.isConnected()	|	Returns true if connected to an AP.	|
|	WiFi.localIP()	|	Returns the IP address (IPv4) assigned to the ESP32.	|
|	WiFi.macAddress()	|	Returns the MAC address of the ESP32.	|
|	WiFi.RSSI()	|	Returns the signal strength in dBm of the connected network.	|
|	WiFi.disconnect()	|	Disconnects from the current Wi-Fi network.	|
|	WiFi.setAutoReconnect(bool)	|	Enables/disables automatic reconnection.	|

**Access Point (AP) Mode APIs**    
|	Function	|	Description	|
|	-	|	-	|
| WiFi.mode(WIFI_AP) | Sets mode to Access Point. |
|	WiFi.softAP(ssid, pass, ch, [hide], [max])	|	Starts an Access Point. ch=channel, hide=hidden SSID, max=max clients.	|
|	WiFi.softAPIP()	|	Returns the IP address of the ESP32 in AP mode (usually 192.168.4.1).	|
|	WiFi.softAPmacAddress()	|	Returns the MAC address of the SoftAP.	|
|	WiFi.softAPgetStationNum()	|	Returns the number of devices currently connected to the SoftAP.	|
|  WiFi.softAPgetConnectedStations() |  Returns count of connected clients. |
|	WiFi.softAPdisconnect(wifi_off)	|	Disconnects all clients and shuts down the SoftAP.	|

**Network Scanning APIs**
|	Function	|	Description	|
|	-	|	-	|
|	WiFi.scanNetworks()	|	Scans for available networks. Returns the number of networks found.	|
|	WiFi.SSID(index)	|	Returns the SSID of the network at the scanned index.	|
|	WiFi.RSSI(index)	|	Returns the signal strength in dBm of the network at the scanned index.	|



## Reference

https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFi.h

https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi/examples

