#pragma once
#include <esp_netif.h>

#define HTTP_SERVER_PORT                8000
#define HTTP_SERVER_SEND_WAIT_TIMEOUT   10
#define HTTP_SERVER_RECV_WAIT_TIMEOUT   10



/* Public types --------------------------------------------------------------*/
typedef enum {
    HTTP_MSG_WIFI_CONNECT_INIT = 0,
    HTTP_MSG_WIFI_CONNECT_SUCCESS,
    HTTP_MSG_WIFI_CONNECT_FAIL,
    HTTP_MSG_OTA_UPDATE_SUCCESSFUL,
    HTTP_MSG_OTA_UPDATE_FAILED,
    HTTP_MSG_OTA_UPDATE_INITIALIZED
} http_server_message_e;

typedef struct {
    http_server_message_e msgID;
} http_server_message_t;

/* Public function prototypes ------------------------------------------------*/

/**
 * @brief   Sends a message to the queue.
 * @param[in]   msgID   - 
 * 
 * @return  pdTRUE if an item was successfully sent to the queue, otherwise
 *          pdFALSE is returned.
 */
BaseType_t http_server_monitor_send_message(http_server_message_e msgID);

/**
 * @brief   Starts the HTTP server.
 * 
 */
void http_server_start(void);

void http_server_stop(void);
