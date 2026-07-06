# 🧤 MECAGLOVE – Gesture Controlled Mecanum Robot

## 🚀 Overview

MECAGLOVE is a wireless gesture-controlled robotic system using a glove equipped with motion sensors. The system allows intuitive control of a mecanum-wheel robot through hand movements.

## 🧩 System Architecture

The project consists of two main parts:

* **Glove (Transmitter)**

  * ESP32-C3
  * MPU6050 (motion sensing)
  * NRF24L01 (wireless communication)
  * Buttons for additional control

* **Robot (Receiver)**

  * Arduino Mega
  * NRF24L01
  * 4 DC motors with mecanum wheels
  * Motor drivers (TB6612FNG or equivalent)

---

## 🎮 Features

* Real-time wireless control using NRF24L01
* Gesture-based movement (tilt to move)
* Mecanum wheel omnidirectional motion
* Expandable with additional buttons and modes

---

## ⚙️ How It Works

1. The glove reads motion data from the MPU6050
2. Data is converted into pitch and roll values
3. Values are sent wirelessly via NRF24L01
4. The robot receives the data
5. Motor speeds are calculated using mecanum kinematics
6. The robot moves accordingly

---

## 🛠️ Hardware Requirements

### Transmitter

* ESP32-C3
* MPU6050
* NRF24L01
* OLED SSD1306 (optional)
* Push buttons

### Receiver

* Arduino Mega (or compatible)
* NRF24L01
* 4 DC motors
* 2 Motor drivers (TB6612FNG recommended)
* Mecanum wheels

---

## 📡 Communication

* Protocol: SPI (NRF24L01)
* Address: `"GOAT1"`
* Data Packet:

```cpp
struct Data_Package {
  int16_t pitch;
  int16_t roll;
  uint8_t button1;
  uint8_t button2;
};
```

---

## 🧠 Control Logic

* Pitch → Forward / Backward
* Roll → Left / Right strafing
* Button → Custom actions (expandable)

---

## 📦 Future Improvements

* Add PID stabilization
* Gesture filtering (Kalman / Complementary filter)
* Speed modes
* OLED UI enhancements
* Battery monitoring

---

## 👨‍💻 Author

Developed as an embedded systems and robotics project.

---

## ⭐ Notes

This project is ideal for learning:

* Embedded systems
* Wireless communication
* Robotics kinematics
* Sensor integration

---

🔥 *Feel free to fork, improve, and build your own smart robotic system!*
