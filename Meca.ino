#include <SPI.h>
#include <RF24.h>

// --- NRF24L01 PINS ---
#define CE_PIN   48
#define CSN_PIN  49

// --- MOTOR PINS (DRIVER 1: PINS 2-7) ---
#define FL_PWM  2
#define FL_IN1  3
#define FL_IN2  4

#define FR_IN1  5
#define FR_IN2  6
#define FR_PWM  7

// --- MOTOR PINS (DRIVER 2: PINS 8-13) ---
#define BL_PWM  8
#define BL_IN1  9
#define BL_IN2  10

#define BR_IN1  11
#define BR_IN2  12
#define BR_PWM  13 

// --- SYSTEM OBJECTS ---
RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "GOAT1"; 

struct __attribute__((packed)) Data_Package {
  int16_t pitch; 
  int16_t roll;
  uint8_t button1;
  uint8_t button2;
};
Data_Package myData;

unsigned long lastReceiveTime = 0;
unsigned long lastBlinkTime = 0;
bool ledState = LOW;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("--- MEGA MECANUM RECEIVER BOOTING ---");

  pinMode(FL_PWM, OUTPUT); pinMode(FL_IN1, OUTPUT); pinMode(FL_IN2, OUTPUT);
  pinMode(FR_PWM, OUTPUT); pinMode(FR_IN1, OUTPUT); pinMode(FR_IN2, OUTPUT);
  pinMode(BL_PWM, OUTPUT); pinMode(BL_IN1, OUTPUT); pinMode(BL_IN2, OUTPUT);
  pinMode(BR_PWM, OUTPUT); pinMode(BR_IN1, OUTPUT); pinMode(BR_IN2, OUTPUT);
  
  stopAllMotors();

  if (!radio.begin()) {
    Serial.println("CRITICAL: NRF24L01 not found.");
    while (1) {
      digitalWrite(BR_IN1, LOW); digitalWrite(BR_IN2, LOW);
      digitalWrite(BR_PWM, HIGH); delay(100);
      digitalWrite(BR_PWM, LOW); delay(100);
    }
  }
  
  Serial.println("OK: NRF24L01 Ready. Waiting for Glove...");
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);
  radio.startListening(); 
}

void loop() {
  if (radio.available()) {
    radio.read(&myData, sizeof(Data_Package));
    lastReceiveTime = millis(); 
  }
  
  if (millis() - lastReceiveTime > 500) {
    // --- DISCONNECTED STATE (Blinking LED) ---
    driveMotor(FL_PWM, FL_IN1, FL_IN2, 0);
    driveMotor(FR_PWM, FR_IN1, FR_IN2, 0);
    driveMotor(BL_PWM, BL_IN1, BL_IN2, 0);

    digitalWrite(BR_IN1, LOW);
    digitalWrite(BR_IN2, LOW);

    if (millis() - lastBlinkTime > 250) {
      lastBlinkTime = millis();
      ledState = !ledState;
      digitalWrite(BR_PWM, ledState);
    }
  } else {
    // --- CONNECTED STATE ---
    executeMecanumKinematics();
  }
}

// --- MECANUM KINEMATICS & CONTROL ---
void executeMecanumKinematics() {
  int speedFL, speedFR, speedBL, speedBR;
  int spinSpeed = 160; 

  Serial.print("Pitch: "); Serial.print(myData.pitch);
  Serial.print(" | Roll: "); Serial.println(myData.roll);
  // 1. BUTTON PRIORITY (SPINNING)
  if (myData.button1 == 1) {
    // Spin Left (Counter-Clockwise)
    speedFL = -spinSpeed;
    speedBL = -spinSpeed;
    speedFR = spinSpeed;
    speedBR = spinSpeed;
  } 
  else if (myData.button2 == 1) {
    // Spin Right (Clockwise)
    speedFL = spinSpeed;
    speedBL = spinSpeed;
    speedFR = -spinSpeed;
    speedBR = -spinSpeed;
  } 
  // 2. RAW MECANUM TRANSLATION AXIS LOGIC
  else {
    // Roll controls Forward/Backward directly (Negative Roll = Backward)
    int yAxis = map(myData.pitch, -90, 90, -255, 255); 
    
    // Pitch controls Left/Right. 
    // Positive Pitch = Move Left (Requires negative X in Mecanum math)
    int xAxis = map(-myData.roll, -90, 90, -255, 255);  

    // Standard Mecanum Vector Math
    speedFL = yAxis + xAxis;
    speedFR = yAxis - xAxis;
    speedBL = yAxis - xAxis;
    speedBR = yAxis + xAxis;

    // Constrain outputs to prevent PWM overflows
    speedFL = constrain(speedFL, -255, 255);
    speedFR = constrain(speedFR, -255, 255);
    speedBL = constrain(speedBL, -255, 255);
    speedBR = constrain(speedBR, -255, 255);
  }

  // 3. APPLY TO HARDWARE DRIVERS
  driveMotor(FL_PWM, FL_IN1, FL_IN2, speedFL);
  driveMotor(FR_PWM, FR_IN1, FR_IN2, speedFR);
  driveMotor(BL_PWM, BL_IN1, BL_IN2, speedBL);
  driveMotor(BR_PWM, BR_IN1, BR_IN2, speedBR);
}

// --- LOW LEVEL MOTOR HARDWARE DRIVER ---
void driveMotor(int pwmPin, int in1, int in2, int speed) {
  if (speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    analogWrite(pwmPin, speed);
  } 
  else if (speed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    analogWrite(pwmPin, abs(speed)); 
  } 
  else {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, HIGH);
    analogWrite(pwmPin, 0);
  }
}

void stopAllMotors() {
  driveMotor(FL_PWM, FL_IN1, FL_IN2, 0);
  driveMotor(FR_PWM, FR_IN1, FR_IN2, 0);
  driveMotor(BL_PWM, BL_IN1, BL_IN2, 0);
  driveMotor(BR_PWM, BR_IN1, BR_IN2, 0);
}