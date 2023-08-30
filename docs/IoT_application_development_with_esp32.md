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

### 10. ESP-IDF Error handling

- Overview of Error Handling
- Error Codes
- Converting Error Codes to Error Messages
- ESP_ERROR_CHECK Macro
- Error Handling patterns.

- Error handling: Identifying and handling run-time errors is important for developing robust applications. There can be multiple kinds of run-time errors:
  - Recoverable errors:
    - Error indicated by functions through return values (error codes).
    - C++ exceptions, thrown using `throw` keyword.
  - Unrecoverable (fatal) errors:
    - Failed assertion (using `assert` macro and equivalent methods) and abort() calls.
    - CPU exceptions: access to protected regions of memory, illegal instruction, etc.
    - System level checks: watchdog timeout, cache access error, stack overflow, stack mashing, heap corruption, etc.

- Error Codes in brief:
  - Most ESP-IDF-specific functions use `esp_err_t` type to return error codes.
    - `esp_err_t` is a signed integer type.
    - Success (no error) is indicated with ESP_OK code, which is defined as zero.
  - Common error codes for generic failures (out of memory, timeout, invalid argument, etc.) are defined in `esp_err.h` file.
  - Various ESP-IDF header files defined possible error codes using preprocessor defines. Usually these defines start with `ESP_ERR_` prefix.

- Converting Error Codes to Error Messages
  - Conversion to strings for debug logging.
    - For each error code defined in ESP-IDF components, `esp_err_t` value can be converted to an error code name using `esp_err_to_name()` or `esp_err_to_name_r()` functions.

- `ESP_ERROR_CHECK` Macro
  - Similar to Assert...
    - `ESP_ERROR_CHECK` macro checks `esp_err_t` value rather than a bool condition.
      - If the argument of `ESP_ERROR_CHECK` is not equal `ESP_OK`, than an error message is printed on the console, and `abort()` is called.

- Strategies for handling errors:
  - Document: [link](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/error-handling.html#error-handling-patterns)
  - 1. Attempt to recover. Depending on the situation, we may try the following methods:
    - Retry the call after some time;
    - Attempt to de-initialize the driver and re-initialize it again;
    - Fix the error condition using an out-of-band mechanism (e.g reset an external peripheral which is not responding.)

    ```C
    esp_err_t err;
    do {
        err = sdio_slave_send_queue(addr, len, arg, timeout);
        // keep retrying while the sending queue is full
    } while (err == ESP_ERR_TIMEOUT);
    if (err != ESP_OK) {
        // handle other errors
    }
    ```

  - 2. Propagate the error to the caller. In some middleware components this means that a function must exit with same error code, making sure any resource allocations are rolled back.

    ```C
    sdmmc_card_t* card = calloc(1, sizeof(sdmmc_card_t));
    if (card == NULL) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = sdmmc_card_init(host, &card);
    if (err != ESP_OK) {
        // Clean up
        free(card);
        // Propagate the error to the upper layer (e.g. to notify the user).
        // Alternatively, application can define and return custom error code.
        return err;
    }
    ```

  - 3. Convert into unrecoverable error, for example using `ESP_ERROR_CHECK`.

    ```C
    ESP_ERROR_CHECK(spi_bus_initialize(host, bus_config, dma_chan));
    ```
