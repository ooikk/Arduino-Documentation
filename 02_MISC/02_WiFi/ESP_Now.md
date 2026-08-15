## ESP32 ESP-NOW     

ESP-NOW is a fast, connectionless 2.4 GHz wireless communication protocol developed by Espressif. It enables raw board-to-board data transmission without requiring a Wi-Fi router, IP address assignment, or DHCP handshakes.

By operating directly on the Data Link Layer (Layer 2) using vendor-specific IEEE 802.11 Action Frames, ESP-NOW bypasses the overhead of the TCP/IP stack.     

<img width="75%" height="auto" alt="image" src="https://github.com/user-attachments/assets/bd4fec0a-db6d-488d-adfb-3e52e9612147" />       

*ESP-NOW Protocol Model vs OSI Model. Source: Espressif Systems*

### Technical Highlights      
- Payload Capacity: Up to 250 bytes per packet.
- Latency: Extremely low, typically ~2ms to 10ms transmission time.
- Power Consumption: Ideal for deep-sleep battery devices. A sensor can wake up, transmit data, receive confirmation, and sleep in less than 15ms.
- Security: Supports CCMP (AES-128) encryption for paired peers using a Pre-shared Master Key (PMK) and Local Master Keys (LMK).
- Capacity: Supports up to 20 unencrypted peers and 10 encrypted peers simultaneously.
- Coexistence: Can operate alongside Wi-Fi Station or Access Point modes on the same channel.
- Range: Similar to standard Wi-Fi (up to 100m – 500m+ in open space, depending on the antenna and PCB design).      

**Note:**    
To transmit text strings over ESP-NOW, you must embed a fixed-size character array inside the struct. Example ```char message[180];``` below:
```cpp
typedef struct struct_message {
  int counter;          // 4 bytes
  float temperature;    // 4 bytes
  bool state;           // 1 byte
  char message[180];    // Fixed array buffer (keeps total struct under 250 bytes)
} struct_message;
```
Important ESP-NOW Limit: The total size of struct_message cannot exceed 250 bytes (the hard hardware payload limit for an ESP-NOW frame).

### Application Topology Modes    
ESP-NOW supports several flexible network topologies:     
|	Topology Mode	|	Description	|	Typical Use Case	|
|	-	|	-	|	-	|
|	Unicast: One-Way (1:1)	|	One Sender transmits directly to a specific Receiver's MAC address.	|	Wireless light switch, remote control.	|
|	Broadcast: One-to-Many (1:N)	|	One Master board broadcasts or sends data to multiple Slave boards.	|	Ideal for beaconing, time synchronization, or triggering simultaneous actions. Central controller updating multiple displays/actuators. 	|
|	Many-to-One (N:1)	|	Multiple battery-powered Nodes send readings to one central Receiver Hub.	|	Distributed soil moisture or temperature sensors.	|
|	Bi-Directional (2-Way)	|	Every node registers the other as a peer, sending and receiving data back and forth.	|	Mesh-like telemetry, acknowledgment feedback systems.	|

### ESP-NOW Header Source Code     
The header file esp_now.h is part of the Espressif ESP-IDF framework integrated directly into the Arduino ESP32 core:     
Official GitHub Link: [espressif/esp-idf — esp_now.h](https://github.com/espressif/esp-idf/blob/master/components/esp_wifi/include/esp_now.h)


### ESP-NOW API Reference    
To use these functions, include the header in your sketch: ```#include <esp_now.h>```     

**1. Core Lifecycle APIs**    
```cpp
esp_err_t esp_now_init(void);
```
- Usage: Initializes the ESP-NOW protocol stack. Wi-Fi mode (```WiFi.mode(...)```) must be initialized before calling ```esp_now_init()```.
- Returns: ```ESP_OK``` on success, ```ESP_ERR_ESPNOW_NOT_INIT``` or other error codes on failure.

```cpp
esp_err_t esp_now_deinit(void);
```
- Usage: De-initializes ESP-NOW and frees associated memory buffers.

```cpp
esp_err_t esp_now_get_version(uint32_t *version)
```
- Usage: Gets the current ESP-NOW version.     


**2. Peer Management APIs**    
Before sending a targeted message to another board, it must be registered as a peer using the ```esp_now_peer_info_t``` struct.       
This is ```esp_now_peer_info_t``` struct.     
```cpp
typedef struct esp_now_peer_info {
    uint8_t peer_addr[ESP_NOW_ETH_ALEN];    /**< ESPNOW peer MAC address that is also the MAC address of station or softap */
    uint8_t lmk[ESP_NOW_KEY_LEN];           /**< ESPNOW peer local master key that is used to encrypt data */
    uint8_t channel;                        /**< Wi-Fi channel that peer uses to send/receive ESPNOW data. If the value is 0,
                                                 use the current channel which station or softap is on. Otherwise, it must be
                                                 set as the channel that station or softap is on. */
    wifi_interface_t ifidx;                 /**< Wi-Fi interface that peer uses to send/receive ESPNOW data */
    bool encrypt;                           /**< ESPNOW data that this peer sends/receives is encrypted or not */
    void *priv;                             /**< ESPNOW peer private data */
} esp_now_peer_info_t;
```

```cpp
esp_err_t esp_now_add_peer(const esp_now_peer_info_t *peer);
```
- Usage: Adds a peer to the peer list. The struct requires setting ```.peer_addr``` (6-byte MAC), ```.channel``` (0–14), and ```.encrypt``` (bool).

```cpp
esp_err_t esp_now_del_peer(const uint8_t *peer_addr);
```
- Usage: Removes a peer from the registered peer list using its 6-byte MAC address.

```cpp
esp_err_t esp_now_mod_peer(const esp_now_peer_info_t *peer);
```
- Usage: Modifies settings for an existing peer (e.g., updating Wi-Fi channel or encryption keys).

```cpp
bool esp_now_is_peer_exist(const uint8_t *peer_addr);
```
- Usage: Checks if a board with the specified MAC address is already registered in the peer table.
```cpp
esp_err_t esp_now_get_peer(const uint8_t *mac_addr, esp_now_peer_info_t *peer)
```
- Usage: Retrieves the configuration of a specific peer.

```cpp
int esp_now_get_peer_num(void)
```
- Returns the current number of peers in the list.     


**3. Data Transmission APIs**    
```cpp
esp_err_t esp_now_send(const uint8_t *peer_addr, const uint8_t *data, size_t len);
```     

- Arguments:    
  - ```peer_addr```: Target 6-byte MAC address array (or ```NULL``` to send to all registered peers).
  - ```data```: Pointer to byte buffer array or struct cast to ```const uint8_t*```.
  - ```len```: Number of bytes to transmit (max 250 bytes).
- Usage: Transmits data packet over raw 2.4 GHz frames.

**4. Callback Registration APIs**    
```cpp
esp_err_t esp_now_register_send_cb(esp_now_send_cb_t cb);
```
- Usage: Registers a function executed automatically when a send operation completes. Confirms whether the packet was successfully delivered (```ESP_NOW_SEND_SUCCESS```) or failed (```ESP_NOW_SEND_FAIL```).

```cpp
esp_err_t esp_now_unregister_send_cb(void)
```
- Usage: Unregisters the send callback.

```cpp
esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t cb);
```
- Usage: Registers a function executed whenever an incoming packet is received by the ESP32.

```cpp
esp_err_t esp_now_unregister_recv_cb(void)
```
- Usage: Unregisters the receive callback.     



**5. Security**     
```cpp
esp_err_t esp_now_set_pmk(const uint8_t *pmk)
```
- Usage: Sets the Primary Master Key (PMK) for encrypting the Local Master Keys (LMK). If not set, a default PMK is used.     


### Step-by-Step ESP-NOW Tutorial for ESP32-S3: One-Way Unicast (1:1) configuration     
**Step 1**: Find the Receiver Board's MAC Address    
Upload this short utility code to your Receiver ESP32-S3 board and open the Serial Monitor (115200 baud) to copy its MAC address.    
```cpp
#include <WiFi.h>
// for option 2
#include <esp_wifi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_MODE_STA);
  delay(100);  // Give the Wi-Fi driver time to power on and load registers

  // Option 1: Standard Arduino approach
  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Option 2: Direct ESP-IDF hardware query (bypasses standard wrapper)
  uint8_t rawMac[6];
  esp_wifi_get_mac(WIFI_IF_STA, rawMac);
  Serial.printf("Direct MAC:   %02X:%02X:%02X:%02X:%02X:%02X\n", 
                rawMac[0], rawMac[1], rawMac[2], rawMac[3], rawMac[4], rawMac[5]);

}

void loop() {
  // Nothing here
}

```

**Step 2**: Sender Code (ESP32-S3)     
Paste your receiver's MAC address into the ```receiverAddress``` array below.     
```cpp
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
// Receive Callback Function (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
// Send Callback Function (Updated for ESP32 Arduino Core 3.x)
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
// Send Callback Function (Updated for ESP32 Arduino Core 2.x)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif

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
```cpp
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
**Note**: In ESP-NOW unicast mode, ```esp_err_t!=ESP_OK``` or "Delivery Fail" means the sender's radio transmitted the packet, but never received a hardware ACK frame back from the target receiver.    

Unlike standard Wi-Fi, ESP-NOW unicast requires the receiving node's radio to acknowledge the packet within milliseconds over the air.

### ESP32 Core v3.x and Core v2.x     

The core difference is that v2.x only provided the sender's MAC address, while v3.x provides a rich context structure that includes the sender's MAC, the destination MAC, and the Wi-Fi channel the packet was received on.

Here is the detailed technical breakdown of the differences.

#### 1. Core v2.x: The Legacy Approach (`const uint8_t * mac`)    

In v2.x, the callback signature was:

```cpp
// Example placeholder – replace with your actual v2.x signature
void OnDataReceived(const uint8_t * mac, const uint8_t *incomingData, int len);
```

- **What it is**: `mac` is simply a raw pointer to a 6-byte array representing the **Source MAC address** (the MAC address of the device that sent the packet).
- **Limitation**: It lacked context. If your ESP32 was running both Station (STA) and SoftAP modes simultaneously, or if it was scanning multiple channels, the callback had no way to tell you which interface received the packet or which channel it arrived on.

#### 2. Core v3.x: The Modern Approach (`const esp_now_recv_info_t *info`)    

In v3.x, the callback signature was updated to:

```cpp
// Example placeholder – replace with your actual v3.x signature
void OnDataReceived(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len);
```

Instead of a simple MAC pointer, it passes a pointer to a new structure called `esp_now_recv_info_t`.

#### The `esp_now_recv_info_t` Structure   

In Arduino ESP32 Core v3.x (ESP-IDF v5.x), the `esp_now_recv_info_t` structure is defined in `esp_now.h` as follows:

```cpp
typedef struct esp_now_recv_info {
    uint8_t *src_addr;            /**< Source MAC address (6 bytes) */
    uint8_t *des_addr;            /**< Destination MAC address (6 bytes) */
    wifi_pkt_rx_ctrl_t *rx_ctrl;  /**< Rx control metadata header */
} esp_now_recv_info_t;
```

---

#### Breakdown of Struct Fields

**1. `src_addr` (`uint8_t *`)**    
Pointer to the 6-byte array containing the **transmitter's (sender's) MAC address**.
* **Accessing:** `recv_info->src_addr[0]` through `recv_info->src_addr[5]`

**2. `des_addr` (`uint8_t *`)**     
Pointer to the 6-byte array containing the **intended recipient's MAC address** (your receiver's MAC or `FF:FF:FF:FF:FF:FF` for broadcast packets).
* **Accessing:** `recv_info->des_addr[0]` through `recv_info->des_addr[5]`

**3. `rx_ctrl` (`wifi_pkt_rx_ctrl_t *`)**     
Pointer to a nested Wi-Fi PHY control structure containing low-level radio metadata about the received frame. Key members inside `rx_ctrl` include:

| Field | Type | Description |
| :--- | :--- | :--- |
| `rx_ctrl->rssi` | `int` | **Received Signal Strength Indicator** in dBm (e.g., `-65`) |
| `rx_ctrl->channel` | `unsigned` | Primary Wi-Fi channel the packet was received on |
| `rx_ctrl->timestamp` | `uint32_t` | Microsecond hardware timer tick when packet arrived |
| `rx_ctrl->rate` | `unsigned` | PHY bit-rate encoding |
| `rx_ctrl->sig_mode` | `unsigned` | Signal mode (`0`: Non-HT, `1`: HT20, `2`: HT40, etc.) |

---

**Practical Example**     

Here is how to extract and print all fields from `recv_info` inside your receive callback:

```cpp
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
  // 1. Extract Source MAC
  const uint8_t *src = recv_info->src_addr;
  Serial.printf("Sender MAC : %02X:%02X:%02X:%02X:%02X:%02X\n",
                src[0], src[1], src[2], src[3], src[4], src[5]);

  // 2. Extract Destination MAC
  const uint8_t *dst = recv_info->des_addr;
  Serial.printf("Target MAC : %02X:%02X:%02X:%02X:%02X:%02X\n",
                dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]);

  // 3. Extract Radio Telemetry (RSSI & Channel)
  if (recv_info->rx_ctrl != NULL) {
    Serial.printf("Signal RSSI: %d dBm\n", recv_info->rx_ctrl->rssi);
    Serial.printf("Channel    : %d\n", recv_info->rx_ctrl->channel);
  }
}
```


**Field Breakdown**   

- `info->src_mac[6]`: This is the direct replacement for the old `mac` pointer. It holds the **MAC address of the sender**.
- `info->des_mac[6]`: The **destination MAC address**. This is crucial for devices operating in SoftAP mode or STA+AP concurrent mode, as the device has multiple MAC addresses. This tells you exactly which of your device's interfaces the packet was addressed to.
- `info->channel`: The **Wi-Fi channel** (1–13/14) on which the packet was received. This is highly useful if your application implements channel hopping or if you are running a sniffer/monitor node.


#### 3. How to Migrate Your Code (v2.x to v3.x)  

If you are updating an old sketch to work with Arduino Core v3.x, you only need to change the callback function signature and how you access the MAC address.

**Old Code (v2.x)**

```cpp
// Old v2.x style callback
void OnDataReceived(const uint8_t * mac, const uint8_t *incomingData, int len) {
    // mac is a pointer to 6-byte source MAC
    Serial.printf("From: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Process incomingData...
}
```

**New Code (v3.x)**

```cpp
// New v3.x style callback
void OnDataReceived(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    // info->src_mac replaces the old mac pointer
    Serial.printf("From: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  info->src_addr[0], info->src_addr[1], info->src_addr[2],
                  info->src_addr[3], info->src_addr[4], info->src_addr[5]);

    // Optional: use destination MAC and channel
    // info->src_addr
    // info->wifi_pkt_rx_ctrl_t

    // Process incomingData...
}
```



**Summary Table**

| Feature | Core v2.x (`const uint8_t * mac`) | Core v3.x (`const esp_now_recv_info_t * info`) |
|---------|-----------------------------------|------------------------------------------------|
| Sender MAC | `mac` | `info->src_addr` |
| Receiver/Dest MAC | Not available | `info->des_addr` |
| RX Wi‑Fi Channel | Not available | `info->rx_ctrl->channel` |
| Multi‑Interface Support | Poor (Ambiguous) | Excellent (Explicitly identifies interface) |
| Underlying ESP‑IDF | v4.4 and older | v5.1 and newer |

### Code Example for Bi-Directional (2-Way)    


To make two ESP32-S3 boards communicate bi-directionally, both boards need to act as both Sender and Receiver.

The most practical and reliable way to implement 2-way communication is the Request–Response (Ping-Pong) pattern:
- Board A sends a command or query packet to Board B.
- Board B receives the packet, processes it, and immediately transmits a response packet back to Board A.

**1. Board A Code (Initiator / Controller)**    
This board sends a command to Board B every 2 seconds and listens for the incoming status response.    

*Note: Replace ```BOARD_B_MAC``` with the actual MAC address of your second ESP32-S3 board.*     

```cpp
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

// Delivery Confirmation Callback (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
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

*Note: Replace ```BOARD_A_MAC``` with the actual MAC address of your first ESP32-S3 board.*     
```cpp
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

// Delivery Confirmation Callback (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
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



### AES-128 key encryption on 2-way ESP-NOW between two ESP32-S3 boards     
To enable hardware-accelerated AES-128 (CCMP) encryption on ESP-NOW, Espressif uses a two-tier key system:     
- Primary Master Key (PMK): A 16-byte global key used to encrypt the local master keys. Set via ```esp_now_set_pmk()```.
- Local Master Key (LMK): A 16-byte device-to-device key stored in ```peerInfo.lmk``` for a specific peer.     

Both boards must share the exact same PMK and LMK, and both must operate on the same fixed Wi-Fi channel (channel 0/auto-channel cannot negotiate encrypted handshake frames).

**Encryption Configuration Steps**    
```cpp
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
*Note: Replace ```BOARD_B_MAC``` with Board B's MAC address.*    
```cpp
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

// Delivery Confirmation Callback (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
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
*Note: Replace `BOARD_A_MAC` with Board A's MAC address.*      
```cpp
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

// Delivery Confirmation Callback (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
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

### Encrypted ESP-NOW with ESP32-S3 Deep Sleep for battery-powered sensor nodes

When using standard Wi-Fi, an ESP32 must wake up, scan for networks, negotiate WPA2 authentication, and wait for a DHCP IP assignment—a process that takes **2 to 5 seconds** at ~120 mA current draw.

With **Encrypted ESP-NOW + Deep Sleep**, the ESP32-S3 wakes up, sets its radio directly to a fixed Wi-Fi channel, transmits an encrypted payload, receives hardware acknowledgment, and goes back to deep sleep in **less than 50 milliseconds**. This provides a **~99% reduction** in active energy consumption, allowing a single LiPo battery to power a sensor node for months or years.

**Technical Execution Flow**    

```text
   [Deep Sleep (~10µA)]
            │
   (Timer / RTC Wakeup)
            │
            ▼
┌───────────────────────┐
│ 1. Read Sensor Data   │ (~1-5 ms)
└───────────┬───────────┘
            │
┌───────────▼───────────┐
│ 2. Init Radio & Keys  │ (~20 ms)
│    (WIFI_STA + Channel│
│     PMK + LMK Peer)   │
└───────────┬───────────┘
            │
┌───────────▼───────────┐
│ 3. Send Encrypted     │ (~5-10 ms)
│    ESP-NOW Packet     │
└───────────┬───────────┘
            │
┌───────────▼───────────┐
│ 4. Wait for ACK /     │ (~2-5 ms)
│    Timeout Safeguard  │
└───────────┬───────────┘
            │
┌───────────▼───────────┐
│ 5. Enter Deep Sleep   │ (Back to ~10µA)
└───────────────────────┘
```

**Complete Sensor Node Code Example**     

This code wakes up every 10 seconds (configurable), reads a simulated temperature sensor, increments a persistent boot counter stored in RTC memory, transmits the encrypted payload to the receiver board, and goes back to sleep immediately.


```cpp
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ====================================================================
// DEEP SLEEP & RTC CONFIGURATION
// ====================================================================
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10          /* Time ESP32 will go to sleep (in seconds) */

// Persistent RTC memory variable (retained across deep sleep cycles)
RTC_DATA_ATTR int bootCount = 0;

// ====================================================================
// ENCRYPTION & PEER CONFIGURATION
// ====================================================================
// ⚠️ REPLACE WITH YOUR RECEIVER BOARD'S MAC ADDRESS
uint8_t receiverAddress[] = {0x24, 0xEC, 0x4A, 0x36, 0x9B, 0x70};

// 16-Byte AES Keys (MUST match receiver exactly)
static const char PMK_KEY[] = "16BytePMKKey1234";
static const char LMK_KEY[] = "16ByteLMKKey5678";

// Payload Structure
typedef struct struct_sensor_data {
  int boot_number;
  float temperature;
  float battery_voltage;
} struct_sensor_data;

struct_sensor_data sensorPayload;
esp_now_peer_info_t peerInfo;

// Flag to track transmission status
volatile bool sendCompleted = false;

// Delivery Confirmation Callback (Compatible with ESP32 Core v2.x and v3.x)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif

  Serial.print("Encrypted Delivery Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success (ACK)" : "Fail");
  sendCompleted = true;
}

void setup() {
  Serial.begin(115200);
  
  // 1. Increment persistent boot counter
  bootCount++;
  Serial.printf("\n--- Wakeup #%d ---\n", bootCount);

  // 2. Read Sensor Data (Keep code fast!)
  sensorPayload.boot_number = bootCount;
  sensorPayload.temperature = 22.0 + (random(0, 100) / 10.0); // Simulated sensor
  sensorPayload.battery_voltage = 3.72;                        // Simulated ADC reading

  // 3. Initialize Wi-Fi Station Mode & Pin Channel
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // Fixed Channel 1 required for AES

  // 4. Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed! Sleeping...");
    goToSleep();
  }

  // 5. Set Primary Master Key (PMK)
  esp_now_set_pmk((uint8_t *)PMK_KEY);

  // 6. Register Send Callback
  esp_now_register_send_cb(OnDataSent);

  // 7. Add Encrypted Peer
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 1;      // Must match explicit channel set above
  peerInfo.encrypt = true;    // Enable AES-128
  memcpy(peerInfo.lmk, LMK_KEY, 16);

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer! Sleeping...");
    goToSleep();
  }

  // 8. Transmit Encrypted Payload
  Serial.printf("Sending Payload: Boot #%d | Temp: %.1f°C\n", 
                sensorPayload.boot_number, sensorPayload.temperature);
                
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&sensorPayload, sizeof(sensorPayload));

  if (result != ESP_OK) {
    Serial.println("Send Initiation Failed!");
    goToSleep();
  }

  // 9. Wait for hardware ACK callback with a short 500ms safeguard timeout
  uint32_t startTime = millis();
  while (!sendCompleted && (millis() - startTime < 500)) {
    delay(1); // Short pause to keep CPU waiting efficiently
  }

  // 10. Enter Deep Sleep immediately
  goToSleep();
}

void goToSleep() {
  Serial.println("Entering Deep Sleep now...");
  Serial.flush(); // Ensure serial logs are printed before power down

  // Enable timer wakeup
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  
  // Turn off Wi-Fi radio explicitly for maximum power saving
  esp_wifi_stop();
  
  // Enter Deep Sleep
  esp_deep_sleep_start();
}

void loop() {
  // Never reached! Execution re-enters setup() on wakeup.
}
```

**Power Optimization Highlights in This Code**    

*   **RTC_DATA_ATTR Variable:** Stores `bootCount` in the RTC slow memory that stays powered during deep sleep. When the chip wakes up, variables marked with `RTC_DATA_ATTR` retain their values without being reset.
*   **Empty `loop()`:** Deep sleep turns off the main dual CPUs. When the wake-up timer fires, the ESP32 performs a cold boot and runs `setup()` from the top. Putting all logic in `setup()` eliminates unnecessary loop iterations.
*   **ACK Timeout Safeguard:** The `while (!sendCompleted)` loop waits for the physical 2.4 GHz ACK callback from the receiver board. The 500ms safety timeout prevents the board from hanging and draining battery if the receiver board is turned off or out of range.
*   **`esp_wifi_stop()`:** Calling `esp_wifi_stop()` right before `esp_deep_sleep_start()` forces the Wi-Fi modem and radio synthesizers into full power-down mode immediately.


**Receiver Compatibility**     

The receiver board uses standard encrypted ESP-NOW code (matching `PMK_KEY` and `LMK_KEY` on Channel 1). It remains powered on as a gateway and receives messages whenever the sleep node wakes up and transmits.


**Encrypted ESP-NOW Receiver Designs**    

To receive encrypted data from a deep-sleep sensor node, the receiver design depends on your power setup:

1.  **Always-On Gateway (Recommended & Most Common):** The receiver stays powered continuously so it can catch incoming packets from the sleeping sensor node at any moment.
2.  **Synchronized Receiver Node:** The receiver also enters deep sleep and wakes up during aligned time windows to listen for a short burst before sleeping again.

**1. Always-On Encrypted Receiver (Gateway)**      

Because ESP-NOW AES-128 encryption requires hardware key handshake verification, the receiver **MUST** register the sender's MAC address as an encrypted peer with the exact same PMK and LMK keys.

> **Note:** Replace `SENDER_MAC` with the actual MAC address of your deep-sleep sensor node.


```cpp
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ⚠️ REPLACE WITH YOUR SENDER NODE'S MAC ADDRESS
uint8_t senderAddress[] = {0x34, 0x85, 0x18, 0x7B, 0x12, 0x40};

// 16-Byte AES Keys (MUST MATCH SENDER EXACTLY)
static const char PMK_KEY[] = "16BytePMKKey1234";
static const char LMK_KEY[] = "16ByteLMKKey5678";

// Payload Structure (MUST MATCH SENDER EXACTLY)
typedef struct struct_sensor_data {
  int boot_number;
  float temperature;
  float battery_voltage;
} struct_sensor_data;

struct_sensor_data incomingSensorData;
esp_now_peer_info_t peerInfo;

// Receive Callback (Fires instantly when decrypted packet arrives)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
  const uint8_t *mac = recv_info->src_addr;
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif

  memcpy(&incomingSensorData, incomingDataPtr, sizeof(incomingSensorData));

  Serial.println("==================================================");
  Serial.printf("📥 Decrypted Packet Received from MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("  Boot / Transmission Count : #%d\n", incomingSensorData.boot_number);
  Serial.printf("  Temperature               : %.2f °C\n", incomingSensorData.temperature);
  Serial.printf("  Battery Voltage           : %.2f V\n", incomingSensorData.battery_voltage);
  Serial.println("==================================================\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1. Set Wi-Fi Station Mode & Pin to Channel 1 (Must match sender)
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  // 2. Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // 3. Set Primary Master Key (PMK)
  esp_now_set_pmk((uint8_t *)PMK_KEY);

  // 4. Register Receive Callback
  esp_now_register_recv_cb(OnDataRecv);

  // 5. Register Deep-Sleep Sender as an Encrypted Peer
  // (Required for ESP32 hardware AES to decrypt frames from this specific MAC)
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, senderAddress, 6);
  peerInfo.channel = 1;      // Fixed Channel 1
  peerInfo.encrypt = true;    // Enable AES-128 Decryption
  memcpy(peerInfo.lmk, LMK_KEY, 16);

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add encrypted sender peer!");
    return;
  }

  Serial.println("Encrypted ESP-NOW Gateway Listening on Channel 1...");
}

void loop() {
  // Main loop remains free for processing, MQTT bridging, SD logging, etc.
}
```

**2. Low-Power "Synchronized Window" Receiver**     

If your receiver must also run on a battery and use Deep Sleep:

*   Both boards use the same timer interval (e.g., wake up every 10 seconds).
*   On wake-up, the receiver keeps its radio active for a **100 ms listening window**.
*   If data is received (or the window expires), the receiver logs the data and returns to sleep.


```cpp
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP  10     // Sleep 10 seconds (Matches sender interval)
#define LISTEN_WINDOW  150    // Listen for 150ms before giving up

uint8_t senderAddress[] = {0x34, 0x85, 0x18, 0x7B, 0x12, 0x40};
static const char PMK_KEY[] = "16BytePMKKey1234";
static const char LMK_KEY[] = "16ByteLMKKey5678";

typedef struct struct_sensor_data {
  int boot_number;
  float temperature;
  float battery_voltage;
} struct_sensor_data;

struct_sensor_data incomingSensorData;
esp_now_peer_info_t peerInfo;
volatile bool dataReceived = false;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataPtr, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
#endif
  memcpy(&incomingSensorData, incomingDataPtr, sizeof(incomingSensorData));
  dataReceived = true;
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) { goToSleep(); }

  esp_now_set_pmk((uint8_t *)PMK_KEY);
  esp_now_register_recv_cb(OnDataRecv);

  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, senderAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = true;
  memcpy(peerInfo.lmk, LMK_KEY, 16);
  esp_now_add_peer(&peerInfo);

  // 100ms Synchronized RX Listening Window
  uint32_t startWindow = millis();
  while (!dataReceived && (millis() - startWindow < LISTEN_WINDOW)) {
    delay(1);
  }

  if (dataReceived) {
    Serial.printf("📥 Rx Boot #%d | Temp: %.1f°C | Batt: %.2fV\n",
                  incomingSensorData.boot_number,
                  incomingSensorData.temperature,
                  incomingSensorData.battery_voltage);
  } else {
    Serial.println("⚠️ Listen window timed out. No packet caught.");
  }

  goToSleep();
}

void goToSleep() {
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_wifi_stop();
  esp_deep_sleep_start();
}

void loop() {}
```

**Critical Checkpoints for Encrypted ESP-NOW**     

*   **Peer Registration is Required:** Unlike unencrypted promiscuous listening, the receiver must add the sender's MAC address with `esp_now_add_peer()` and set `peerInfo.encrypt = true`. Otherwise, incoming encrypted frames will be dropped at the hardware PHY level.
*   **Matching Channel & Keys:** PMK, LMK, and the Wi-Fi channel (1) must match byte-for-byte across both sketches.
*   **Data Struct Alignment:** Ensure field order and types (`int`, `float`) are identical on both sides.

https://www.luisllamas.es/que-es-esp-now-esp32/

https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/

### Many-to-One (N:1) - Star Topology: Encrypted ESP-NOW communication with deep sleep nodes

An ESP32 in deep sleep completely powers off its Wi-Fi radio and cannot hear incoming ESP-NOW transmissions.

To put both the Gateway and the Sensor Nodes into deep sleep, you must implement **Time-Synchronized Wake Windows (Duty Cycling)**:

- **Gateway:** Wakes up on a fixed schedule (e.g., every 60 seconds) and leaves its radio open for a brief 1–2 second window.
- **Sensor Nodes:** Wake up at the exact same interval, burst their encrypted payloads to the Gateway during this window, collect replies, and go back to sleep.
- **Gateway:** Closes its listening window after processing all incoming nodes and returns to deep sleep.

#### Critical Design Considerations for Dual Deep Sleep

**Clock Drift Buffer (`LISTEN_WINDOW_MS`)**    

Internal ESP32 RTC timers drift by ~1% to 5% due to temperature fluctuations. Setting `LISTEN_WINDOW_MS = 1500` ensures the Gateway wakes up early enough and stays awake long enough to catch nodes whose internal clocks run slightly slower or faster.

**Synchronized Cycles**     

Both Gateway (`GATEWAY_SLEEP_SEC`) and Sensor Nodes (`currentSleepSec`) must use the exact same sleep interval (e.g., 60 seconds).

**Battery Savings**   

The Gateway's average active time drops from 100% (always on) down to ~2.5% (awake 1.5s every 60s), making it suitable for solar/battery power.

#### Important Encrypted Deep Sleep Rules

**LMK Key Requirements**   

The LMK key must be exactly 16 bytes (128 bits). If the key length is incorrect, peer registration returns `ESP_ERR_ESPNOW_KEY_IF`.

**Max Encrypted Peers**    

Standard ESP32 hardware supports up to 6 encrypted peers simultaneously in legacy modes, while ESP32-S3 supports up to 17 encrypted peers. Because the nodes only pair with 1 Gateway, you can scale up to hundreds of sleeping sensor nodes; only the central Gateway needs to manage/rotate peer table allocations if active peers exceed hardware capacity.

**Wi-Fi Channel Uniformity**      

Every node and gateway must remain strictly locked on `PEER_CHANNEL 1` via `esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);`. Encrypted frames cannot be parsed during channel hopping.

#### ESP-NOW Closed-Loop Auto-Wakeup Synchronization Protocol

**Overview**     

This protocol synchronizes the deep-sleep cycles of multiple battery-powered **Sensor Nodes** with a central **Gateway Node** using ESP-NOW.

To minimize power consumption and prevent network collisions, Sensor Nodes remain in deep sleep for the majority of the cycle. The Gateway wakes up slightly earlier than the nodes to establish an active listening window, calculates clock phase offsets in real time, and sends dynamic sleep duration commands back to each node.

```text
Gateway:  |-- WAKE --|-- LISTEN & REPLY --|--------------- DEEP SLEEP ---------------|-- WAKE --|
                      ^                  ^
Node 1:   ------------|-- TX / RX ACK ---|---------------- DEEP SLEEP -------------------------|
                      (Node wakes up)    (Sleep command adjusts next wakeup time)
```

**Step 1: Sensor Node Implementation**     

Each Sensor Node reads sensors, transmits its payload via encrypted ESP-NOW, and listens briefly (150ms) for a dynamic configuration command from the Gateway before entering deep sleep.

**1. Dynamic Deep Sleep Function**     

The function accepts a dynamically calculated sleep duration in seconds or milliseconds received from the Gateway.

```cpp
void goToSleep(uint32_t seconds) {
  if (seconds < 1) {
    seconds = 1; // Guard against negative/zero sleep duration
  }

  Serial.printf(
    "Node #%d entering deep sleep for %u seconds...\n\n",
    NODE_ID,
    seconds
  );

  Serial.flush();

  esp_wifi_stop();
  esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
  esp_deep_sleep_start();
}
```

**2. Dual-Data Structures**     

The following data structures define sensor telemetry sent to the Gateway and incoming synchronization commands returned to the Node.

```cpp
typedef struct struct_sensor_data {
  uint8_t node_id;
  float temperature;
  float humidity;
  uint16_t vcc_mv;
  uint32_t boot_count;
} struct_sensor_data;

typedef struct struct_command {
  uint8_t target_node_id;
  int sleep_duration_sec; // Dynamic sleep time calculated by Gateway
  bool relay_state;
} struct_command;
```

**Step 2: Central Gateway Node Implementation**    

The Gateway powers on, pre-registers known Node MAC addresses, and opens a receiver window defined by `LISTEN_WINDOW_MS`.

Upon packet arrival from a node, the Gateway:

1. Records the precise elapsed time, `t_elapsed`, since its own wake-up.
2. Calculates the remaining time until the next scheduled Gateway wake-up cycle.
3. Immediately replies with a `struct_command` containing the exact `sleep_duration_sec` tailored to that specific node.
4. Waits until all nodes respond or `LISTEN_WINDOW_MS` expires.
5. Adjusts its own sleep timer by subtracting `WAKEUP_BUFFER_SEC`, ensuring that it powers on before the nodes during the next cycle.
6. Enters deep sleep.

**Synchronization Formula**     

To eliminate phase drift, calculate sleep durations with millisecond precision:

```text
Node Sleep (ms) = (GATEWAY_SLEEP_SEC x 1000) - t_elapsed_ms     
Gateway Sleep (ms) = ((GATEWAY_SLEEP_SEC - WAKEUP_BUFFER_SEC) x 1000) - t_gateway_elapsed_ms
```

### Broadcast: One-to-Many (1:N)

In an ESP-NOW **Broadcast (1:N)** topology, one Transmitter sends a single message to the universal broadcast MAC address (`FF:FF:FF:FF:FF:FF`). Every Receiver tuned to the same Wi-Fi channel receives the payload simultaneously without needing individual peer MAC registrations or handshakes.

---

#### Core Rules for ESP-NOW Broadcasting

1. **MAC Address:** Target address must be `FF:FF:FF:FF:FF:FF`.
2. **Channel Matching:** All nodes (Transmitter and Receivers) **must** be on the exact same Wi-Fi channel.
3. **Encryption:** Broadcast messages **cannot** be encrypted (`encrypt = false`).
4. **No Acknowledgments (ACK):** The transmitter does not receive hardware delivery ACKs for broadcast packets.

---

#### 1. Transmitter Code (1 - Broadcaster)

This code registers the broadcast peer and transmits a data payload every 2 seconds to all listening nodes.

```cpp
#include <WiFi.h>
#include <esp_now.h>

#define WIFI_CHANNEL 1

// Universal Broadcast MAC Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Sample data structure
typedef struct struct_message {
  uint32_t msg_id;
  float temperature;
  char command[16];
} struct_message;

struct_message txData;
esp_now_peer_info_t peerInfo;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.printf("Broadcast Status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);

  // Set Wi-Fi to Station mode and set channel
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  // Lock to specific Wi-Fi Channel
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register send callback
  esp_now_register_send_cb(OnDataSent);

  // Register broadcast peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false; // Broadcast MUST NOT be encrypted

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add broadcast peer");
    return;
  }

  Serial.println("Broadcaster initialized successfully.");
}

void loop() {
  static uint32_t counter = 0;

  // Prepare payload
  txData.msg_id = ++counter;
  txData.temperature = 25.5f + (rand() % 50) / 10.0f;
  snprintf(txData.command, sizeof(txData.command), "SYNC_LIGHTS");

  // Send broadcast packet (pass NULL or broadcastAddress as target)
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&txData, sizeof(txData));

  if (result == ESP_OK) {
    Serial.printf("[%u] Broadcast sent: Temp = %.1f C, Cmd = %s\n", 
                  txData.msg_id, txData.temperature, txData.command);
  } else {
    Serial.println("Error sending broadcast packet");
  }

  delay(2000);
}
```

---

#### 2. Receiver Code (N - Multiple Receivers)

Flash this exact same firmware onto **N separate ESP32 boards**. They will all listen on Channel 1 and process incoming broadcast packets.

```cpp
#include <WiFi.h>
#include <esp_now.h>

#define WIFI_CHANNEL 1

typedef struct struct_message {
  uint32_t msg_id;
  float temperature;
  char command[16];
} struct_message;

struct_message rxData;

// ESP32 Arduino Core v3.x Callback Signature
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  // Extract Sender MAC Address
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
           recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);

  memcpy(&rxData, incomingData, sizeof(rxData));

  Serial.printf("Received Broadcast from %s | ID: %u | Temp: %.1f C | Cmd: %s\n",
                macStr, rxData.msg_id, rxData.temperature, rxData.command);
}

/* 
// NOTE: If using older ESP32 Arduino Core v2.x, use this callback signature instead:
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&rxData, incomingData, sizeof(rxData));
  Serial.printf("ID: %u | Temp: %.1f C | Cmd: %s\n", rxData.msg_id, rxData.temperature, rxData.command);
}
*/

void setup() {
  Serial.begin(115200);

  // Set Wi-Fi to Station mode and match transmitter channel
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register receive callback
  esp_now_register_recv_cb(OnDataRecv);

  Serial.printf("Receiver Listening on Channel %d... MAC: %s\n", 
                WIFI_CHANNEL, WiFi.macAddress().c_str());
}

void loop() {
  // Receiver stays active listening in background callback
  delay(1000);
}
```

---

#### Comparison Summary

| Feature | Broadcast (1:N) | Unicast Star (N:1 / 1:1) |
| :--- | :--- | :--- |
| **Target MAC** | `FF:FF:FF:FF:FF:FF` | Specific Peer MAC (e.g., `AA:BB:CC:...`) |
| **Peer Registration** | Register once with broadcast address | Must register each node's MAC individually |
| **Encryption** | Not Supported | Supported (`encrypt = true`) |
| **Delivery ACK** | No ACK from receivers | Hardware ACK per transmitted packet |
| **Use Case** | System triggers, time sync, beacon signals | Reliable sensor data logging, commands |

### Broadcast: One-to-Many (1:N) with Deep Sleep

#### Timing Architecture

```
Broadcaster: |-- WAKE & TX --|-------------------- DEEP SLEEP (10,000 ms) --------------------|-- WAKE & TX --|
                              \                                                                  /
Receiver:    |-- CONTINUOUS LISTEN --|-- RX PACKET & CALC SLEEP --|-- DEEP SLEEP --|-- WAKE & LISTEN --|
                                                                                    ^
                                                                        (Wakes up WAKEUP_BUFFER_MS early)
```

1. **Broadcaster:** Records its wake time, calculates time remaining until the next scheduled broadcast cycle (`next_broadcast_ms`), sends the packet, and enters deep sleep.
2. **Receiver (Initial Boot):** Stays continuously awake in listening mode until it receives the first broadcast packet.
3. **Receiver (Synced State):** Extracts `next_broadcast_ms`, subtracts a safety margin (`WAKEUP_BUFFER_MS = 150 ms`), and enters deep sleep. It wakes up slightly before the broadcaster transmits to ensure zero packet loss.
4. **Resynchronization:** If a receiver misses a broadcast due to RF interference, it stays awake past its timeout threshold until the next broadcast arrives to re-sync automatically.

---

#### 1. Broadcaster Code (1 - Broadcaster)

Because the broadcaster enters deep sleep immediately after transmitting, all processing occurs within `setup()`.

```cpp
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define WIFI_CHANNEL 1
#define BROADCAST_INTERVAL_MS 10000 // 10-second total cycle time

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message {
  uint32_t msg_id;
  uint32_t next_broadcast_ms; // Remaining ms until the NEXT broadcast
  float temperature;
  char command[16];
} struct_message;

struct_message txData;
esp_now_peer_info_t peerInfo;
RTC_DATA_ATTR uint32_t msgCounter = 0; // Retain counter across deep sleep

void goToSleepMs(int32_t sleepMs) {
  const int32_t MIN_SLEEP_MS = 100;
  if (sleepMs < MIN_SLEEP_MS) sleepMs = MIN_SLEEP_MS;

  Serial.printf("Broadcaster entering deep sleep for %d ms...\n\n", sleepMs);
  Serial.flush();

  esp_wifi_stop();
  esp_sleep_enable_timer_wakeup((uint64_t)sleepMs * 1000ULL);
  esp_deep_sleep_start();
}

void setup() {
  uint32_t startMs = millis();
  Serial.begin(115200);

  // Initialize Wi-Fi in Station Mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    goToSleepMs(BROADCAST_INTERVAL_MS);
  }

  // Register broadcast peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  // Calculate elapsed active awake time before transmission
  uint32_t activeElapsedMs = millis() - startMs;

  // Prepare Payload
  txData.msg_id = ++msgCounter;
  txData.temperature = 24.2f + (rand() % 40) / 10.0f;
  snprintf(txData.command, sizeof(txData.command), "SYNC_STATE");
  
  // Inform receivers how many milliseconds remain until the NEXT broadcast window
  txData.next_broadcast_ms = BROADCAST_INTERVAL_MS - activeElapsedMs;

  // Transmit Broadcast
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&txData, sizeof(txData));
  
  if (result == ESP_OK) {
    Serial.printf("[%u] Broadcast Sent! Next broadcast in: %u ms\n", 
                  txData.msg_id, txData.next_broadcast_ms);
  } else {
    Serial.println("Broadcast send failed!");
  }

  // Calculate Broadcaster's remaining sleep duration
  uint32_t totalAwakeMs = millis() - startMs;
  int32_t broadcasterSleepMs = (int32_t)BROADCAST_INTERVAL_MS - (int32_t)totalAwakeMs;

  goToSleepMs(broadcasterSleepMs);
}

void loop() {
  // Never reached due to deep sleep
}
```

---

#### 2. Receiver Code (N - Receivers)

Receivers start in continuous listen mode. Once synchronized, they sleep and wake up `WAKEUP_BUFFER_MS` prior to each incoming transmission.

```cpp
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define WIFI_CHANNEL 1
#define WAKEUP_BUFFER_MS 150         // Wake up 150 ms early to catch broadcast
#define MISS_SYNC_TIMEOUT_MS 15000   // Stay awake if broadcast is missed (>15s)

typedef struct struct_message {
  uint32_t msg_id;
  uint32_t next_broadcast_ms;
  float temperature;
  char command[16];
} struct_message;

struct_message rxData;
volatile bool packetReceived = false;
int32_t calculatedSleepMs = 0;

void goToSleepMs(int32_t sleepMs) {
  const int32_t MIN_SLEEP_MS = 10;
  if (sleepMs < MIN_SLEEP_MS) sleepMs = MIN_SLEEP_MS;

  Serial.printf("Receiver sleeping for %d ms (Buffer: %d ms)...\n\n", 
                sleepMs, WAKEUP_BUFFER_MS);
  Serial.flush();

  esp_wifi_stop();
  esp_sleep_enable_timer_wakeup((uint64_t)sleepMs * 1000ULL);
  esp_deep_sleep_start();
}

// ESP32 Arduino Core v3.x Callback Signature
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  memcpy(&rxData, incomingData, sizeof(rxData));

  // Subtract wakeup margin from the broadcaster's remaining cycle time
  int32_t targetSleepMs = (int32_t)rxData.next_broadcast_ms - WAKEUP_BUFFER_MS;

  calculatedSleepMs = targetSleepMs;
  packetReceived = true;
}

/* 
// For ESP32 Arduino Core v2.x, use this signature instead:
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&rxData, incomingData, sizeof(rxData));
  calculatedSleepMs = (int32_t)rxData.next_broadcast_ms - WAKEUP_BUFFER_MS;
  packetReceived = true;
}
*/

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Receiver active. Listening for broadcast sync signal...");
}

void loop() {
  // 1. If packet was received, enter synchronized deep sleep immediately
  if (packetReceived) {
    Serial.printf("Sync Received | Msg ID: %u | Temp: %.1f C | Cmd: %s\n",
                  rxData.msg_id, rxData.temperature, rxData.command);
    goToSleepMs(calculatedSleepMs);
  }

  // 2. If initial boot or missed sync, keep listening in awake state
  static uint32_t lastWarningMs = 0;
  if (millis() - lastWarningMs > 5000) {
    lastWarningMs = millis();
    Serial.printf("Listening... Awake time: %.2f sec\n", millis() / 1000.0f);
  }

  delay(10);
}
```

---

#### Key Operational Characteristics

| Scenario | Broadcaster Behavior | Receiver Behavior |
| :--- | :--- | :--- |
| **Initial Power Up** | Sends broadcast with `next_broadcast_ms ≈ 9980` | Awake in continuous scan mode until packet arrives |
| **Normal Cycle** | Wakes every `10,000 ms`, sends packet, sleeps | Wakes at `9,850 ms` (150 ms early), receives, sleeps |
| **Missed Packet** | Continues normal sleep cycle | Misses window $\rightarrow$ stays awake until next broadcast re-syncs |

