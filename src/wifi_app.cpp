#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <lwip/netdb.h>

#include "config.hpp"
#include "wifi_app.hpp"
#include "http_server.hpp"


/* Private variables ---------------------------------------------------------*/

/**
 * @brief   Tag used for ESP serial console messages.
 */
static const char TAG[] = "wifi_app";

static QueueHandle_t s_wifi_app_event_queue = NULL;
esp_netif_t *g_esp_netif_station = NULL;
esp_netif_t *g_esp_netif_ap = NULL;

/* Private function prototype ------------------------------------------------*/

/**
 * @brief   WiFi Application main task.
 * 
 */
static void wifi_app_task(void *param);

/**
 * @brief   Initialize the WiFi Application event handler for WiFi and IP
 *          events.
 * 
 */
static void wifi_app_event_handler_init(void);

/**
 * @brief   WiFi application event handler.
 * 
 * @param event_handler_arg     - Argument data, aside from event data, that is
 *                                passed to the handler when its called.
 * @param event_base            - Base ID of the event.
 * @param event_id              - ID of the event.
 * @param event_data            - Event data
 */
static void wifi_app_event_handler(void *event_handler_arg,
                                    esp_event_base_t event_base,
                                    int32_t event_id,
                                    void *event_data);

/**
 * @brief   Initialize the TCP/IP stack and default WiFi configuration.
 * 
 */
static void wifi_app_default_config_init(void);

/**
 * @brief   Configure the WiFi Access Point settings and assigns the static IP
 *          to the SoftAP.
 */
static void wifi_app_soft_ap_config(void);

/* Public function definition ------------------------------------------------*/
BaseType_t wifi_app_send_message(wifi_app_message_e msgID)
{
    wifi_app_message_t msg;
    msg.msgID = msgID;
    return xQueueSend(s_wifi_app_event_queue, &msg, portMAX_DELAY);
}

void wifi_app_start(void)
{
    ESP_LOGI(TAG, "Starting WiFi Application.");

    /* 1. Configure logging messages. */
    esp_log_level_set("*", ESP_LOG_DEBUG);
    esp_log_level_set("wifi", ESP_LOG_DEBUG);
    esp_log_level_set("dhcpc", ESP_LOG_DEBUG);

    /* 2. Create message queue. */
    s_wifi_app_event_queue = xQueueCreate(WIFI_APP_MAX_QUEUE_HANDLE,
                                            sizeof(wifi_app_message_t));

    /* 3. Start the WiFi application. */
    xTaskCreatePinnedToCore(&wifi_app_task,
                            "wifi_app_task",
                            WIFI_APP_TASK_STACK_SIZE,
                            NULL,
                            WIFI_APP_TASK_PRIORITY,
                            NULL,
                            WIFI_APP_TASK_CORE_ID);
}

/* Private function definition -----------------------------------------------*/
static void wifi_app_task(void *param)
{
    wifi_app_message_t msg;
    /* 1. Initialize the event handler. */
    wifi_app_event_handler_init();

    /* 2. Initialize TCP/IP stack and WiFi configuration. */
    wifi_app_default_config_init();

    /* 3. SoftAP configuration. */
    wifi_app_soft_ap_config();

    /* 4. Start WiFi. */
    ESP_ERROR_CHECK(esp_wifi_start());

    /* 5. Send start http server message. */
    wifi_app_send_message(WIFI_APP_MESSAGE_START_HTTP_SERVER);
    while(1) {
        if (xQueueReceive(s_wifi_app_event_queue, &msg, portMAX_DELAY) 
            != pdTRUE) {
                continue;
            }

        switch (msg.msgID)
        {
            case WIFI_APP_MESSAGE_START_HTTP_SERVER: {
                ESP_LOGI(TAG, "WIFI_APP_MESSAGE_START_HTTP_SERVER");
                http_server_start();
            }
            break;
            case WIFI_APP_MESSAGE_CONNECTING_FROM_HTTP_SERVER: {
                ESP_LOGI(TAG, "WIFI_APP_MESSAGE_CONNECTING_FROM_HTTP_SERVER");
            }
            break;
            case WIFI_APP_MESSAGE_STATION_CONNECTED_GOT_IP: {
                ESP_LOGI(TAG, "WIFI_APP_MESSAGE_STATION_CONNECTED_GOT_IP");
            }
            break;
            default:
                break;
        }

    }
}

static void wifi_app_event_handler_init(void)
{
    /* 1. Event loop for the WiFi Driver. */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 2. Register event handler. */
    esp_event_handler_instance_t instance_wifi_event;
    esp_event_handler_instance_t instance_ip_event;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_app_event_handler,
                    NULL,
                    &instance_wifi_event));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_app_event_handler,
                    NULL,
                    &instance_ip_event));
}

static void wifi_app_event_handler(void *event_handler_arg,
                                    esp_event_base_t event_base,
                                    int32_t event_id,
                                    void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id)
        {
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
                break;
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_STOP");
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                /* Every time a station is connected to ESP32 AP, the
                WIFI_EVENT_AP_STACONNECTED will arise. */
                ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
                break;
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
                break;
            case WIFI_EVENT_STA_STOP:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_STOP");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                /* This event arises when the DHCP client successfully gets the
                 * IPV4 address from the DHCP server, or when the IPV4 address
                 * is changed. The event means that everything is ready and the
                 * application can begin its tasks (e.g., creating sockets). */
                ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
                break;

            default:
                break;
        }
    }
}

static void wifi_app_default_config_init(void)
{
    /* 1. Initialize TCP/IP stack. */
    ESP_ERROR_CHECK(esp_netif_init());

    /* 2. Default WiFi configuration. */
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    g_esp_netif_station = esp_netif_create_default_wifi_sta();
    g_esp_netif_ap = esp_netif_create_default_wifi_ap();
}

static void wifi_app_soft_ap_config(void)
{
    /* 1. Soft AP - WiFi Access point configuration. */
    wifi_config_t ap_config = {
        .ap = {
            {.ssid = WIFI_AP_SSID},
            {.password = WIFI_AP_PASSWORD},
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = WIFI_AP_CHANNEL,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden = WIFI_AP_SSID_HIDDEN,
            .max_connection = WIFI_AP_MAX_CONN,
            .beacon_interval = WIFI_AP_BEACON_INTERVAL
        }
    };

    /* 2. Configure DHCP for the Access Point. */
    esp_netif_ip_info_t ap_ip_info;
    memset(&ap_ip_info, 0, sizeof(ap_ip_info));

    /* Must stop DHCP server first. */
    esp_netif_dhcps_stop(g_esp_netif_ap);
    inet_pton(AF_INET, WIFI_AP_IP, &ap_ip_info.ip);
    inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);
    inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);

    /* Configure the network interface. */
    ESP_ERROR_CHECK(esp_netif_set_ip_info(g_esp_netif_ap, &ap_ip_info));

    /* Start the AP DHCP server. */
    ESP_ERROR_CHECK(esp_netif_dhcps_start(g_esp_netif_ap));

    /* Set mode Access point and station mode. */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

    /* Default bandwidth is 20MHz. */
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_AP_BANDWIDTH));

    /* Set power save mode. */
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE));
}
