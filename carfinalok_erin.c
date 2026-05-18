#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include <iostream>
#include <sstream>

// Cấu hình Servo
struct ServoPins {
  Servo servo;
  int servoPin;
  String servoName;
  int initialPosition;
};

std::vector<ServoPins> servoPins = {
  { Servo(), 27, "Base", 178 },
  { Servo(), 26, "Shoulder", 8 },
  { Servo(), 25, "Elbow", 92 },
  { Servo(), 33, "Gripper", 43 },
  { Servo(), 32, "Servo5", 0 },
  { Servo(), 21, "Servo6", 0 }
};

// Cấu hình Motor
#define MOTOR_A_IN1 5
#define MOTOR_A_IN2 4
#define MOTOR_B_IN1 2
#define MOTOR_B_IN2 15

// Cấu hình WiFi
const char *ssid = "23CE2";
const char *password = "15072003ke";
IPAddress local_IP(192, 168, 1, 88);
IPAddress gateway(192, 168, 1, 88);
IPAddress subnet(255, 255, 255, 0);

// Khởi tạo server
AsyncWebServer server(80);
AsyncWebSocket wsRobotArmInput("/RobotArmInput");

// Biến điều khiển
struct ControlState {
  bool forward = false;
  bool backward = false;
  bool left = false;
  bool right = false;
} controlState;

// Biến điều khiển Servo
struct ServoControl {
  bool isRotating = false;
  bool shouldRotate = false;
  unsigned long previousMillis = 0;
  const long interval = 5;
  int currentAngle5 = 0;
  int currentAngle6 = 0;
  int targetAngle5 = 0;
  int targetAngle6 = 0;
  const int stepSize = 3;
  bool rotationState = false;
  unsigned long servo6StartTime = 0;
  const int servo6Delay = 1000;
} servoControl;

// Hàm di chuyển servo đến vị trí mới và cập nhật slider trên web
void moveServoToPosition(int servoIndex, int targetAngle) {
  servoPins[servoIndex].servo.write(targetAngle);

  // Cập nhật giá trị slider trên web
  String servoName = servoPins[servoIndex].servoName;
  String jsCommand = "document.getElementById('" + servoName + "').value=" + String(targetAngle) + ";";
  jsCommand += "document.getElementById('" + servoName + "Value').innerText='" + String(targetAngle) + "°';";
  wsRobotArmInput.textAll(jsCommand);
}

// Trang HTML với giao diện màu cam tối giản
const char *htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, minimum-scale=1, user-scalable=no">
  <style>
    input[type="range"], button, .arrow-button, .action-btn {
      touch-action: manipulation;
    }


    :root {
      --primary-color: #FF8C00;
      --secondary-color: #FFA500;
      --accent-color: #FF6347;
      --dark-color: #333;
      --light-color: #FFF8DC;
      --slider-thumb: #FF4500;
      --slider-track: #FFD700;
      --preset-btn1: #4CAF50;
      --preset-btn2: #2196F3;
      --preset-btn3: #9C27B0;
    }
    
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background-color: var(--light-color);
      margin: 0;
      padding: 10px;
    }
    
    .noselect {
      -webkit-touch-callout: none;
      -webkit-user-select: none;
      -khtml-user-select: none;
      -moz-user-select: none;
      -ms-user-select: none;
      user-select: none;
    }
    
    .header {
      text-align: center;
      margin-bottom: 15px;
      color: var(--dark-color);
    }
    
    .header h1 {
      font-size: 2rem;
      margin: 5px 0;
      color: var(--primary-color);
      text-shadow: 1px 1px 2px rgba(0,0,0,0.1);
    }
    
    .control-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
      max-width: 600px;
      margin: 0 auto;
    }
    
    .control-panel {
      background-color: white;
      border-radius: 10px;
      padding: 15px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
    }
    
    .slider-container {
      margin-bottom: 15px;
    }
    
    .slider-label {
      display: flex;
      justify-content: space-between;
      margin-bottom: 5px;
      font-weight: bold;
      color: var(--dark-color);
      font-size: 14px;
    }
    
    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 10px;
      border-radius: 5px;
      background: var(--slider-track);
      outline: none;
    }
    
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--slider-thumb);
      cursor: pointer;
      transition: all 0.2s;
    }
    
    .slider::-moz-range-thumb {
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--slider-thumb);
      cursor: pointer;
    }
    
    .arrow-button {
      background-color: var(--primary-color);
      color: white;
      border-radius: 10px;
      width: 95px;
      height: 75px;
      font-size: 25px;
      border: none;
      margin: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      transition: all 0.2s;
    }
    
    .arrow-button:hover {
      background-color: var(--accent-color);
      transform: scale(1.05);
    }
    
    .arrow-grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 5px;
      width: 295px;
      margin: 0 auto;
    }
    
    .up { grid-column: 2; grid-row: 1; }
    .left { grid-column: 1; grid-row: 2; }
    .right { grid-column: 3; grid-row: 2; }
    .down { grid-column: 2; grid-row: 3; }
    
    .action-btn {
      background-color: var(--accent-color);
      color: white;
      border: none;
      border-radius: 5px;
      padding: 10px;
      width: 100%;
      font-weight: bold;
      margin-top: 10px;
      cursor: pointer;
      transition: all 0.2s;
    }
    
    .action-btn:hover {
      background-color: #FF4500;
    }
    
    .preset-btn-1 {
      background-color: var(--preset-btn1);
    }
    
    .preset-btn-2 {
      background-color: var(--preset-btn2);
    }
    
    .preset-btn-3 {
      background-color: var(--preset-btn3);
    }
    
    .preset-btn-1:hover {
      background-color: #3e8e41;
    }
    
    .preset-btn-2:hover {
      background-color: #0b7dda;
    }
    
    .preset-btn-3:hover {
      background-color: #7B1FA2;
    }
    
    .preset-container {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr;
      gap: 10px;
      margin-top: 10px;
    }
    
    @media (max-width: 600px) {
      .control-grid {
        grid-template-columns: 1fr;
      }
      .preset-container {
        grid-template-columns: 1fr 1fr;
      }
    }
  </style>
</head>
<body class="noselect">
  <div class="header">
    <h1>ROBOCAR 2025</h1>
  </div>
  
  <div class="control-grid">
    <div class="control-panel">
      <div class="slider-container">
        <div class="slider-label">
          <span>Base</span>
          <span id="BaseValue">90°</span>
        </div>
        <input type="range" min="0" max="180" value="178" class="slider" id="Base" 
              oninput='document.getElementById("BaseValue").innerText=this.value+"°";sendButtonInput("Base",this.value)'>
      </div>
      
      <div class="slider-container">
        <div class="slider-label">
          <span>Shoulder</span>
          <span id="ShoulderValue">90°</span>
        </div>
        <input type="range" min="0" max="180" value="8" class="slider" id="Shoulder" 
              oninput='document.getElementById("ShoulderValue").innerText=this.value+"°";sendButtonInput("Shoulder",this.value)'>
      </div>
      
      <div class="slider-container">
        <div class="slider-label">
          <span>Elbow</span>
          <span id="ElbowValue">90°</span>
        </div>
        <input type="range" min="0" max="180" value="92" class="slider" id="Elbow" 
              oninput='document.getElementById("ElbowValue").innerText=this.value+"°";sendButtonInput("Elbow",this.value)'>
      </div>
      
      <div class="slider-container">
        <div class="slider-label">
          <span>Gripper</span>
          <span id="GripperValue">90°</span>
        </div>
        <input type="range" min="0" max="180" value="43" class="slider" id="Gripper" 
              oninput='document.getElementById("GripperValue").innerText=this.value+"°";sendButtonInput("Gripper",this.value)'>
      </div>
      
      <div class="preset-container">
        <button id="Preset1" class="action-btn preset-btn-1" onclick='applyPreset(1)'>gap b 5cm</button>
        <button id="Preset2" class="action-btn preset-btn-2" onclick='applyPreset(2)'>cot 9cm</button>
        <button id="Preset3" class="action-btn preset-btn-3" onclick='applyPreset(3)'>ve nha b</button>
        <button id="Preset5" class="action-btn preset-btn-1" onclick='applyPreset(5)'>cot 15cm</button>
        <button id="Preset4" class="action-btn" onclick='applyPreset(4)'>Reset</button>
      </div>
      
      <button id="RotateServos" class="action-btn" ontouchend='onclickButton(this)'>SHOOT BALL</button>
    </div>
    
    <div class="control-panel" style="display:flex;flex-direction:column;align-items:center;">
      <div style="margin-bottom:15px;font-weight:bold;color:var(--dark-color);">CAR CONTROL</div>
      <div class="arrow-grid">
        <button class="arrow-button up" 
                ontouchstart='sendButtonInput("Forward","1")' 
                ontouchend='sendButtonInput("Forward","0")'>Go</button>
        <button class="arrow-button left" 
                ontouchstart='sendButtonInput("Left","1")' 
                ontouchend='sendButtonInput("Left","0")'>Left</button>
        <button class="arrow-button right" 
                ontouchstart='sendButtonInput("Right","1")' 
                ontouchend='sendButtonInput("Right","0")'>Right</button>
        <button class="arrow-button down" 
                ontouchstart='sendButtonInput("Backward","1")' 
                ontouchend='sendButtonInput("Backward","0")'>Back</button>
      </div>
    </div>
  </div>

  <script>
    var webSocketRobotArmInputUrl = "ws:\/\/" + window.location.hostname + "/RobotArmInput";
    var websocketRobotArmInput;
    
    function initRobotArmInputWebSocket() {
      websocketRobotArmInput = new WebSocket(webSocketRobotArmInputUrl);
      websocketRobotArmInput.onclose = function() {
        setTimeout(initRobotArmInputWebSocket, 2000);
      };
    }
    
    function sendButtonInput(key, value) {
      var data = key + "," + value;
      if (websocketRobotArmInput.readyState === WebSocket.OPEN) {
        websocketRobotArmInput.send(data);
      }
    }
    
    function onclickButton(button) {
      sendButtonInput(button.id, "1");
    }
    
    function applyPreset(presetNumber) {
      if (presetNumber === 1) {
        // Preset 1 values
        updateSlider('Base', 178);
        updateSlider('Shoulder', 53);
        updateSlider('Elbow', 110);
        sendButtonInput("Preset1", "1");
      } else if (presetNumber === 2) {
        // Preset 2 values
        updateSlider('Base', 178);
        updateSlider('Shoulder', 39);
        updateSlider('Elbow', 140);
        sendButtonInput("Preset2", "1");
      } else if (presetNumber === 3) {
        // Preset 3 values
        updateSlider('Base', 24);
        updateSlider('Shoulder', 7);
        updateSlider('Elbow', 111);
        sendButtonInput("Preset3", "1");
      }else if (presetNumber === 5){
        updateSlider('Base', 178);
        updateSlider('Shoulder', 2);
        updateSlider('Elbow', 178);
        sendButtonInput("Preset5", "1");
      }else if (presetNumber === 4){
        updateSlider('Base', 178);
        updateSlider('Shoulder', 8);
        updateSlider('Elbow', 92);
        sendButtonInput("Preset5", "1");
      }
    }
    
    function updateSlider(id, value) {
      document.getElementById(id).value = value;
      document.getElementById(id + 'Value').innerText = value + '°';
      sendButtonInput(id, value);
    }
    
    window.onload = function() {
      initRobotArmInputWebSocket();
      document.getElementById("BaseValue").innerText = document.getElementById("Base").value + "°";
      document.getElementById("ShoulderValue").innerText = document.getElementById("Shoulder").value + "°";
      document.getElementById("ElbowValue").innerText = document.getElementById("Elbow").value + "°";
      document.getElementById("GripperValue").innerText = document.getElementById("Gripper").value + "°";
    };
  </script>
</body>
</html>
)HTMLHOMEPAGE";

void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "File Not Found");
}

void controlMotor(int in1, int in2, int speed) {
  digitalWrite(in1, speed > 0 ? HIGH : LOW);
  digitalWrite(in2, speed < 0 ? HIGH : LOW);
}

void stopMotors() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, LOW);
}

void onRobotArmInputWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
          std::string myData = "";
          myData.assign((char *)data, len);
          std::istringstream ss(myData);
          std::string key, value;
          std::getline(ss, key, ',');
          std::getline(ss, value, ',');
          Serial.printf("Key [%s] Value[%s]\n", key.c_str(), value.c_str());

          if (key == "Forward") controlState.forward = (value == "1");
          else if (key == "Backward") controlState.backward = (value == "1");
          else if (key == "Left") controlState.left = (value == "1");
          else if (key == "Right") controlState.right = (value == "1");
          else if (key == "Base") writeServoValues(0, atoi(value.c_str()));
          else if (key == "Shoulder") writeServoValues(1, atoi(value.c_str()));
          else if (key == "Elbow") writeServoValues(2, atoi(value.c_str()));
          else if (key == "Gripper") writeServoValues(3, atoi(value.c_str()));
          else if (key == "RotateServos") {
            if (!servoControl.isRotating) {
              servoControl.shouldRotate = true;
              servoControl.rotationState = !servoControl.rotationState;
              if (servoControl.rotationState) {
                servoControl.targetAngle5 = 98;
                servoControl.targetAngle6 = 180;
              } else {
                servoControl.targetAngle5 = 0;
                servoControl.targetAngle6 = 0;
              }
              servoControl.servo6StartTime = millis();
            }
          } else if (key == "Preset1") {
            // Preset 1: Base 178, Shoulder 109, Elbow 84
            writeServoValues(0, 178);
            writeServoValues(1, 53);
            writeServoValues(2, 110);
          } else if (key == "Preset2") {
            // Preset 2: Base 178, Shoulder 15, Elbow 133
            writeServoValues(0, 178);
            writeServoValues(1, 39);
            writeServoValues(2, 140);
          } else if (key == "Preset3") {
            // Preset 3: Base 100, Shoulder 20, Elbow 90
            writeServoValues(0, 24);
            writeServoValues(1, 7);
            writeServoValues(2, 111);
          }else if(key == "Preset4"){
            writeServoValues(0, 178);
            writeServoValues(1, 38);
            writeServoValues(2, 105);
          }else if(key == "Preset5"){
            writeServoValues(0, 178);
            writeServoValues(1, 2);
            writeServoValues(2, 178);
          }
        }
        break;
      }
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;
  }
}

void writeServoValues(int servoIndex, int value) {
  servoPins[servoIndex].servo.write(value);
}

void setUpPinModes() {
  for (int i = 0; i < servoPins.size(); i++) {
    servoPins[i].servo.attach(servoPins[i].servoPin);
    servoPins[i].servo.write(servoPins[i].initialPosition);
  }

  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN1, OUTPUT);
  pinMode(MOTOR_B_IN2, OUTPUT);
}

void handleServoRotation() {
  if (!servoControl.shouldRotate) return;

  unsigned long currentMillis = millis();

  if (currentMillis - servoControl.previousMillis >= servoControl.interval) {
    servoControl.previousMillis = currentMillis;

    if (!servoControl.isRotating) {
      servoControl.isRotating = true;
    }

    if (servoControl.currentAngle5 < servoControl.targetAngle5) {
      servoControl.currentAngle5 = min(servoControl.currentAngle5 + servoControl.stepSize, servoControl.targetAngle5);
      writeServoValues(4, servoControl.currentAngle5);
    } else if (servoControl.currentAngle5 > servoControl.targetAngle5) {
      servoControl.currentAngle5 = max(servoControl.currentAngle5 - servoControl.stepSize, servoControl.targetAngle5);
      writeServoValues(4, servoControl.currentAngle5);
    }

    if (servoControl.rotationState) {
      if (currentMillis - servoControl.servo6StartTime >= servoControl.servo6Delay) {
        if (servoControl.currentAngle6 < servoControl.targetAngle6) {
          servoControl.currentAngle6 = min(servoControl.currentAngle6 + servoControl.stepSize, servoControl.targetAngle6);
          writeServoValues(5, servoControl.currentAngle6);
        }
      }
    } else {
      if (servoControl.currentAngle6 > servoControl.targetAngle6) {
        servoControl.currentAngle6 = max(servoControl.currentAngle6 - servoControl.stepSize, servoControl.targetAngle6);
        writeServoValues(5, servoControl.currentAngle6);
      }
    }

    if (servoControl.currentAngle5 == servoControl.targetAngle5 && servoControl.currentAngle6 == servoControl.targetAngle6) {
      servoControl.shouldRotate = false;
      servoControl.isRotating = false;
    }
  }
}

void controlVehicle() {
  if (controlState.forward && !controlState.backward && !controlState.left && !controlState.right) {
    controlMotor(MOTOR_A_IN1, MOTOR_A_IN2, -255);
    controlMotor(MOTOR_B_IN1, MOTOR_B_IN2, 255);
  } else if (controlState.backward && !controlState.forward && !controlState.left && !controlState.right) {
    controlMotor(MOTOR_A_IN1, MOTOR_A_IN2, 255);
    controlMotor(MOTOR_B_IN1, MOTOR_B_IN2, -255);
  } else if (controlState.left && !controlState.forward && !controlState.backward && !controlState.right) {
    controlMotor(MOTOR_A_IN1, MOTOR_A_IN2, 255);
    controlMotor(MOTOR_B_IN1, MOTOR_B_IN2, 255);
  } else if (controlState.right && !controlState.forward && !controlState.backward && !controlState.left) {
    controlMotor(MOTOR_A_IN1, MOTOR_A_IN2, -255);
    controlMotor(MOTOR_B_IN1, MOTOR_B_IN2, -255);
  } else {
    stopMotors();
  }
}

void setup() {
  setUpPinModes();
  Serial.begin(115200);

  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }

  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  wsRobotArmInput.onEvent(onRobotArmInputWebSocketEvent);
  server.addHandler(&wsRobotArmInput);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  wsRobotArmInput.cleanupClients();
  controlVehicle();
  handleServoRotation();
}