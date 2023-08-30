# IoT Application development with ESP32 using the ESP-IDF

## I. Introduction

### 2. Hardware & Software Requirements

- Eclipse ESP-IDF plugin
  - Available for Linux, MacOS, Windows
  - All-in-one installer available for Windows.
    - Installs the ESP-IDF, ESP-IDF tools and Eclipse.
  - You can also install the Plugin into an existing Eclipse CDT installation.
  - Details here -> [link](https://github.com/espressif/idf-eclipse-plugin)

- Other features of the plugin:
  - Creating a new Cmake IDF project.
  - Create ESP launch target with multi-chip support.
  - Compiling the project.
  - Flashing the project.
  - Debugging the project.
  - Viewing serial output.

### 3. PlatformIO IDE for VSCode

- PlatformIO IDE is built as an extension of the VSCode.
- Setting Up the Project:
- 1. Click on `PlatformIO Home` button on the bottom PlatformIO Toolbar
- 2. Clink `+ New Project`
- 3. Select project name.
- 4. Select board: `Espressif ESP32 Dev Module`
- 5. Select Framework: Arduino
- 6. Open PlatformIO on activity bar.
- 7. Select your environment. The tool will auto-detected device via COM: `Auto-detected: COM12`
  - Or you can config in .ini file, for example:

    ```ini
    monitor_speed = 115200
    monitor_port = COM12
    upload_port = COM12
    ```

- 8. Build, Upload, Monitor, etc.

### 4. ESP 32 Kit

- [link](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html)
