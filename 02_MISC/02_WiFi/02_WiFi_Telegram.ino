/*
  ESP32 Telegram Inline-Button Device Controller

  Target:
    - Arduino-ESP32 core 3.x
    - UniversalTelegramBot library
    - ArduinoJson library

  Devices:
    1. External LED: ON/OFF
    2. Low-voltage 2-wire DC fan through a MOSFET driver: ON/OFF and PWM speed
    3. Low-voltage DC motor through an H-bridge: ON/OFF and PWM speed

  Security:
    - Replace all credential placeholders.
    - Never publish your bot token.
    - AUTHORIZED_CHAT_ID restricts control to one Telegram private chat.

  Hardware warning:
    - Never connect a fan or motor directly to an ESP32 GPIO.
    - Use a suitable MOSFET/driver, flyback protection, an external supply,
      and a common ground with the ESP32.
    - This sketch does not directly control AC mains equipment.
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <time.h>

// -----------------------------------------------------------------------------
// User settings
// -----------------------------------------------------------------------------

#define UPLOAD_PHOTO

#ifdef UPLOAD_PHOTO
#include <HTTPClient.h>
#include <SD.h>
#include <SPI.h>
#endif

const char WIFI_SSID[] = "";
const char WIFI_PASSWORD[] = "";
const char BOT_TOKEN[] = "";
const char AUTHORIZED_CHAT_ID[] = "";

// Change these GPIO assignments for your ESP32 board and hardware.
constexpr uint8_t LED_PIN = 1;
constexpr uint8_t BUTTON_PIN = 2;  // Boot button on ESP32-S3 (Pull-up)
constexpr uint8_t FAN_PWM_PIN = 39;
constexpr uint8_t MOTOR_PWM_PIN = 40;
constexpr uint8_t MOTOR_IN1_PIN = 41;
constexpr uint8_t MOTOR_IN2_PIN = 42;

// Arduino-ESP32 3.x LEDC settings.
constexpr uint32_t FAN_PWM_FREQUENCY = 25000;
constexpr uint32_t MOTOR_PWM_FREQUENCY = 20000;
constexpr uint8_t PWM_RESOLUTION_BITS = 8;
constexpr uint16_t PWM_MAX_DUTY = 255;

constexpr unsigned long BOT_POLL_INTERVAL_MS = 1000;
#ifdef UPLOAD_PHOTO

#define SD_SCLK_PIN 4
#define SD_MISO_PIN 5
#define SD_MOSI_PIN 6
#define SD_CS_PIN 7
#define SD_FREQUENCY 4000000  //16000000  // 16MHz or 4MHz

SPIClass sdSPI(FSPI);

constexpr size_t MAX_PHOTO_SIZE = 20 * 1024 * 1024;
#endif

// -----------------------------------------------------------------------------
// Telegram and device state
// -----------------------------------------------------------------------------

WiFiClientSecure telegramClient;
UniversalTelegramBot bot(BOT_TOKEN, telegramClient);

unsigned long previousBotPollMs = 0;

bool ledIsOn = false;
bool fanIsOn = false;
bool motorIsOn = false;
bool motorDirectionForward = true;

// The last selected speed is retained when FAN OFF or MOTOR OFF is pressed.
uint8_t fanSpeedPercent = 50;
uint8_t motorSpeedPercent = 50;

// Each callback_data value is short and uniquely identifies one action.
const char CONTROL_KEYBOARD[] = R"json(
[
  [
    {"text":"LED ON","callback_data":"LED_ON"},
    {"text":"LED OFF","callback_data":"LED_OFF"}
  ],
  [
    {"text":"FAN ON","callback_data":"FAN_ON"},
    {"text":"FAN OFF","callback_data":"FAN_OFF"}
  ],
  [
    {"text":"Fan 25%","callback_data":"FAN_25"},
    {"text":"Fan 50%","callback_data":"FAN_50"}
  ],
  [
    {"text":"Fan 75%","callback_data":"FAN_75"},
    {"text":"Fan 100%","callback_data":"FAN_100"}
  ],
  [
    {"text":"MOTOR ON","callback_data":"MOTOR_ON"},
    {"text":"MOTOR OFF","callback_data":"MOTOR_OFF"}
  ],
  [
    {"text":"MOTOR Forward","callback_data":"MOTOR_FORWARD"},
    {"text":"MOTOR Reverse","callback_data":"MOTOR_REVERSE"}
  ],
  [
    {"text":"Motor 0%","callback_data":"MOTOR_0"},
    {"text":"Motor 25%","callback_data":"MOTOR_25"},
    {"text":"Motor 50%","callback_data":"MOTOR_50"}
  ],
  [
    {"text":"Motor 75%","callback_data":"MOTOR_75"},
    {"text":"Motor 100%","callback_data":"MOTOR_100"}
  ],
  [
    {"text": "UPLOAD IMAGE","callback_data": "UPLOAD_IMAGE"}
  ],
  [
    {"text":"REFRESH STATUS","callback_data":"STATUS"}
  ]
]
)json";

// -----------------------------------------------------------------------------
// Hardware control
// -----------------------------------------------------------------------------

uint32_t percentToDuty(uint8_t percent) {
  percent = constrain(percent, 0, 100);
  return map(percent, 0, 100, 0, PWM_MAX_DUTY);
}

void applyLedOutput() {
  digitalWrite(LED_PIN, ledIsOn ? HIGH : LOW);
}

void applyFanOutput() {
  const uint32_t duty =
    fanIsOn ? percentToDuty(fanSpeedPercent) : 0;

  ledcWrite(FAN_PWM_PIN, duty);
}

void applyMotorOutput() {
  if (!motorIsOn || motorSpeedPercent == 0) {
    ledcWrite(MOTOR_PWM_PIN, 0);
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    return;
  }

  // Fixed forward direction. Add direction buttons if required later.
  //digitalWrite(MOTOR_IN1_PIN, HIGH);
  //digitalWrite(MOTOR_IN2_PIN, LOW);
  /*
  if (currentMotorDirectionForward) {
    digitalWrite(MOTOR_IN1_PIN, HIGH);
    digitalWrite(MOTOR_IN2_PIN, LOW);
  } else {
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, HIGH);
  }
  */

  setMotorDirection(motorDirectionForward);


  ledcWrite(
    MOTOR_PWM_PIN,
    percentToDuty(motorSpeedPercent));
}

void setFanPower(bool turnOn) {
  fanIsOn = turnOn;

  if (fanIsOn && fanSpeedPercent == 0) {
    fanSpeedPercent = 50;
  }

  applyFanOutput();
}

void setFanSpeed(uint8_t percent) {
  fanSpeedPercent = constrain(percent, 0, 100);
  fanIsOn = fanSpeedPercent > 0;
  applyFanOutput();
}

void setMotorPower(bool turnOn) {
  motorIsOn = turnOn;

  if (motorIsOn && motorSpeedPercent == 0) {
    motorSpeedPercent = 50;
  }
  applyMotorOutput();
}

void setMotorSpeed(uint8_t percent) {
  motorSpeedPercent = constrain(percent, 0, 100);
  motorIsOn = motorSpeedPercent > 0;
  applyMotorOutput();
}

void setMotorDirection(bool forward) {
  int previousSpeed = motorSpeedPercent;

  if ((motorDirectionForward != forward) || (digitalRead(MOTOR_IN1_PIN) == LOW && digitalRead(MOTOR_IN2_PIN) == LOW)) {
    // Stop motor before changing direction.
    setMotorSpeed(0);
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    delay(150);

    if (forward) {
      digitalWrite(MOTOR_IN1_PIN, HIGH);
      digitalWrite(MOTOR_IN2_PIN, LOW);
      motorDirectionForward = true;
    } else {
      digitalWrite(MOTOR_IN1_PIN, LOW);
      digitalWrite(MOTOR_IN2_PIN, HIGH);
      motorDirectionForward = false;
    }

    delay(50);
    setMotorSpeed(previousSpeed);
  }
}

// -----------------------------------------------------------------------------
// Telegram dashboard
// -----------------------------------------------------------------------------

String onOffText(bool value) {
  return value ? "ON" : "OFF";
}

String buildStatusText() {
  String status;
  status.reserve(240);

  status += "ESP32 DEVICE CONTROL\n\n";
  status += "LED: ";
  status += onOffText(ledIsOn);

  status += "\nFan: ";
  status += onOffText(fanIsOn);
  status += " | Speed setting: ";
  status += String(fanSpeedPercent);
  status += "%";

  status += "\nMotor: ";
  status += onOffText(motorIsOn);

  status += " | Direction: ";
  status += motorDirectionForward ? "Forward" : "Reverse";

  status += " | Speed setting: ";
  status += String(motorSpeedPercent);
  status += "%";

  status += "\nTemperature: ";
  status += String(random(100, 500) / 10.0f);
  status += " °C";

  status += "\nWi-Fi RSSI: ";
  status += String(WiFi.RSSI());
  status += " dBm";

  status += "\nButton: ";
  status += digitalRead(BUTTON_PIN) == LOW ? "Pressed" : "Released";

  status += "\n\nSelect an action:";
  return status;
}

void sendControlPanel(
  const String& chatId,
  int messageId = 0) {
  // messageId == 0 sends a new message.
  // A non-zero messageId edits the existing dashboard message.
  const String Status = buildStatusText();
  bot.sendMessageWithInlineKeyboard(
    chatId,
    //buildStatusText(),
    Status,
    "",
    CONTROL_KEYBOARD,
    messageId);

  Serial.println("Send Status & Control Keyboard:");
  Serial.println(Status);
}

bool isAuthorized(const String& chatId) {
  //Serial.printf("chatID %s Authorized chatID %s\n", chatId, AUTHORIZED_CHAT_ID);
  return chatId == AUTHORIZED_CHAT_ID;
  //return chatId.equals(AUTHORIZED_CHAT_ID);
}

bool executeCallback(const String& action) {
  if (action == "LED_ON") {
    ledIsOn = true;
    applyLedOutput();
  } else if (action == "LED_OFF") {
    ledIsOn = false;
    applyLedOutput();
  } else if (action == "FAN_ON") {
    setFanPower(true);
  } else if (action == "FAN_OFF") {
    setFanPower(false);
  } else if (action == "FAN_25") {
    setFanSpeed(25);
  } else if (action == "FAN_50") {
    setFanSpeed(50);
  } else if (action == "FAN_75") {
    setFanSpeed(75);
  } else if (action == "FAN_100") {
    setFanSpeed(100);
  } else if (action == "MOTOR_ON") {
    setMotorPower(true);
  } else if (action == "MOTOR_OFF") {
    setMotorPower(false);
  } else if (action == "MOTOR_FORWARD") {
    setMotorDirection(true);
  } else if (action == "MOTOR_REVERSE") {
    setMotorDirection(false);
  } else if (action == "MOTOR_0") {
    setMotorSpeed(0);
  } else if (action == "MOTOR_25") {
    setMotorSpeed(25);
  } else if (action == "MOTOR_50") {
    setMotorSpeed(50);
  } else if (action == "MOTOR_75") {
    setMotorSpeed(75);
  } else if (action == "MOTOR_100") {
    setMotorSpeed(100);
  } else if (action == "STATUS") {
    // No output change; the dashboard will be refreshed below.
  } else {
    return false;
  }
  Serial.printf("Execute Callback Action: %s\n", action);

  return true;
}

void handleCallbackQuery(int index) {
  const String chatId = bot.messages[index].chat_id;
  const String queryId = bot.messages[index].query_id;
  const String action = bot.messages[index].text;
  const int messageId = bot.messages[index].message_id;

  /**
  if (!isAuthorized(chatId)) {
    bot.answerCallbackQuery(
      queryId,
      "Unauthorized user",
      true);
    return;
  }
*/

#ifdef UPLOAD_PHOTO
  if (action == "UPLOAD_IMAGE") {
    bot.answerCallbackQuery(
      queryId,
      "Please attach an image");
    bot.sendMessage(
      chatId,
      "Please send the image as a File/Document:\n\n"
      "1. Press the attachment icon.\n"
      "2. Select File or Document.\n"
      "3. Select a JPG or PNG file.\n"
      "4. Send it to this bot.\n\n"
      "Do not select Gallery/Photo because the current "
      "ESP32 library only detects incoming documents.\n"
      "Max file size 20MB.",
      "");

    Serial.println("Received Text Message command: " + action);
    return;
  }
#endif

  if (!executeCallback(action)) {
    bot.answerCallbackQuery(
      queryId,
      "Unknown action",
      true);
    return;
  }

  // Remove Telegram's button-loading animation.
  bot.answerCallbackQuery(
    queryId,
    "Device updated",
    false);

  // Refresh the status by editing the existing inline-keyboard message.
  sendControlPanel(chatId, messageId);
}

#ifdef UPLOAD_PHOTO

bool isAcceptedImage(const String& name) {
  String lowerName = name;
  lowerName.toLowerCase();
  return lowerName.endsWith(
           ".jpg")
         || lowerName.endsWith(
           ".jpeg")
         || lowerName.endsWith(
           ".png");
}

bool downloadPhotoToSD(const String& fileUrl, const String& destination, long expectedSize) {
  WiFiClientSecure downloadClient;
  downloadClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  HTTPClient https;
  if (!https.begin(downloadClient, fileUrl)) {
    Serial.println("[PHOTO] HTTPS begin failed");
    return false;
  }

  https.setConnectTimeout(15000);
  https.setTimeout(30000);


  //https.setReuse(false);  // before https.GET()

  const int httpCode = https.GET();
  // Immediately after https.GET():
  Serial.printf("[PHOTO] GET=%d, HTTP length=%d, Telegram length=%ld\n",
                httpCode, https.getSize(), expectedSize);

  //Serial.printf("[PHOTO] HTTP GET: %d\n", httpCode);



  if (httpCode != HTTP_CODE_OK) {
    https.end();
    return false;
  }

  File outputFile = SD.open(destination, FILE_WRITE);
  if (!outputFile) {
    Serial.println("[PHOTO] Cannot open SD file for writing");
    https.end();
    return false;
  }

  // Immediately before writeToStream():
  Serial.println("[PHOTO] Starting Telegram-to-SD transfer, please wait...");

  const int bytesWritten = https.writeToStream(&outputFile);

  // Immediately after writeToStream():
  Serial.printf("[PHOTO] Transfer returned %d\n", bytesWritten);


  outputFile.flush();
  const size_t savedSize = outputFile.size();
  outputFile.close();
  https.end();

  Serial.printf("[PHOTO] Download result: %d, SD file size: %u, expected: %ld\n",
                bytesWritten, (unsigned)savedSize, expectedSize);

  const bool success =
    bytesWritten > 0 && savedSize == (size_t)bytesWritten && (expectedSize <= 0 || savedSize == (size_t)expectedSize);

  if (!success) {
    SD.remove(destination);  // Remove an empty or incomplete image
  }
  return success;
}

String makeSafeFileName(String fileName) {
  fileName.replace("/", "_");
  fileName.replace("\\", "_");
  fileName.replace("..", "_");
  fileName.replace(":", "_");
  fileName.replace("*", "_");
  fileName.replace("?", "_");
  fileName.replace("\"", "_");
  fileName.replace("<", "_");
  fileName.replace(">", "_");
  fileName.replace("|", "_");

  if (fileName.length() == 0) {
    fileName = "image.jpg";
  }

  return fileName;
}

void handleIncomingDocument(int index) {
  const String chatId = bot.messages[index].chat_id;

  const String fileName = bot.messages[index].file_name;

  const String filePath = bot.messages[index].file_path;

  const long fileSize = bot.messages[index].file_size;

  const String caption = bot.messages[index].file_caption;

  Serial.println();
  Serial.println(
    "========== DOCUMENT RECEIVED ==========");

  Serial.printf("File name: %s\n", fileName.c_str());

  Serial.printf("File size: %ld bytes\n", fileSize);

  Serial.printf("Caption: %s\n", caption.c_str());

  // Do not print filePath publicly because
  // it contains the bot token.
  Serial.println("Telegram download path obtained");

  bot.sendMessage(
    chatId,
    "Image document received: " + fileName + "\nSize: " + String(fileSize) + " bytes",
    "");

  // validate file extension and file size
  if (!isAcceptedImage(fileName)) {
    bot.sendMessage(
      chatId,
      "Please send a JPG or PNG image.",
      "");
    return;
  }

  if (fileSize <= 0 || fileSize > MAX_PHOTO_SIZE) {
    bot.sendMessage(
      chatId,
      "The image is too large.",
      "");

    return;
  }

  // Next step:
  // downloadPhotoToSD(
  //   filePath,
  //   "/telegram_photo.jpg"
  // );

  String safeFileName = makeSafeFileName(fileName);

  String destination = String("/tel_") + safeFileName;
  //Serial.printf("[PHOTO] Saving as: %s\n", destination.c_str());
  Serial.printf("[PHOTO] Downloading %s (%ld bytes)...\n",
                destination.c_str(), fileSize);
  unsigned long started = millis();
  bool success = downloadPhotoToSD(filePath, destination, fileSize);
  Serial.printf("[PHOTO] %s after %lu seconds\n",
                success ? "Complete" : "Failed",
                (millis() - started) / 1000);

  if (success) {
    bot.sendMessage(
      chatId,
      "Image downloaded successfully:\n" + destination,
      "");
  } else {
    bot.sendMessage(
      chatId,
      "Image download failed.",
      "");
  }
}
#endif

void handleTextMessage(int index) {
  const String chatId = bot.messages[index].chat_id;

  /**
  if (!isAuthorized(chatId)) {
    bot.sendMessage(chatId, "Unauthorized chat.", "");
    return;
  }
*/

  String command = bot.messages[index].text;
  command.trim();
  command.toLowerCase();

  // Convert /status@BotUsername into /status for group compatibility.
  const int atPosition = command.indexOf('@');
  if (atPosition > 0) {
    command = command.substring(0, atPosition);
  }

  Serial.println("Received Text Message command: " + command);

  if (command == "/start" || command == "/panel") {
    sendControlPanel(chatId);
  } else if (command == "/status") {
    bot.sendMessage(chatId, buildStatusText(), "");
  } else if (command == "/help") {
    bot.sendMessage(
      chatId,
      "/panel - Open inline controls\n"
      "/status - Show device status\n"
      "/help - Show commands",
      "");
  } else {
    bot.sendMessage(
      chatId,
      "Unknown command. Send /panel to open the controls.",
      "");
  }
}

void handleTelegramUpdates(int updateCount) {
  for (int i = 0; i < updateCount; i++) {

    const String chatId =
      bot.messages[i].chat_id;

    // Reject unauthorized chats before processing anything.
    if (!isAuthorized(chatId)) {
      Serial.printf(
        "[TELEGRAM] Unauthorized chat ID: %s\n",
        chatId.c_str());

      bot.sendMessage(
        chatId,
        "Unauthorized user.",
        "");

      continue;  // continue to next chatId, i+1
    }

    // 1. Inline keyboard button press
    if (bot.messages[i].type == "callback_query") {
      handleCallbackQuery(i);
#ifdef UPLOAD_PHOTO
      // 2. Incoming file/document
    } else if (
      bot.messages[i].type == "message" && bot.messages[i].hasDocument) {
      handleIncomingDocument(i);
#endif
      // 3. Ordinary text message
    } else {
      handleTextMessage(i);
    }
  }
}

// -----------------------------------------------------------------------------
// Wi-Fi and setup
// -----------------------------------------------------------------------------

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }

  Serial.println();
  Serial.print("Wi-Fi connected. IP: ");
  Serial.println(WiFi.localIP());
}

void synchronizeClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Synchronizing time");
  time_t now = time(nullptr);

  while (now < 24 * 3600) {
    Serial.print('.');
    delay(250);
    now = time(nullptr);
  }

  Serial.println("Success Synchronized.");
}

void setupOutputs() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW);

  if (!ledcAttach(
        FAN_PWM_PIN,
        FAN_PWM_FREQUENCY,
        PWM_RESOLUTION_BITS)) {
    Serial.println("ERROR: Fan PWM attach failed");
  }

  if (!ledcAttach(
        MOTOR_PWM_PIN,
        MOTOR_PWM_FREQUENCY,
        PWM_RESOLUTION_BITS)) {
    Serial.println("ERROR: Motor PWM attach failed");
  }

  applyLedOutput();
  applyFanOutput();
  applyMotorOutput();
}

void setup() {
  Serial.begin(115200);
  delay(200);

#ifdef UPLOAD_PHOTO
  sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
    Serial.print("SD Card initialization failed! Try again.");
    if (!SD.begin(SD_CS_PIN, sdSPI, SD_FREQUENCY)) {
      delay(1000);  // Halt
      Serial.print(".");
    }
  }
  Serial.println("SD card ready");

  Serial.printf("Card size: %llu MB\n", SD.cardSize() / (1024ULL * 1024ULL));
  /*
  File test = SD.open("/sd_test.txt", FILE_WRITE);
  if (!test) {
    Serial.println("[TEST] SD.open failed");
  } else {
    size_t written = test.print("SD write test\n");
    test.flush();
    Serial.printf("[TEST] SD wrote %u bytes; file size %u\n",
                  (unsigned)written, (unsigned)test.size());
    test.close();
  }
*/

#endif

  setupOutputs();
  connectWiFi();
  synchronizeClock();

  Serial.println("Setup CA Cert");
  telegramClient.setCACert(
    TELEGRAM_CERTIFICATE_ROOT);
  Serial.println("CA Cert setup success.");
  // Wait up to 10 seconds for a new update during each long-poll request.
  bot.longPoll = 10;
  // Increae the default JSON length of 1500 byte to support long CONTROL_KEYBOARD
#ifdef UPLOAD_PHOTO
  bot.maxMessageLength = 6144;
#else
  bot.maxMessageLength = 4096;
#endif
  Serial.println("Send to bot: ESP32 inline controller is online. Send /panel.");
  bot.sendMessage(
    AUTHORIZED_CHAT_ID,
    "ESP32 inline controller is online. Send /panel.",
    "");

  Serial.println("Enter polling command loop.");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (
    millis() - previousBotPollMs >= BOT_POLL_INTERVAL_MS) {
    int updateCount =
      bot.getUpdates(
        bot.last_message_received + 1);

    while (updateCount > 0) {
      handleTelegramUpdates(updateCount);

      updateCount =
        bot.getUpdates(
          bot.last_message_received + 1);
    }

    previousBotPollMs = millis();
  }
}
