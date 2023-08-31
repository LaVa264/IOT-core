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

### 11. Embedded C coding style

- Header files:

```C
#pragma once

/* Include directives --------------------------------------------------------*/
#include <Arduino.h>

/* Public defines ------------------------------------------------------------*/
#define CODING_STYLE    1                        /* Describe it if necessary. */

/* Extern variables ----------------------------------------------------------*/
extern esp_netif_t *g_sta;

/* Public types --------------------------------------------------------------*/

/**
 * @brief   Provide a description for your typedef.
 * @note    Add not if necessary.
 * @note    postfix enum with `_e`.
 */
typedef enum {
    STYLE_1 = 0,
    STYLE_2
} coding_style_e;

/**
 * @brief   Provide a description for your structure.
 * @note    Add not if necessary.
 * @note    postfix struct with `_t`.
 */
typedef struct {
    void *data;
    void *msgID;
} coding_style_t;

/* Public function prototypes ------------------------------------------------*/
/**
 * @brief   public function definitions description in the header file only.
 * @param   var_1   - describe var_1 here.
 * @param   var_2   - describe var_2 here.
 * @return  ESP_OK, if successful.
 */
esp_err_t coding_style_public(int var_1, int var_2);
```

- C source files:

```C
/* Include directives --------------------------------------------------------*/
/* 1. Standard libraries. */
#include <stdio.h>
#include <string.h>

/* 2. ESP-IDF headers. */
#include <esp_err.h>

/* 3. Application headers. */
#include "ota.h"


/* Global variables ----------------------------------------------------------*/
bool g_var = false;                             /* `g_` for global variables. */

/* Private variables ---------------------------------------------------------*/
static bool s_var = false;                      /* `s_` for static variables. */

/* Private function prototypes -----------------------------------------------*/

/**
 * @brief   private function definitions require a description.
 * @note    Use the `file_name` prefix for the function name e.g. `coding_style`
 * @param   var_1   - describe var_1 here.
 * @return  ESP_OK, if successful.
 */
static esp_err_t coding_style_static(int var_a);

/* Public function definition ------------------------------------------------*/
esp_err_t coding_style_public(int var_1, int var_2)
{
    return ESP_OK;
}

/* Private function definition -----------------------------------------------*/
static esp_err_t coding_style_static(int var_a)
{
    return ESP_OK;
}
```

## V. Components & Sensors Library

### 12. Components & Sensors Library overview

- Sensor Library Integration is Completely optional

## IIX. Wifi Application

### 17. Wifi Implementation overview

- Wifi Application (High-Level Perspective)
- About the Implementation:
  - The ESP32 should start its Access Point so that other devices can connect to it.
    - This enables users to access information e.g, sensor data, device info, connection status/information, user option to connect to and disconnect from an AP, display local time, etc.
  - The WiFi application will start an HTTP Server, which will support a web page.
  - The application will contain a FreeRTOS task that accepts FreeRTOS Queue messages (xQueueCreate(), xQueueSend() and xQueueReceive()) for event coordination.

- Simplified Overview of Wifi Application
  - 1. Connecting Device (mobile phone, laptop, etc.)
    - Become `station` of ESP32's SoftAP when connected.
    - DHCP service from the ESP32's SoftAP will dynamically assign an IP to your device.
    - Interact with the ESP32 via the webpage.

  - 2. ESP32 SoftAP/Station
    - AP/STA combination mode.
    - We assign an IP to the SoftAP; the interface of the ESP32 is statically configured.
    - DHCP server dynamically assigns an IP for connecting stations.
    - We set a maximum number of stations allowed to connect.

  - 3. Connecting Device
    - When the ESP32 connects to an AP, local time can be obtained utilizing SNTP ()(Simple Network Time Protocol)(if the AP is connected to the internet).
    - DHCP service from the AP, will dynamically assign an IP to the ESP32 in our application.

- ESP-IDF WiFi Driver APIs, in brief:
  - Wifi driver: [link](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html)
  - API reference: [link](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
  - ESP-NETIF (Network interface): [link](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_netif.html)

- ESP32 Wifi Feature list:
- The following features are supported:
  - 4 virtual WiFi Interfaces, which are STA, AP, Sniffer and reserved.
  - Station-only mode, AP-only mode, station/AP coexistence mode.
  - WPA/WPA2/WPA3/WPA2-Enterprise/WPA3-Enterprise/WAPI/WPS and DPP.
  - Modem-sleep.
  - Long Range mode, which supports up to 1KM of data traffic.
  - Up to 20M bit/s TCP through put and 30M bit/s UDP throughput over the air.
  - Sniffer.
  - Both fast scan and all-channel scan.
  - Multiple antennas.
  - Wifi aware (NAN).

- How to write a WiFi Application?
  - Preparation:
    - Generally, the most effective way to begin your own Wi-Fi application is to select an example which is similar to your own application, and port the useful part into your project. We recommend you to select an [example](https://github.com/espressif/esp-idf/tree/82cceabc6e/examples/wifi)
  - Setting Wi-Fi Compile-time Options.
  - Init Wi-Fi.
  - Start/Connect WiFi.
  - Event-handling: Generally, it is easy to write code in `sunny-day` scenarios, such as `WIFI_EVENT_STA_START` and `WIFI_EVENT_STA_CONNECTED`. The hard part is to write routines in `rainy-day` scenarios, such as `WIFI_EVENT_STA_DISCONNECTED`. Good handling of `rainy-day` scenarios is fundamental to robust WiFi applications.

- **The primary principle to write a robust application with WiFi API is to always check the error code and write the error-handling code**. Generally, the error-handling code can be used:
  - For recoverable errors, in which case you can write a recoverable-error code. For example, when `esp_wifi_start()` returns `ESP_ERR_NO_MEM`, the recoverable-code `vTaskDelay()` can be called in order to get a microsecond's delay for another try.
  - For non-recoverable, yet non-critical errors, in which case printing the error code is a good method for error handling.
  - For non-recoverable and also critical errors, in which case `assert` may be a good method for error handling.

- ESP32 Wi-Fi API Parameter Initialization:
  - When initializing struct parameters for the API, one of two approaches should be followed:
    - Explicitly set all fields of the parameter.
    - Use get API to get current configuration first, then set application specific fields.
  - Initializing or getting the entire structure is very important, because most of the time the value 0 indicates that default value is used.

- **ESP32 WiFi Programming Model**:

- The ESP32 Wi-Fi programming model is depicted as follows:
                      Default Handler                           User handler
|TCP Stack|--(event)-->| Event Task|--(callback or event)--> |Application task|
                            /\                                     ||
                            ||    (event)                          ||
                        |WiFi Driver|<========(API call)===========//

- The WiFi driver can be considered a black box that knows nothing about high-layer code, such as the TCP/IP Stack, Application Task, and Event Task.
- The application task (code) generally calls WiFi driver APIs to initialize WiFi and handles WiFi events when necessary. WiFi Driver receives API calls, handle them, and posts events to the application.
- WiFi event handling is based on the `esp_event` library. Events are sent by the WiFi driver to the default event loop. Application may handle these events in callbacks registered using `esp_event_handler_register()`. WiFi events are also handled by `esp_netif` component to provide a set of default behaviors. For example, when WiFi station connects to an AP, `esp_netif` will automatically start the DHCP client by default.

- **ESP-NETIF**:
  - The purpose of the `ESP-NETIF` library is twofold:
    - It provides an abstraction layer for the application on top of the TCP/IP stack. this allows applications to choose between IP stacks in the future.
    - The APIs it provides are thread-safe, event if the underlying TCP/IP stack APIs are not.

  - ESP-IDF currently implements ESP-NETIF for the lwIP TCP/IP stack only. However, the adapter itself is TCP/IP implementation-agnostic and allows different implementations.
  - **ESP-NETIF Architecture**:

```text
                       |          (A) USER CODE                 |
                       |                 Apps                   |
      .................| init          settings      events     |
      .                +----------------------------------------+
      .                   .                |           *
      .                   .                |           *
  --------+            +===========================+   *     +-----------------------+
          |            | new/config   get/set/apps |   *     | init                  |
          |            |                           |...*.....| Apps (DHCP, SNTP)     |
          |            |---------------------------|   *     |                       |
    init  |            |                           |****     |                       |
    start |************|  event handler            |*********|  DHCP                 |
    stop  |            |                           |         |                       |
          |            |---------------------------|         |                       |
          |            |                           |         |    NETIF              |
    +-----|            |                           |         +-----------------+     |
    | glue|---<----|---|  esp_netif_transmit       |--<------| netif_output    |     |
    |     |        |   |                           |         |                 |     |
    |     |--->----|---|  esp_netif_receive        |-->------| netif_input     |     |
    |     |        |   |                           |         + ----------------+     |
    |     |...<....|...|  esp_netif_free_rx_buffer |...<.....| packet buffer         |
    +-----|     |  |   |                           |         |                       |
          |     |  |   |                           |         |         (D)           |
    (B)   |     |  |   |          (C)              |         +-----------------------+
  --------+     |  |   +===========================+
COMMUNICATION   |  |                                               NETWORK STACK
DRIVER          |  |           ESP-NETIF
                |  |                                         +------------------+
                |  |   +---------------------------+.........| open/close       |
                |  |   |                           |         |                  |
                |  -<--|  l2tap_write              |-----<---|  write           |
                |      |                           |         |                  |
                ---->--|  esp_vfs_l2tap_eth_filter |----->---|  read            |
                       |                           |         |                  |
                       |            (E)            |         +------------------+
                       +---------------------------+
                                                                   USER CODE
                             ESP-NETIF L2 TAP
```

- Data and Event Flow in the diagram:
  - `.........` Initialization line from user code to ESP-NETIF and communication driver.
  - `--<--->--` Data packets going from communication media to TCP/IP stack and back.
  - `********` Events aggregated in ESP-NETIF propagate to the driver, user code, and network stack.
  - `|` User settings and runtime configuration.

- ESP-NETIF Interaction
- **A. User code, Boilerplate**
  - Overall application interaction with a specific IO driver for communication media and configured TCP/IP network stack is abstracted using ESP-NETIF APIs and is outlined as below:
    - 1. Initialization code
      - 1.1. Initialize IO driver.
      - 1.2. Creates a new instance of ESP-NETIF and configure it with:
        - ESP-NETIF specific options (flags, behavior, name).
        - Network stack options (netif init and input functions, not publicly available).
        - IO driver specific options (transmit, free rx buffer functions, IO driver handle)
      - 1.3. Attaches the IO driver handle to the ESP-NETIF instance created in the above steps.
      - 1.4. Configures event handlers
        - Use default handlers for common interfaces defined in IO drivers; or defined a specific handler for customized behavior or new interfaces.
        - Register handlers for app-related events (such as IP lost or acquired).
    - 2. Interaction with network interfaces using ESP-NETIF API.
      - 2.1. Gets and sets TCP/IP-related parameters (DHCP, IP, etc)
      - 2.2. Receives IP events (connect or disconnect)
      - 2.3. Controls application lifecycle (set interface up or down)

- **B. Communication Driver, IO Driver, and Media Driver**
  - Communication driver plays these two important roles in relation to ESP-NETIF:
    - 1. Event handlers: Defines behavior patterns of interaction with ESP-NETIF (e.g., ethernet link-up -> turn netif on)
    - 2. Glue IO layer: Adapts the input or output functions to use ESP-NETIF transmit, receive, and free receive buffer.
      - Install driver_transmit to the appropriate ESP-NETIF object so that outgoing packets from the network stack are passed to the IO driver.
      - Calls `esp_netif_receive()` to pass incoming data to the network stack.

- **C. ESP-NETIF**
  - ESP-NETIF serves as an intermediary between an IO driver and a network stack, connecting the packet data path between the two. It provides a set of interfaces for attaching a driver to an ESP-NETIF object at runtime and configures a network stack during compiling. Additionally, a set of APIs is provided to control the network interface lifecycle and its TCP/IP properties.
  - As an overview, the ESP-NETIF public interface can be divided into six groups:
    - 1. Initialization APIs (to create and configure ESP-NETIF instance).
    - 2. Input or Output API (for passing data between IO driver and network stack).
    - 3. Event or Action API.
      - Used for network interface lifecycle management
      - ESP-NETIF provides building blocks for designing event handlers.
    - 4. Setters and Getters API for basic network interface properties.
    - 5. Network stack abstraction API: enabling user interaction with TCP/IP stack.
      - Set interface up or down
      - DHCP server and client API
      - DNS API
      - SNTP API
    - 6. Driver conversion utilities API

- **D. Network Stack**
  - The network stack has no public interaction with application code with regard to public interfaces and shall be fully abstracted by ESP-NETIF API.

- **E. ESP-NETIF L2 TAP Interface**
  - The ESP-NETIF L2 TAP interface is a mechanism in ESP-IDF used to access Data Link Layer (L2 per OSI/ISO) for frame reception and transmission from the user application.
  - From a user perspective, the ESP-NETIF L2 TAP interface is accessed using file descriptors of VFS, which provides file-like interfacing (using functions like open(), read(), write(), etc).

- Configuration Steps and ESP-IDF APIs Used:
  - Define WiFi Settings -> header file with SSID, Password, IP, gateway, Netmask, etc.
  - Define WiFi FreeRTOS task -> Use `xTaskCreatePinnedToCore()` or `xTaskCreate()`.
  - Create an event handler -> Call `esp_event_handler_instance_register()`.
  - Implement default configuration -> Initialize TCP/IP stack using `esp_netif_init()` and WiFi configuration by calling `esp_wifi_init()`, `esp_wifi_set_storage()`, and default configurations `esp_netif_create_default_wifi_ap()`, `esp_netif_create_default_wifi_sta()`.
  - Define ESP32 SoftAP configuration -> Define AP settings `wifi_config_t` struct and static IP (in our case) APs Used `esp_netif_set_ip_info()`, `esp_netif_dhcps_start()`, `esp_wifi_set_mode()`, `esp_wifi_set_config()`, `esp_wifi_set_bandwidth()`, `esp_wifi_set_ps()`.
  - Start WiFi `esp_wifi_start()`.

## 9. HTTP Server

### 22. HTTP Server implementation

- The HTTP Server will support the web page files (.html, .css, .js).
- It will also support OTA (Over the Air) Firmware updates.
- The HTTP server will be able to respond to Connection and Disconnection buttons on the web page e.g, by entering SSID & Password into text fields and clicking connect and disconnect for removing a connection.
- The Web server will also handle sending connection information (SSID and IP, Gateway, Netmask) about the active connection to the web page.

- Overview:
  - The HTTP Server component provides an ability for running a lightweight web server on ESP32.
  - Following are detailed steps to use the API exposed by the HTTP Server:
  - `httpd_start()`: Creates an instance of HTTP server, allocate memory/resources for it depending upon the specified configuration and outputs a handle to the server instance. The server has both, a listening socket (TCP) for HTTP traffic, and a control socket (UDP) for control signals, which are selected in a round robin fashion in the server task loop.
    - The task priority and stack size are configurable during server instance creation by passing httpd_config_t structure to `httpd_start()`.
    - TCP traffic is parsed as HTTP requests and, depending on the requested URI, user registered handlers are invoked which are supposed to send back HTTP response packets.
  - `httpd_stop()`: This stops the server with the provided handle and frees up any associated memory/resources.
    - This is a blocking function that first signals a halt to the server task and then waits for the task to terminate.
    - While stopping, the task closes all open connections, removes registered URI handlers and resets all session context data to empty.
  - `httpd_register_uri_handler()`: A URI handler is registered by passing object of type httpd_uri_t structure which has members including uri name, method type, function pointer of type `esp_err_t *handler (httpd_req_t *req)` and `user_ctx` pointer to user context data.

- Configuration steps and notable ESP-IDF Functions Used
  - Embed Binary Data (index.html, app.css and code.js) -> Embedding Binary file.
  - Create HTTP Server start and stop functions -> create a wrappers around these.
  - Create the default HTTP server configuration and adjust to our need -> Create the struct `httpd_config_t` and call `httpd_start()`.
  - Register the URI handlers.
