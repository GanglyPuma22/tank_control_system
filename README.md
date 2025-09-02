
# 🦎 Tank Control System

## Overview
The Tank Control System is a versatile, extensible platform designed to manage and automate environmental conditions for animal enclosures—originally tailored for lizard habitats 🦎, but easily adaptable to a wide range of applications such as aquariums 🐠, terrariums 🌱, and smart home projects 🏠.

## ✨ Key Features
- **🌡️ Sensor Integration:** Supports temperature, humidity, and other environmental sensors for real-time monitoring.
- **📷 Camera Integration:** Captures and streams live video from inside the tank, viewable via a web interface from anywhere (WAN support).
- **🌐 Web-Based Control:** Provides a web page for remote monitoring and control of tank devices (lights, heat lamps, etc.).
- **⏰ Time-of-Day & Temperature Automation:** Automatically controls devices based on user-defined schedules and temperature thresholds.
- **🧩 Extensible Device Management:** Easily add or modify devices and sensors for different species or use cases.

## 🚀 Planned Features
- **🌦️ Weather Data Integration:** Match tank conditions to the native environment of the animal by pulling real-time weather data from their natural habitat. This includes temperature/humidity control and simulated sunrise/sunset and light levels throughout the day.
- **🤖 AI Computer Vision:** Integrate AI-based image analysis to detect key events (e.g., animal activity, feeding, abnormal behavior) and send notifications or alerts.
- **💡 Light Sequencing & Intensity Control:** Create a custom order and timing in which lights should turn on/off, and set intensities of the included LED strip for different times of day.
- **🔥 Mountable IR Heat Sensor & Multi Sensor Control:** Many existing temperature sensors fail to accurately capture the surface temperature of the basking spot. A mountable IR sensor solves that issue - it just needs line of sight to the surface. Additionally, multi sensor control support lets users maintain hot and cold side temperatures.

## 🛠️ Example Use Cases
- 🦎 Lizard/reptile enclosures with precise day/night and temperature cycles
- 🐟 Aquarium automation (lighting, feeding, water quality monitoring)
- 🏠 Smart home environmental control

## 🚦 Getting Started
1. **🔌 Hardware:**
   - ESP32-based controller (e.g., M5Stamp C3)
   - Supported sensors (DHT11, etc.)
   - Camera module (OV2640 or similar)
   - Relays for device control (lights, heat lamps)
2. **💻 Software:**
   - PlatformIO/Arduino framework
   - Configure Wi-Fi and Firebase credentials in `src/config/Credentials.h`
   - Build and upload the firmware
   - Firebase RealtimeDatabase needed for LAN control and stream viewing, for WAN you need to create a Firebase Web App.
3. **🌍 Web Interface:**
   - For LAN: Instruction coming soon!
   - For WAN: Instruction coming soon!

## 🧩 Extending the System
- ➕ Add new sensors or devices by creating new classes in the `src/devices/` or `src/sensors/` directories
- 🛠️ Modify automation logic in `main.cpp` or device classes
- 🤝 Integrate additional APIs or AI modules as needed

## 🤝 Contributing
Contributions are welcome! Please open issues or submit pull requests for new features, bug fixes, or documentation improvements.

---

*This project is under active development. Stay tuned for more features and improvements!* 🚧
