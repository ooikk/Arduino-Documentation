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

const char WIFI_SSID[] = "";
const char WIFI_PASSWORD[] = "";
const char BOT_TOKEN[] = "";
const char AUTHORIZED_CHAT_ID[] = "";

// Change these GPIO assignments for your ESP32 board and hardware.
constexpr uint8_t LED_PIN = 1;
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

// -----------------------------------------------------------------------------
// Telegram and device state
// -----------------------------------------------------------------------------

WiFiClientSecure telegramClient;
UniversalTelegramBot bot(BOT_TOKEN, telegramClient);

unsigned long previousBotPollMs = 0;

bool ledIsOn = false;
bool fanIsOn = false;
bool motorIsOn = false;

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
    {"text":"Motor 0%","callback_data":"MOTOR_0"},
    {"text":"Motor 25%","callback_data":"MOTOR_25"},
    {"text":"Motor 50%","callback_data":"MOTOR_50"}
  ],
  [
    {"text":"Motor 75%","callback_data":"MOTOR_75"},
    {"text":"Motor 100%","callback_data":"MOTOR_100"}
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
  digitalWrite(MOTOR_IN1_PIN, HIGH);
  digitalWrite(MOTOR_IN2_PIN, LOW);
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
  status += " | Speed setting: ";
  status += String(motorSpeedPercent);
  status += "%";

  status += "\nWi-Fi RSSI: ";
  status += String(WiFi.RSSI());
  status += " dBm";

  status += "\n\nSelect an action:";
  return status;
}

void sendControlPanel(
  const String &chatId,
  int messageId = 0) {
  // messageId == 0 sends a new message.
  // A non-zero messageId edits the existing dashboard message.
  bot.sendMessageWithInlineKeyboard(
    chatId,
    buildStatusText(),
    "",
    CONTROL_KEYBOARD,
    messageId);
}

bool isAuthorized(const String &chatId) {
  //Serial.printf("chatID %s Authorized chatID %s\n", chatId, AUTHORIZED_CHAT_ID);
  return chatId == AUTHORIZED_CHAT_ID;
}

bool executeCallback(const String &action) {
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

  return true;
}

void handleCallbackQuery(int index) {
  const String chatId = bot.messages[index].chat_id;
  const String queryId = bot.messages[index].query_id;
  const String action = bot.messages[index].text;
  const int messageId = bot.messages[index].message_id;

  if (!isAuthorized(chatId)) {
    bot.answerCallbackQuery(
      queryId,
      "Unauthorized user",
      true);
    return;
  }

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

void handleTextMessage(int index) {
  const String chatId = bot.messages[index].chat_id;

  if (!isAuthorized(chatId)) {
    bot.sendMessage(chatId, "Unauthorized chat.", "");
    return;
  }

  String command = bot.messages[index].text;
  command.trim();
  command.toLowerCase();

  // Convert /status@BotUsername into /status for group compatibility.
  const int atPosition = command.indexOf('@');
  if (atPosition > 0) {
    command = command.substring(0, atPosition);
  }

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
    if (bot.messages[i].type == "callback_query") {
      handleCallbackQuery(i);
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

  setupOutputs();
  connectWiFi();
  synchronizeClock();

  Serial.println("Setup CA Cert");
  telegramClient.setCACert(
    TELEGRAM_CERTIFICATE_ROOT);
  Serial.println("CA Cert setup success.");
  // Wait up to 10 seconds for a new update during each long-poll request.
  bot.longPoll = 10;
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
