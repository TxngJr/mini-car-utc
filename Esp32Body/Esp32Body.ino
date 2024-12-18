#include <Arduino.h>
#include <HardwareSerial.h>

#define FRONT_LEFT_FORWARD_PIN 19
#define FRONT_LEFT_BACKWARD_PIN 18
#define FRONT_LEFT_PWM_PIN 2

#define FRONT_RIGHT_FORWARD_PIN 21
#define FRONT_RIGHT_BACKWARD_PIN 4
#define FRONT_RIGHT_PWM_PIN 15

#define BACK_LEFT_FORWARD_PIN 25
#define BACK_LEFT_BACKWARD_PIN 26
#define BACK_LEFT_PWM_PIN 12

#define BACK_RIGHT_FORWARD_PIN 27
#define BACK_RIGHT_BACKWARD_PIN 14
#define BACK_RIGHT_PWM_PIN 13

#define FREQUENCY 5000
#define RESOLUTION 8

#define CHANNEL_FRONT_LEFT 0
#define CHANNEL_FRONT_RIGHT 1
#define CHANNEL_BACK_LEFT 2
#define CHANNEL_BACK_RIGHT 3

#define SPEED_ROTATION 200

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  int pins[] = { FRONT_LEFT_FORWARD_PIN, FRONT_LEFT_BACKWARD_PIN, FRONT_LEFT_PWM_PIN,
                 FRONT_RIGHT_FORWARD_PIN, FRONT_RIGHT_BACKWARD_PIN, FRONT_RIGHT_PWM_PIN,
                 BACK_LEFT_FORWARD_PIN, BACK_LEFT_BACKWARD_PIN, BACK_LEFT_PWM_PIN,
                 BACK_RIGHT_FORWARD_PIN, BACK_RIGHT_BACKWARD_PIN, BACK_RIGHT_PWM_PIN };

  for (int pin : pins) {
    pinMode(pin, OUTPUT);
  }

  ledcSetup(CHANNEL_FRONT_LEFT, FREQUENCY, RESOLUTION);
  ledcSetup(CHANNEL_FRONT_RIGHT, FREQUENCY, RESOLUTION);
  ledcSetup(CHANNEL_BACK_LEFT, FREQUENCY, RESOLUTION);
  ledcSetup(CHANNEL_BACK_RIGHT, FREQUENCY, RESOLUTION);

  ledcAttachPin(FRONT_LEFT_PWM_PIN, CHANNEL_FRONT_LEFT);
  ledcAttachPin(FRONT_RIGHT_PWM_PIN, CHANNEL_FRONT_RIGHT);
  ledcAttachPin(BACK_LEFT_PWM_PIN, CHANNEL_BACK_LEFT);
  ledcAttachPin(BACK_RIGHT_PWM_PIN, CHANNEL_BACK_RIGHT);
}
int mapJoystickToPWM(int value) {
  return map(value, -20, 20, -255, 255);
}

void moveWheel(int channel, int forwardPin, int backwardPin, int speed) {
  if (speed > 0) {
    digitalWrite(forwardPin, HIGH);
    digitalWrite(backwardPin, LOW);
  } else if (speed < 0) {
    digitalWrite(forwardPin, LOW);
    digitalWrite(backwardPin, HIGH);
    speed = -speed;
  } else {
    digitalWrite(forwardPin, LOW);
    digitalWrite(backwardPin, LOW);
  }
  ledcWrite(channel, speed);
}

void loop() {
  static bool stringComplete = false;
  static String receivedData = "";
  if (Serial2.available()) {
    char inChar = (char)Serial2.read();
    receivedData += inChar;

    if (inChar == '\n') {
      stringComplete = true;
    }
  }

  if (stringComplete) {
    int pos1 = receivedData.indexOf(',');
    int pos2 = receivedData.indexOf(',', pos1 + 1);

    int x = receivedData.substring(0, pos1).toInt();
    int y = receivedData.substring(pos1 + 1, pos2).toInt();
    int a = receivedData.substring(pos2 + 1).toInt();

    x = -x;
    y = -y;

    // Map the joystick values to the PWM range
    int speedX = mapJoystickToPWM(x);
    int speedY = mapJoystickToPWM(y);

    // Calculate wheel speeds
    int speedFL = speedY + speedX;
    int speedFR = speedY - speedX;
    int speedBL = speedY - speedX;
    int speedBR = speedY + speedX;

    speedFL = constrain(speedFL, -255, 255);
    speedFR = constrain(speedFR, -255, 255);
    speedBL = constrain(speedBL, -255, 255);
    speedBR = constrain(speedBR, -255, 255);

    Serial.println(speedFL);
    Serial.println(speedFR);
    Serial.println(speedBL);
    Serial.println(speedBR);

    if (a == 1) {
      speedFL = SPEED_ROTATION;
      speedFR = -SPEED_ROTATION;
      speedBL = SPEED_ROTATION;
      speedBR = -SPEED_ROTATION;
    } else if (a == 2) {
      speedFL = -SPEED_ROTATION;
      speedFR = SPEED_ROTATION;
      speedBL = -SPEED_ROTATION;
      speedBR = SPEED_ROTATION;
    }

    moveWheel(CHANNEL_FRONT_LEFT, FRONT_LEFT_FORWARD_PIN, FRONT_LEFT_BACKWARD_PIN, speedFL);
    moveWheel(CHANNEL_FRONT_RIGHT, FRONT_RIGHT_FORWARD_PIN, FRONT_RIGHT_BACKWARD_PIN, speedFR);
    moveWheel(CHANNEL_BACK_LEFT, BACK_LEFT_FORWARD_PIN, BACK_LEFT_BACKWARD_PIN, speedBL);
    moveWheel(CHANNEL_BACK_RIGHT, BACK_RIGHT_FORWARD_PIN, BACK_RIGHT_BACKWARD_PIN, speedBR);
    receivedData = "";
    stringComplete = false;
  }
}
