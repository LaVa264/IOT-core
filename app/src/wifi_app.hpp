#pragma once

#include <esp_netif.h>

/**
 * @brief   SSID is the name for a WiFi network, devices will look for all
 *          networks in range when you attempt to connect to local WiFi.
 */
#define WIFI_AP_SSID            "ESP32_AP"
#define WIFI_AP_PASSWORD        "1234567890"
#define WIFI_AP_CHANNEL         1

/**
 * @brief   Max number of stations allowed to connect in.
 */
#define WIFI_AP_MAX_CONN        5
#define WIFI_AP_SSID_HIDDEN     0                 /* Access point visibility. */

/**
 * @brief   Beacon broadcast interval is the time lag between each of the
 *          beacons sent by your router or access points, by definition, the
 *          lower the value, the smaller the time lag which means that the
 *          beacon is sent more frequently. The higher the value, the bigger
 *          the time lag which means that the beacon is sent broadcasted less
 *          frequently.
 *          The beacon is needed for your devices or clients to receive
 *          information about the particular router, in this case the beacon
 *          includes some main information such as SSID, timestamp, and various
 *          parameters.
 *          Most of the routers out of the box has the default Beacon interval
 *          function value set at 100ms.
 */
#define WIFI_AP_BEACON_INTERVAL 100
#define WIFI_AP_IP              "192.168.0.1"
#define WIFI_AP_GATEWAY         "192.168.0.1"
#define WIFI_AP_NETMASK         "255.255.255.0"
#define WIFI_AP_BANDWIDTH       WIFI_BW_HT20
#define WIFI_STA_POWER_SAVE     WIFI_PS_NONE      /* Not use power save mode. */

#define MAX_SSID_LENGTH         32                  /* IEEE standard maximum. */
#define MAX_PASSWORD_LENGTH     64                  /* IEEE standard maximum. */

#define MAX_CONNECTION_RETRIES  5            /* Retries number on disconnect. */

/**
 * @brief   Netif objects for the station and access point.
 * 
 */
extern esp_netif_t *g_esp_netif_station;
extern esp_netif_t *g_esp_netif_ap;


/* Public types --------------------------------------------------------------*/

/**
 * @brief   Message IDs for the WiFi application task.
 * @note    Expand this based on your application requirements.
 */
typedef enum {
    WIFI_APP_MESSAGE_START_HTTP_SERVER = 0,
    WIFI_APP_MESSAGE_CONNECTING_FROM_HTTP_SERVER,
    WIFI_APP_MESSAGE_STATION_CONNECTED_GOT_IP
} wifi_app_message_e;

typedef struct {
    wifi_app_message_e msgID;
} wifi_app_message_t;

/* Public function prototypes ------------------------------------------------*/

/**
 * @brief   Sends a message to the queue.
 * @param[in]   msgID   - 
 * 
 * @return  pdTRUE if an item was successfully sent to the queue, otherwise
 *          pdFALSE is returned.
 */
BaseType_t wifi_app_send_message(wifi_app_message_e msgID);

/**
 * @brief   Starts the WiFi task.
 * 
 */
void wifi_app_start(void);