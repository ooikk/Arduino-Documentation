#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "Nightingale_IoT";
const char* password = "98297824";

// Web server login credentials
const char* http_username = "admin";
const char* http_password = "123456";

// GPIO Pin assignments
const int output26 = 48;
const int output27 = 47;
bool output26State = LOW;
bool output27State = HIGH;
bool toggleBasicState = false; // LOW / OFF by default
bool toggleState = true;        // HIGH / ON by default
int sliderVal = 30;
int slider2Val = -30;

WebServer server(80);

const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      html { font-family: Helvetica, sans-serif; display: inline-block; margin: 0px auto; text-align: center;}
      
      :root {
        --card-border: #555555;
        --accent-blue: #4CAF50;
      }

      /* Styled anchor links acting as buttons */
      .btn-link { 
        display: inline-block;
        background-color: #4CAF50; 
        border: none; 
        color: white; 
        padding: 16px 40px; 
        text-decoration: none; 
        font-size: 22px; 
        margin: 2px; 
        cursor: pointer;
        border-radius: 4px;
      }
      .btn-link.off { background-color: #555555; }
      .btn-link.off:hover { background-color: #da190b; }
      .btn-link.on { background-color: #4CAF50; }
      .btn-link.on:hover { background-color: #3e8e41; }

      .container {
        display: flex;
		    font-family: Arial, sans-serif;
        justify-content: center;
        align-items: center;
		
        gap: 5px;
        margin-top: 5px;
		    padding: 5px;
        flex-wrap: wrap;
        background-color: #FBC02D;
      }

      .container > div {
        border: 1px solid #ccc;
        border-radius: 6px;
        padding: 5px;
        background-color: #F5F5DC;
	      margin-top: 5px;
		   margin-bottom: 5px;
      }

      .box {
        flex: 0 0 250px;
        text-align: center;
		/*
        border: 1px solid #ccc;
        border-radius: 6px;
        padding: 15px;*/
      }

      input[type=text] {
        padding: 10px;
        font-size: 16px;
        border: 1px solid #ccc;
        border-radius: 4px;
      }

      .btn-send {
        padding: 10px 15px;
        font-size: 16px;
        background-color: #008CBA;
        color: white;
        border: none; 
        border-radius: 4px;
        cursor: pointer;
      }
      .btn-send:hover { background-color: #007399; }

      /* Toggle Switch Styling */
      .switch-container {
        display: flex;
        align-items: center;
        gap: 12px;
        padding: 5px;
      }

      .switch {
        position: relative;
        display: inline-block;
        width: 50px;
        height: 26px;
      }

      .switch input { opacity: 0; width: 0; height: 0; }

      .slider {
        position: absolute;
        cursor: pointer;
        top: 0; left: 0; right: 0; bottom: 0;
        background-color: var(--card-border);
        transition: .3s;
        border-radius: 34px;
      }

      .slider:before {
        position: absolute;
        content: "";
        height: 18px;
        width: 18px;
        left: 4px;
        bottom: 4px;
        background-color: white;
        transition: .3s;
        border-radius: 50%;
      }

      input:checked + .slider { background-color: var(--accent-blue); }
      input:checked + .slider:before { transform: translateX(24px); }

      /*   Slider 2  */



    /* Top line: Label and value side-by-side */
    .slider-header {
      display: flex;
      align-items: center;
      gap: 8px;
      margin-bottom: 12px;
      font-size: 18px;
    }

    /* Slider line: Min, slider bar, and Max aligned vertically center */
    .slider-row {
      display: flex;
      align-items: center;
      gap: 12px;
    }

    /* Expands slider to take remaining horizontal width */
    .slider2 {
      -webkit-appearance: none;
      appearance: none;
      flex: 1;
	  width: 250px;
      height: 24px;
      background: #e0e0e0;
      outline: none;
      border: none;
      margin: 0;
      padding: 0;
    }

    /* WebKit Thumb */
    .slider2::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 24px;
      height: 24px;
      background: #48BA93;
      cursor: pointer;
      border: none;
    }

    /* Firefox Thumb */
    .slider2::-moz-range-thumb {
      width: 24px;
      height: 24px;
      background: #48BA93;
      cursor: pointer;
      border: none;
    }

    /* Firefox Track */
    .slider2::-moz-range-track {
      background: #e0e0e0;
      height: 24px;
      border: none;
    }

    </style>
  </head>
  <body>
    <h1>ESP32 Web Server</h1>

    <!-- Direct <a> Links (Valid HTML) -->
    <div class="container">
      <div class="box">
        <p><strong>GPIO 26 Status:</strong> <span>%STATE26%</span></p>
        <p><a href="/26" class="btn-link %BUTTON26_CLASS%">%STATUS26%</a></p>   
      </div>

      <div class="box">
        <p><strong>GPIO 27 Status:</strong> <span>%STATE27%</span></p>
        <p><a href="/27" class="btn-link %BUTTON27_CLASS%">%STATUS27%</a></p> 
      </div>
    </div>

    <!-- Custom Payload Input -->
    <div class="container">
      <div style="display: flex; align-items: center; gap: 10px;">
        <span><strong>Custom Payload: </strong></span>
        <input type="text" id="customInput" placeholder="e.g. 180 or Text">
        <button id="btnSendCustom" class="btn-send" onclick="sendCustomPayload()">Send</button>
      </div>
    </div>

    <!-- Toggle Switches -->
    <div class="container">
      <div style="display: flex; align-items: center; gap: 10px;"> 
        <span><strong>Basic Toggle </strong></span>
        <input type="checkbox" id="toggleSwitchBasic" %TOGGLE_BASIC_CHECKED%>
      </div>
   
      <div style="display: flex; align-items: center; gap: 10px;">
        <span><strong>GPIO Toggle </strong></span>
        <div class="switch-container">
          <label class="switch">
            <input type="checkbox" id="toggleSwitch" %TOGGLE_CHECKED% >
            <span class="slider"></span>
          </label>
        </div>
      </div>
    </div>

	  <!-- Slider bar  -->
  <div class="container">
  <div style="display: flex; flex-direction: column; align-items: center; gap: 6px;">
    
    <!-- Top Label/Value Display -->
    <div>
      <span>Current Value: </span>
      <strong id="sliderValue" style="color: red;">%SLIDER_VAL%</strong>
    </div>

    <!-- Slider & Min/Max Controls -->
    <div style="display: flex; align-items: center; gap: 8px;">
      <span><strong>Slider Bar:</strong></span>
      <span>0</span>
      <input type="range" min="0" max="100" value="%SLIDER_VAL%" id="mySlider">
      <span>100</span>
    </div>

  </div>

  <!-- Slider 2 bar  -->

  <div class="slider-container">

    <!-- Header line with label and value inline -->
    <div class="slider-header">
      <label for="slider2"><strong>Custom range slider: </strong></label>
      <strong id="slider2Value" style="color: red;">%SLIDER2_VAL%</strong>
    </div>

    <!-- Row with min/max values aligned vertically with the slider -->
    <div class="slider-row">
      <span>-200</span>
      <input type="range" min="-200" max="200" value="%SLIDER2_VAL%" class="slider2" id="slider2">
      <span>200</span>
    </div>

  </div>
</div>

    <script>
      // Function to post text with auth header included
      function sendCustomPayload() {
        const inputVal = document.getElementById("customInput").value;
        if (!inputVal) {
          alert("Please enter a payload!");
          return;
        }

        fetch("/custom", {
          method: "POST",
          headers: { "Content-Type": "text/plain" },
          credentials: "same-origin",
          body: inputVal
        })
        .then(response => {
          if (!response.ok) throw new Error("Auth or Server Error");
          return response.text();
        })
        .then(data => {
          alert("ESP32 Response: " + data);
          document.getElementById("customInput").value = "";
        })
        .catch(error => {
          console.error("Error:", error);
          alert("Failed to send payload: " + error.message);
        });
      }

  // Function to post toggle switch
      function sendToggleSwitchCommand(endpoint, value) {
        fetch(endpoint, {
          method: "POST",
          headers: { "Content-Type": "text/plain" },
          credentials: "same-origin",
          body: value
        })
        .then(response => response.text())
        .then(data => console.log("Response:", data))
        .catch(error => console.error("Error:", error));
      }

      // Safe Event Listeners
      const toggleSwitch = document.getElementById('toggleSwitch');
      if (toggleSwitch) {
        toggleSwitch.addEventListener('change', (e) => {
          sendToggleSwitchCommand('/toggle', e.target.checked ? '1' : '0');
        });
      }

      const toggleSwitchBasic = document.getElementById('toggleSwitchBasic');
      if (toggleSwitchBasic) {
        toggleSwitchBasic.addEventListener('change', (e) => {
          sendToggleSwitchCommand('/toggleBasic', e.target.checked ? '1' : '0');
        });
      }


  // Function to post Slider Bar value

const mySlider = document.getElementById('mySlider');
const sliderValueDisplay = document.getElementById('sliderValue');

if (mySlider) {
  // 1. Update text live on screen while dragging
  mySlider.addEventListener('input', (e) => {
    sliderValueDisplay.innerText = e.target.value;
  });

  // 2. Send value to ESP32 when drag finishes
  mySlider.addEventListener('change', (e) => {
    sendSliderValue('/slider',e.target.value);
  });
}

function sendSliderValue(endpoint, value) {
  fetch(endpoint, {
    method: "POST",
    headers: { "Content-Type": "text/plain" },
    credentials: "same-origin", // Included for HTTP Basic Auth
    body: value
  })
  .then(response => response.text())
  .then(data => console.log("ESP32 Response:", data))
  .catch(error => console.error("Error sending slider value:", error));
}

// Function for Slider 2 value
// Dynamic value update on drag
  const mySlider2 = document.getElementById('slider2');
  const slider2ValueDisplay = document.getElementById('slider2Value');


if (mySlider2) {
  // 1. Update text live on screen while dragging
  mySlider2.addEventListener('input', (e) => {
    slider2ValueDisplay.innerText = e.target.value;
  });

  // 2. Send value to ESP32 when drag finishes
  mySlider2.addEventListener('change', (e) => {
    sendSliderValue('/slider2',e.target.value);
  });
}



    </script>
  </body>
</html>
)rawliteral";

bool isAuthenticated() {
  if (!server.authenticate(http_username, http_password)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

void handleRoot() {
  if (!isAuthenticated()) return;

  String html = htmlPage;
  html.replace("%STATE26%", output26State ? "ON" : "OFF");
  html.replace("%STATUS26%", output26State ? "Turn OFF" : "Turn ON");
  html.replace("%BUTTON26_CLASS%", output26State ? "off" : "on");

  html.replace("%STATE27%", output27State ? "ON" : "OFF");
  html.replace("%STATUS27%", output27State ? "Turn OFF" : "Turn ON");
  html.replace("%BUTTON27_CLASS%", output27State ? "off" : "on");
  // Dynamic Toggle State Replacements ---
  html.replace("%TOGGLE_BASIC_CHECKED%", toggleBasicState ? "checked" : "");
  html.replace("%TOGGLE_CHECKED%", toggleState ? "checked" : "");

  // Replaces both the slider position AND the text display
  html.replace("%SLIDER_VAL%", String(sliderVal));
  html.replace("%SLIDER2_VAL%", String(slider2Val));

  server.send(200, "text/html", html);
}

void handleGPIO26() {
  if (!isAuthenticated()) return;
  output26State = !output26State;
  digitalWrite(output26, output26State);
  Serial.printf("GPIO 26 toggled to: %d\n", output26State);
  
  // handleRoot();
  // Redirect back to root after button press
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleGPIO27() {
  if (!isAuthenticated()) return;
  output27State = !output27State;
  digitalWrite(output27, output27State);
  Serial.printf("GPIO 27 toggled to: %d\n", output27State);
  
  // handleRoot();
  // Redirect back to root after button press
  server.sendHeader("Location", "/");
  server.send(303);
}

// Check if String is a valid integer
bool isInteger(const String& s) {
  String str = s;
  str.trim();
  if (str.length() == 0) return false;
  size_t i = 0;
  if (str[0] == '+' || str[0] == '-') i = 1;
  if (i >= str.length()) return false;
  for (; i < str.length(); i++) {
    if (!isDigit(str[i])) return false;
  }
  return true;
}

// Check if String is a valid number (integer or float)
bool isNumber(const String& s) {
  String str = s;
  str.trim();
  if (str.length() == 0) return false;
  boolean seenDot = false;
  size_t i = 0;
  if (str[0] == '+' || str[0] == '-') i = 1;
  if (i >= str.length()) return false;
  for (; i < str.length(); i++) {
    char c = str[i];
    if (isDigit(c)) continue;
    if (c == '.' && !seenDot) {
      seenDot = true;
      continue;
    }
    return false;
  }
  return true;
}

// Function to receive and process Custom Payload from web browser
void handleCustomPayload() {
  if (!isAuthenticated()) return;

  if (server.hasArg("plain")) {
    String payload = server.arg("plain");
    payload.trim();

    Serial.println("=================================");
    Serial.print("Custom Payload Received: ");
    Serial.println(payload);
    Serial.println("=================================");

    if (isInteger(payload)) {
      Serial.printf("Integer : %d\n", payload.toInt());
    } else if (isNumber(payload)) {
      Serial.printf("Floating number: %f\n", payload.toFloat());
    } else {
      // FIXED: Added .c_str() to prevent crashing the ESP32
      Serial.printf("It's String: %s\n", payload.c_str());
    }

    server.send(200, "text/plain", "Payload '" + payload + "' received successfully!");
  } else {
    server.send(400, "text/plain", "Error: No payload received");
  }
}

void handleToggleSwitch() {
  if (!isAuthenticated()) return;

  if (server.hasArg("plain")) {
    String payload = server.arg("plain");
    payload.trim();
    // Update internal state when web browser sends '1' or '0'
    toggleState = (payload == "1");
    Serial.printf("GPIO Toggle state saved in ESP32: %s\n", toggleState ? "ON" : "OFF");

    Serial.print("GPIO Toggle Command Received: ");
    Serial.println(payload);
    server.send(200, "text/plain", "Toggle updated to " + payload);
  } else {
    server.send(400, "text/plain", "No state received");
  }
}

void handleToggleSwitchBasic() {
  if (!isAuthenticated()) return;

  if (server.hasArg("plain")) {
    String payload = server.arg("plain");
    payload.trim();
    // Update internal state when web browser sends '1' or '0'
    toggleBasicState = (payload == "1");
    Serial.printf("Toggle Basic state saved in ESP32: %s\n", toggleBasicState ? "ON" : "OFF");

    Serial.print("Toggle Basic Switch Command Received: ");
    Serial.println(payload);
    server.send(200, "text/plain", "Toggle Basic Switch updated to " + payload);
  } else {
    server.send(400, "text/plain", "No state received");
  }
}


void handleSlider() {
  if (!isAuthenticated()) return;

  if (server.hasArg("plain")) {
    String payload = server.arg("plain");
    payload.trim();

    // Convert payload to integer and save
    sliderVal = payload.toInt();

    Serial.printf("Slider value set to: %d\n", sliderVal);

    // Optional: Drive a PWM pin or servo with sliderVal here
    // analogWrite(PWM_PIN, map(sliderVal, 0, 100, 0, 255));

    server.send(200, "text/plain", "Slider updated to " + String(sliderVal));
  } else {
    server.send(400, "text/plain", "No slider value received");
  }
}

void handleSlider2() {
  if (!isAuthenticated()) return;

  if (server.hasArg("plain")) {
    String payload = server.arg("plain");
    payload.trim();

    // Convert payload to integer and save
    slider2Val = payload.toInt();

    Serial.printf("Slider2 value set to: %d\n", slider2Val);

    // Optional: Drive a PWM pin or servo with sliderVal here
    // analogWrite(PWM_PIN, map(sliderVal, 0, 100, 0, 255));

    server.send(200, "text/plain", "Slider2 updated to " + String(slider2Val));
  } else {
    server.send(400, "text/plain", "No slider2 value received");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  digitalWrite(output26, output26State);
  digitalWrite(output27, output27State);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/26", handleGPIO26);
  server.on("/27", handleGPIO27);
  server.on("/custom", HTTP_POST, handleCustomPayload);
  server.on("/toggle", HTTP_POST, handleToggleSwitch);
  server.on("/toggleBasic", HTTP_POST, handleToggleSwitchBasic);
    // Register the new slider route
  server.on("/slider", HTTP_POST, handleSlider);
  server.on("/slider2", HTTP_POST, handleSlider2);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}