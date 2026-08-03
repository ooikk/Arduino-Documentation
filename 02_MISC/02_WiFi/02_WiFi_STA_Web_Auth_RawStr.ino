
#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "Nightingale_IoT";
const char* password = "98297824";

// Username and password for web page access
const char* http_username = "admin";
const char* http_password = "123456";

// Assign output variables to GPIO pins
const int output26 = 48;
const int output27 = 47;
bool output26State = LOW;
bool output27State = HIGH;

// HTML content to be served
// const char* htmlPage = R"rawliteral(
// store the raw HTML template directly in Flash Memory
const char htmlPage[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>
	<head>
  <style>
   html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
    .button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 25px; margin: 2px; cursor: pointer;}
    .on { background-color: #4CAF50; color: white; }
    .on:hover { background-color: #5a8263; padding: 13px 30px;}
    .off { background-color: #555555; color: white; }
    .off:hover { background-color: #da190b; padding: 13px 30px;}

	  .container {
     display: flex;               /* enable flexbox */
     justify-content: center;     /* center horizontally */
     align-items: center;         /* center vertically within container */
     gap: 20px;                   /* space between GPIO blocks */
    }

   .box {
     flex: 0 0 250px;             /* fixed width; adjust as needed */
	   text-align: center;          /* center text inside each block */
    }

  <!-- comment out
   .box p {
    display: flex;
    justify-content: center;
    align-items: center;
    gap: 6px;
    margin: 8px 0;
   }
  -->

<!-- To receive text from Web browser  -->
      input[type=text] {
        padding: 10px;
        font-size: 16px;
        border: 1px solid #ccc;
        border-radius: 4px;
      }

      .btn-send {
        padding: 10px 20px;
        font-size: 16px;
        background-color: #008CBA;
        color: white;
        border: none;
        border-radius: 4px;
        cursor: pointer;
      }
      .btn-send:hover { background-color: #007399; }

   </style>
   
   </head>
  
  
  <body>
   <h1>ESP32 Web Server</h1>
   
  <!--  comment out: not inline display
   <p style="display: inline-block;" >
   <p><strong>GPIO 26 Status:</strong> 
   <span id="gpio26">%STATE26%<span></p>
   <p><a href="/26"><button class="%BUTTON26%">%STATUS26%</button></a></p>
   </p>

	<p style="display: inline-block;">	
  <p><strong>GPIO 27 Status:</strong>
	<span id="gpio27">%STATE27%</span></p>
	<p><a href="/27"><button class="%BUTTON27%">%STATUS27%</button></a></p>
  </p>
  -->

  <div class="container">

  <div class="box">
   <p><strong>GPIO 26 Status:</strong> 
   <span id="gpio26">%STATE26%</span></p>
   <p><a href="/26"><button class="%BUTTON26%">%STATUS26%</button></a></p>   
  </div>

 <div class="box">
   <p><strong>GPIO 27 Status:</strong>
	 <span id="gpio27">%STATE27%</span></p>
	 <p><a href="/27"><button class="%BUTTON27%">%STATUS27%</button></a></p> 
<!--   <p><button class="%BUTTON27%" onclick="location.href='/27'">%STATUS27%</button></p>  -->
 <!--  <p><a href="/27" class="%BUTTON27%" role="button">%STATUS27%</a></p> -->
 </div>

 </div>

<!-- To receive text from Web browser  -->
    <div class="container">
      <div>
        <span class="card-title"><strong>Custom Payload: </strong></span>
      </div>
      <div>
        <input type="text" id="customInput" placeholder="e.g. 180 or JSON">
        <button id="btnSendCustom" class="btn-send" onclick="sendCustomPayload()">Send</button>
      </div>
    </div>

    <script>
      function sendCustomPayload() {
        const inputVal = document.getElementById("customInput").value;
        if (!inputVal) {
          alert("Please enter a payload before sending!");
          return;
        }

        fetch("/custom", {
          method: "POST",
          headers: { "Content-Type": "text/plain" },
          body: inputVal
        })
        .then(response => response.text())
        .then(data => {
          alert("ESP32 Response: " + data);
          document.getElementById("customInput").value = ""; // Clear input field
        })
        .catch(error => {
          console.error("Error:", error);
          alert("Failed to send payload.");
        });
      }
    </script>


	</body>
</html>
)rawliteral";

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

// Function to handle the root URL and show the current states
void handleRoot() {
  if (!isAuthenticated()) return;

  String html = htmlPage;
  html.replace("%STATE26%", output26State ? "ON" : "OFF");
  html.replace("%STATUS26%", output26State ? "To OFF" : "To ON");
  html.replace("%BUTTON26%", output26State ? "button off" : "button on");

  html.replace("%STATE27%", output27State ? "ON" : "OFF");
  html.replace("%STATUS27%", output27State ? "To OFF" : "To ON");
  html.replace("%BUTTON27%", output27State ? "button off" : "button on");

  server.send(200, "text/html", html);
}

// Function to handle turning GPIO 26 on
void handleGPIO26() {
  if (!isAuthenticated()) return;
  output26State = !output26State;
  digitalWrite(output26, output26State);
  handleRoot();
}

void handleGPIO27() {
  if (!isAuthenticated()) return;
  output27State = !output27State;
  digitalWrite(output27, output27State);
  handleRoot();
}

// Function to receive and process Custom Payload from web browser
void handleCustomPayload() {
  if (!isAuthenticated()) return;

  // Check if a POST body exists
  if (server.hasArg("plain")) {
    String payload = server.arg("plain");

    // Log received payload to Serial Monitor
    Serial.println("=================================");
    Serial.print("Custom Payload Received: ");
    Serial.println(payload);
    Serial.println("=================================");

    // TODO: Add custom logic here to parse/process 'payload'
    // e.g., if (payload == "180") { ... }

    server.send(200, "text/plain", "Payload '" + payload + "' received successfully!");
  } else {
    server.send(400, "text/plain", "Error: No payload received");
  }
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

  server.on("/26", handleGPIO26);
  server.on("/27", handleGPIO27);

// Register the new route for receiving Custom Payload via HTTP POST
  server.on("/custom", HTTP_POST, handleCustomPayload);


  // Start the web server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle incoming client requests
  server.handleClient();
}