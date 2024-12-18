#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <esp_camera.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include "esp_camera.h"

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

const char* ssid = "UTCMiniCar";
const char* password = "12345678";

AsyncWebServer server(80);
WebSocketsServer webSocketCamera = WebSocketsServer(81);
WebSocketsServer webSocketJoystick = WebSocketsServer(82);

const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>UTC Game MiniCar</title>
  <meta name="apple-mobile-web-app-capable" content="yes" />
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent" />
  <style>
    body,
    html {
      margin: 0;
      padding: 0;
      width: 100%;
      height: 100%;
      overflow: hidden;
      font-family: sans-serif;
      background-color: #000;
    }

    .game-container {
      position: relative;
      width: 100%;
      height: 100%;
      perspective: 1000px;
    }

    .game-background {
      position: absolute;
      width: 100%;
      height: 100%;
      background-color: #000;
      background-size: cover;
      background-position: center;
      pointer-events: none;
    }

    .joystick-container {
      position: absolute;
      bottom: 40px;
      left: 40px;
      width: 130px;
      height: 130px;
      background-color: rgba(255, 255, 255, 0.1);
      border-radius: 50%;
      display: flex;
      justify-content: center;
      align-items: center;
      touch-action: none;
    }

    .joystick {
      width: 60px;
      height: 60px;
      background-color: rgba(255, 255, 255, 0.8);
      border-radius: 50%;
      position: relative;
      touch-action: none;
      -webkit-user-select: none;
      -ms-user-select: none;
      user-select: none;
      -webkit-tap-highlight-color: transparent;
    }

    #fullscreen-btn {
      position: absolute;
      top: 10px;
      right: 20px;
      background-color: #333;
      color: #fff;
      border: none;
      padding: 10px;
      border-radius: 5px;
      cursor: pointer;
      -webkit-user-select: none;
      -ms-user-select: none;
      user-select: none;
      -webkit-tap-highlight-color: transparent;
    }

    .arrow-buttons {
      position: absolute;
      bottom: 40px;
      right: 40px;
      display: flex;
      flex-direction: row;
      -webkit-user-select: none;
      -ms-user-select: none;
      user-select: none;
      -webkit-tap-highlight-color: transparent;
    }

    .arrow-button {
      width: 60px;
      height: 60px;
      margin-left: 10px;
      background-color: rgba(255, 255, 255, 0.8);
      border: none;
      border-radius: 50%;
      display: flex;
      justify-content: center;
      align-items: center;
      font-size: 24px;
      cursor: pointer;
      user-select: none;
      -webkit-user-select: none;
      -ms-user-select: none;
      user-select: none;
      -webkit-tap-highlight-color: transparent;
    }

    .arrow-button:active {
      background-color: rgba(255, 255, 255, 0.6);
    }
  </style>
</head>

<body>
  <div class="game-container">
    <img id="streamImage" class="game-background" />
    <div class="joystick-container">
      <div class="joystick" id="joystick"></div>
    </div>
    <div class="arrow-buttons">
      <button class="arrow-button" id="arrow-left">⟲</button>
      <button class="arrow-button" id="arrow-right">⟳</button>
    </div>
    <button id="fullscreen-btn">Fullscreen</button>
  </div>
  <script>
    document.addEventListener("DOMContentLoaded", () => {
      const streamImage = document.getElementById("streamImage");
      const joystick = document.getElementById("joystick");
      const joystickContainer = document.querySelector(".joystick-container");
      const fullscreenButton = document.getElementById("fullscreen-btn");
      const arrowLeft = document.getElementById("arrow-left");
      const arrowRight = document.getElementById("arrow-right");
      let joystickActive = false;
      let startX, startY;
      let lastSentTime = 0;
      const intervalTime = 50;
      let rotateInterval;

      const socketCamera = new WebSocket(
        "ws://" + window.location.hostname + ":81/"
      );

      socketCamera.addEventListener("open", (event) => {
        console.log("Hello Kub! socketCamera");
      });

      const socketJoystick = new WebSocket(
        "ws://" + window.location.hostname + ":82/"
      );

      socketJoystick.addEventListener("open", (event) => {
        console.log("Hello Kub! socketJoystick");
      });

      socketCamera.addEventListener("message", (event) => {
        const blob = event.data;
        const imageUrl = URL.createObjectURL(blob);

        streamImage.src = imageUrl;

        streamImage.onload = () => {
          URL.revokeObjectURL(imageUrl);
        };
      });

      const sendJoystickData = (x, y, a) => {
        const payload = JSON.stringify({ x: Math.floor(x), y: Math.floor(y), a: a });
        socketJoystick.send(payload);
      };

      joystick.addEventListener("touchstart", (e) => {
        joystickActive = true;
        startX = e.touches[0].clientX;
        startY = e.touches[0].clientY;
      });

      joystick.addEventListener("touchmove", (e) => {
        const currentTime = Date.now();
        if (!joystickActive) return;
        const moveX = e.touches[0].clientX - startX;
        const moveY = e.touches[0].clientY - startY;

        const distance = Math.min(
          Math.sqrt(moveX * moveX + moveY * moveY),
          40
        );

        const angle = Math.atan2(moveY, moveX);

        const x = distance * Math.cos(angle);
        const y = distance * Math.sin(angle);

        joystick.style.transform = `translate(${x}px, ${y}px)`;

        if (currentTime - lastSentTime > intervalTime) {
          lastSentTime = currentTime;
          sendJoystickData(x, y, 0);
        }
      });

      joystick.addEventListener("touchend", () => {
        joystickActive = false;
        joystick.style.transform = `translate(0px, 0px)`;
        sendJoystickData(0, 0, 0);
      });

      arrowLeft.addEventListener("touchstart", () => {
        rotateInterval = setInterval(() => {
          sendJoystickData(0, 0, 1);
        }, intervalTime);
      });

      arrowRight.addEventListener("touchstart", () => {
        rotateInterval = setInterval(() => {
          sendJoystickData(0, 0, 2);
        }, intervalTime);
      });

      arrowLeft.addEventListener("touchend", () => {
        clearInterval(rotateInterval);
        sendJoystickData(0, 0, 0);
      });

      arrowRight.addEventListener("touchend", () => {
        clearInterval(rotateInterval);
        sendJoystickData(0, 0, 0);
      });

      fullscreenButton.addEventListener("click", () => {
        if (
          !document.fullscreenElement &&
          !document.webkitFullscreenElement &&
          !document.msFullscreenElement
        ) {
          if (document.documentElement.requestFullscreen) {
            document.documentElement.requestFullscreen();
          } else if (document.documentElement.webkitRequestFullscreen) {
            document.documentElement.webkitRequestFullscreen();
          } else if (document.documentElement.msRequestFullscreen) {
            document.documentElement.msRequestFullscreen();
          }
          fullscreenButton.textContent = "Close";
        } else {
          if (document.exitFullscreen) {
            document.exitFullscreen();
          } else if (document.webkitExitFullscreen) {
            document.webkitExitFullscreen();
          } else if (document.msExitFullscreen) {
            document.msExitFullscreen();
          }
          fullscreenButton.textContent = "Fullscreen";
        }
      });
    });
  </script>
</body>

</html>
)rawliteral";

void onWebSocketJoystickEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_TEXT) {
    String message = String((char*)payload);
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (!error) {
      int x = doc["x"];
      int y = doc["y"];
      int a = doc["a"];
      Serial.println(String(x) + "," + String(y) + "," + String(a));
    }
  }
}

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_4;
  config.ledc_timer = LEDC_TIMER_2;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;  // Reduced from UXGA to SVGA for quicker processing
    config.jpeg_quality = 12;            // Reduced quality to reduce processing time
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    return;
  }
}

void sendCameraFrame() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    return;
  }
  webSocketCamera.broadcastBIN(fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void setup() {
  Serial.begin(115200);

  WiFi.softAP(ssid, password);

  setupCamera();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html);
  });

  webSocketCamera.begin();
  webSocketJoystick.onEvent(onWebSocketJoystickEvent);
  webSocketJoystick.begin();

  server.begin();
}

void loop() {
  webSocketCamera.loop();
  webSocketJoystick.loop();  // Ensure frequent calls to process joystick data quickly
  sendCameraFrame();         // Send camera frames after processing WebSocket events
}