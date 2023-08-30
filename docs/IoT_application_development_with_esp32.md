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

### 9. ESP-IDF FreeRTOS

- ESP-IDF RTOS differences from Vanilla FreeRTOS and ESP-IDF application startup.
- FreeRTOS task states.
- ESP-IDF FreeRTOS task creation and xTaskCreatePinnedToCore API.
- Use xTaskDelay().

- FreeRTOS is a real-time operating system kernel for embedded devices. The scheduler in a RTOS is designed to provide predictable execution pattern, and often embedded systems have real time requirements, which means that they must respond to a certain event within a strictly defined time or deadline. The guarantee to meet these requirements can only be made if the behavior of the OS scheduler can be predicted or is deterministic.

- ESP-IDF FreeRTOS: [link](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html)

  - Differences from Vanilla FreeRTOS
    - The ESP-IDF FreeRTOS is adapted for multi-core support.
    - Unlike Vanilla FreeRTOS, we don't have to call `vTaskStartScheduler()`.
    - FreeRTOS task stack size is specified in bytes in ESP-IDF FreeRTOS not WORDs.

  - Application Startup Flow:
    - app_main and application startup: [link](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/startup.html)
    - The high level view of startup process is as follows:
      - 1. First stage boot-loader in ROM loads second-stage boot-loader image to RAM (IRAM & DRAM) from flash offset 0x1000.
      - 2. Second stage boot-loader loads partition table and main app image from flash. Main app incorporates both RAM segments and read-only mapped via flash cache.
      - 3. Application startup executes. At this point the second CPU and RTOS scheduler are started.

    - Application startup covers everything that happens after the app starts executing and before the `app_main` function starts running inside main task. This is also spit into three stages:
      - Port initialization of hardware and basic C runtime environment.
      - System initialization of software services and FreeRTOS.
      - Running the main task and calling `app_main`.

- FreeRTOS Task Creation:
  - Create tasks using FreeRTOS xTaskCreate() APIs.
    - We must include `freertos/task.h`.
    - `xTaskCreate()` -> Lets the ESP-IDF FreeRTOS choose which core the task runs on.
    - `xTaskCreatePinnedToCore()` -> Allows for specifying which core the task runs on.

- `vTaskDelay()` is used to send a task into BLOCKED state for a set number of Ticks.
  - The actual time that the task remain blocked depends on the tick rate.
  - The constant portTICK_PERIOD_MS can be used to calculate real time from the tick rate - with the resolution of one tick period.
