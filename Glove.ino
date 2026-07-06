#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// --- PIN DEFINITIONS (ESP32-C3) ---
#define CE_PIN   3
#define CSN_PIN  7
#define SCK_PIN  6
#define MISO_PIN 5
#define MOSI_PIN 4

#define I2C_SDA  8
#define I2C_SCL  9

#define BTN1_PIN 2
#define BTN2_PIN 0

// ===== STABLE FILTER (ANTI-NOISE) =====
unsigned long lastChangeTime1 = 0;
unsigned long lastChangeTime2 = 0;

const unsigned long stableDelay = 10;

int rawState1 = HIGH;
int rawState2 = HIGH;

int stableState1 = HIGH;
int stableState2 = HIGH;

// --- STATUS FLAGS ---
bool isOledOK = false;
bool isMpuOK  = false;
bool isNrfOK  = false;

// --- OFFSET ---
int pitchOffset = 0;
int rollOffset = 0;

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "GOAT1";

Adafruit_SSD1306 display(128, 64, &Wire, -1);
Adafruit_MPU6050 mpu;

struct __attribute__((packed)) Data_Package {
  int16_t pitch;   
  int16_t roll;    
  uint8_t button1; 
  uint8_t button2;
};
Data_Package myData;

void setup() {
Serial.begin(115200);
delay(100);

pinMode(BTN1_PIN, INPUT_PULLUP);
pinMode(BTN2_PIN, INPUT_PULLUP);

Wire.begin(I2C_SDA, I2C_SCL);
Wire.setClock(400000);
Wire.setTimeOut(100);

// OLED
Wire.beginTransmission(0x3C);
if (Wire.endTransmission() == 0 && display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
isOledOK = true;
display.clearDisplay();
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(0, 10);
display.println("Booting...");
display.display();
}

// MPU6050
if (mpu.begin()) {
isMpuOK = true;
mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
mpu.setGyroRange(MPU6050_RANGE_250_DEG);
mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

// NRF24
SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);
if (radio.begin()) {
isNrfOK = true;
radio.openWritingPipe(address);
radio.setPALevel(RF24_PA_HIGH);
radio.setDataRate(RF24_1MBPS);
radio.stopListening();
}
}

void loop() {

// ===== FILTER BTN1 =====
int reading1 = digitalRead(BTN1_PIN);
if (reading1 != rawState1) {
rawState1 = reading1;
lastChangeTime1 = millis();
}
if ((millis() - lastChangeTime1) > stableDelay) {
stableState1 = rawState1;
}

// ===== FILTER BTN2 =====
int reading2 = digitalRead(BTN2_PIN);
if (reading2 != rawState2) {
rawState2 = reading2;
lastChangeTime2 = millis();
}
if ((millis() - lastChangeTime2) > stableDelay) {
stableState2 = rawState2;
}

myData.button1 = !stableState1;
myData.button2 = !stableState2;

// --- MPU ---
if (isMpuOK) {
sensors_event_t a, g, temp;
mpu.getEvent(&a, &g, &temp);


int standardPitch = (int)(atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI);
int standardRoll  = (int)(atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI);

int rawPitch = -standardRoll; 
int rawRoll  = standardPitch;

rawPitch -= pitchOffset;
rawRoll  -= rollOffset;

int cleanPitch = (abs(rawPitch) <= 15) ? 0 : rawPitch;
int cleanRoll  = (abs(rawRoll) <= 15) ? 0 : rawRoll;

if (abs(cleanRoll) >= abs(cleanPitch)) {
  myData.roll = cleanRoll;
  myData.pitch = 0;
} else {
  myData.pitch = cleanPitch;
  myData.roll = 0;
}


} else {
myData.pitch = 0;
myData.roll = 0;
}

if (myData.button1 == 1 || myData.button2 == 1) {
myData.pitch = 0;
myData.roll = 0;
}

bool txSuccess = false;
if (isNrfOK) {
txSuccess = radio.write(&myData, sizeof(Data_Package));
}

if (isOledOK) {
display.clearDisplay();


int centerX = 32, centerY = 32, radius = 30, deadzoneRadius = 4; 
display.drawCircle(centerX, centerY, radius, WHITE);
display.drawCircle(centerX, centerY, deadzoneRadius, WHITE);
display.drawLine(centerX - radius, centerY, centerX + radius, centerY, WHITE); 
display.drawLine(centerX, centerY - radius, centerX, centerY + radius, WHITE); 

int indicatorX = map(myData.roll, -90, 90, centerX + radius, centerX - radius);
indicatorX = constrain(indicatorX, centerX - radius, centerX + radius);

int indicatorY = map(myData.pitch, -90, 90, centerY + radius, centerY - radius);
indicatorY = constrain(indicatorY, centerY - radius, centerY + radius);

display.fillCircle(indicatorX, indicatorY, 3, WHITE);

int textX = 66; 
display.setCursor(textX, 4);
display.print(isNrfOK ? (txSuccess ? "TX:Connect" : "TX:Fail") : "TX:Error");
display.setCursor(textX, 20); display.print("P: "); display.print(myData.pitch);
display.setCursor(textX, 36); display.print("R: "); display.print(myData.roll);
display.setCursor(textX, 52); display.print("B1B2:"); display.print(myData.button1); display.print(myData.button2);
display.display();


}

delay(50);
}
