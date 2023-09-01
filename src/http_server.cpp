#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <esp_http_server.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <mesh_util.h>

#include "config.hpp"
#include "http_server.hpp"

/* Private variables ---------------------------------------------------------*/

/**
 * @brief   Tag used for ESP serial console messages.
 */
static const char TAG[] = "http_server";
static httpd_handle_t s_http_server_handler = NULL;
static TaskHandle_t s_http_server_monitor = NULL;
static QueueHandle_t s_http_server_event_queue = NULL;
static int g_fw_update_state = OTA_UPDATE_PENDING_STATE;

const esp_timer_create_args_t g_fw_update_reset_args = {
    .callback = &http_server_fw_update_reset_callback,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "fw_update_reset"
};

static esp_timer_handle_t s_fw_update_reset;

/**
 * @brief   Embedded files.
 */
extern const uint8_t
jquery_3_3_1_min_js_start[] asm("_binary_resources_jquery_3_3_1_min_js_start");

extern const uint8_t
jquery_3_3_1_min_js_end[] asm("_binary_resources_jquery_3_3_1_min_js_end");

extern const uint8_t
index_html_start[] asm("_binary_resources_index_html_start");

extern const uint8_t
index_html_end[] asm("_binary_resources_index_html_end");

extern const uint8_t
app_css_start[] asm("_binary_resources_app_css_start");

extern const uint8_t
app_css_end[] asm("_binary_resources_app_css_end");

extern const uint8_t
app_js_start[] asm("_binary_resources_app_js_start");

extern const uint8_t
app_js_end[] asm("_binary_resources_app_js_end");

extern const uint8_t
favicon_ico_start[] asm("_binary_resources_favicon_ico_start");

extern const uint8_t
favicon_ico_end[] asm("_binary_resources_favicon_ico_end");

/* Private function prototype ------------------------------------------------*/

/**
 * @brief   Setup the default httpd server configuration.
 * 
 * @return  httpd_handle_t - HTTP server instance handle if successful, NULL if
 *          if failure.
 */
static httpd_handle_t http_server_configure(void);

/**
 * @brief   Jquery get handler requested when accessing the webpage.
 * 
 * @param req - HTTP request.
 * @return esp_err_t - ESP_OK.
 */
static esp_err_t http_server_jquery_handler(httpd_req_t *req);
static esp_err_t http_server_index_html_handler(httpd_req_t *req);
static esp_err_t http_server_app_css_handler(httpd_req_t *req);
static esp_err_t http_server_app_js_handler(httpd_req_t *req);
static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req);

/**
 * @brief   Receive binary file and handle firmware update.
 * 
 * @param req - HTTP request.
 * @return esp_err_t - ESP_OK, otherwise ESP_FAIL if timeout occurs and the
 *                     update can not be started.
 */
static esp_err_t http_server_ota_update_handler(httpd_req_t *req);

/**
 * @brief   OTA status handler responds with the firmware update status after
 *          the OTA update is started and responds with the compile time/date
 *          when the page is first requested.
 * 
 * @param req 
 * @return esp_err_t 
 */
static esp_err_t http_server_ota_status_handler(httpd_req_t *req);

/**
 * @brief   HTTP server monitor task used to track events of the HTTP server.
 * 
 * @param param 
 */
static void http_server_monitor(void *param);

/**
 * @brief   Checks fw update status and creates the update timer if ready.
 */
static void http_server_fw_update_reset_timer(void);

/* Public function definition ------------------------------------------------*/

void http_server_start(void)
{
    if (s_http_server_handler == NULL) {
        s_http_server_handler = http_server_configure();
    }
}

void http_server_stop(void)
{
    if (s_http_server_handler != NULL) {
        s_http_server_handler = NULL;
        httpd_stop(s_http_server_handler);

        ESP_LOGI(TAG, "HTTP server stopped.");
    }

    if (s_http_server_monitor) {
        vTaskDelete(s_http_server_monitor);
        s_http_server_monitor = NULL;
    }
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
    http_server_message_t msg;
    msg.msgID = msgID;
    return xQueueSend(s_http_server_event_queue, &msg, portMAX_DELAY);
}

void http_server_fw_update_reset_callback(void *param)
{
    ESP_LOGI(TAG, "Timer timed-out, resetting the device.");
    esp_restart();
}

/* Private function definition -----------------------------------------------*/
static httpd_handle_t http_server_configure(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* 1. Create HTTP server monitor task. */
    xTaskCreatePinnedToCore(&http_server_monitor,
                            "http_server_monitor",
                            HTTP_SERVER_MONITOR_STACK_SIZE,
                            NULL,
                            HTTP_SERVER_MONITOR_PRIORITY,
                            &s_http_server_monitor,
                            HTTP_SERVER_MONITOR_CORE_ID);

    /* 2. Create message queue. */
    s_http_server_event_queue = xQueueCreate(
                                HTTP_SERVER_MONITOR_MAX_QUEUE_HANDLE,
                                sizeof(http_server_message_t));

    /* 3. The core that the HTTP server will run on. */
    config.core_id = HTTP_SERVER_MONITOR_CORE_ID;

    /* 4. Configure default priority to 1 less than the WiFi task. */
    config.task_priority = HTTP_SERVER_MONITOR_PRIORITY;

    config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;
    config.max_uri_handlers = 20;
    config.recv_wait_timeout = HTTP_SERVER_RECV_WAIT_TIMEOUT;
    config.send_wait_timeout = HTTP_SERVER_SEND_WAIT_TIMEOUT;
    config.server_port = HTTP_SERVER_PORT;

    ESP_LOGI(TAG, "Configured the HTTP server.");
    ESP_LOGI(TAG, "Starting HTTP server on port: %d", config.server_port);

    if (httpd_start(&s_http_server_handler, &config) == ESP_OK) {
        /* Register URI handler. */

        httpd_uri_t jquery_js = {
            .uri = "/jquery-3.3.1.min.js",
            .method = HTTP_GET,
            .handler = http_server_jquery_handler,
            .user_ctx = NULL
        };

        httpd_uri_t index_html = {
            .uri = "/index.html",
            .method = HTTP_GET,
            .handler = http_server_index_html_handler,
            .user_ctx = NULL
        };

        httpd_uri_t app_css = {
            .uri = "/app.css",
            .method = HTTP_GET,
            .handler = http_server_app_css_handler,
            .user_ctx = NULL
        };

        httpd_uri_t app_js = {
            .uri = "/app.js",
            .method = HTTP_GET,
            .handler = http_server_app_js_handler,
            .user_ctx = NULL
        };

        httpd_uri_t favicon_ico = {
            .uri = "/favicon.ico",
            .method = HTTP_GET,
            .handler = http_server_favicon_ico_handler,
            .user_ctx = NULL
        };

        httpd_uri_t ota_update = {
            .uri = "/OTAupdate",
            .method = HTTP_POST,
            .handler = http_server_ota_update_handler,
            .user_ctx = NULL
        };

        httpd_uri_t ota_status = {
            .uri = "/OTAstatus",
            .method = HTTP_GET,
            .handler = http_server_ota_status_handler,
            .user_ctx = NULL
        };

        httpd_register_uri_handler(s_http_server_handler, &jquery_js);
        httpd_register_uri_handler(s_http_server_handler, &index_html);
        httpd_register_uri_handler(s_http_server_handler, &app_css);
        httpd_register_uri_handler(s_http_server_handler, &app_js);
        httpd_register_uri_handler(s_http_server_handler, &favicon_ico);
        httpd_register_uri_handler(s_http_server_handler, &ota_update);
        httpd_register_uri_handler(s_http_server_handler, &ota_status);

        return s_http_server_handler;
    }

    return NULL;
}

static esp_err_t http_server_jquery_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Jquery is requested.");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req,
                    (const char *)jquery_3_3_1_min_js_start,
                    jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);
    return ESP_OK;
}

static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "index.html is requested.");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req,
                    (const char *)index_html_start,
                    index_html_end - index_html_start);
    return ESP_OK;
}

static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "app.css is requested.");
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req,
                    (const char *)app_css_start,
                    app_css_end - app_css_start);
    return ESP_OK;
}

static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "app.js is requested.");
    httpd_resp_set_type(req, "text/js");
    httpd_resp_send(req,
                    (const char *)app_js_start,
                    app_js_end - app_js_start);
    return ESP_OK;
}

static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "favicon.ico is requested.");
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req,
                    (const char *)favicon_ico_start,
                    favicon_ico_end - favicon_ico_start);
    return ESP_OK;
}

static esp_err_t http_server_ota_update_handler(httpd_req_t *req)
{
    esp_ota_handle_t ota_handler;
    char ota_buffer[1024];
    int content_length = req->content_len;
    int received_content = 0;
    int recv_len = 0;
    bool is_req_body_started = false;
    bool flash_successful = false;

    /* 1. Find an OTA app partition which can be passed to esp_ota_begin().
     * Finds next partition round-robin, starting from the current running
     * partition. */
    const esp_partition_t *update_partition =
        esp_ota_get_next_update_partition(NULL);

    do {
        /* 1. Read data from request. */
        recv_len = httpd_req_recv(req,
                                    ota_buffer,
                                    MIN(content_length, 1024));
        if (recv_len < 0) {
            if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
                ESP_LOGE(TAG, "Socket timeout.");

                /* Retry receiving if timeout occurred. */
                continue;
            }

            ESP_LOGE(TAG, "Error when OTA updating.");
            return ESP_FAIL;
        }
        printf("OTA receive: %d of %d\r", received_content, content_length);

        /* If it is first data we are receiving, it will have the information in
         * the header that we need. */
        if (!is_req_body_started) {
            is_req_body_started = true;
            /* Get the location of the binary file content. */
            char *body_start_p = strstr(ota_buffer, "\r\n\r\n") + 4;

            int body_part_len = recv_len - (body_start_p - ota_buffer);
            printf("OTA filesize: %d\r\n", content_length);

            esp_err_t err = esp_ota_begin(update_partition,
                                            OTA_SIZE_UNKNOWN,
                                            &ota_handler);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error with OTA begin, cancelling OTA.");
                return ESP_FAIL;
            } else {
                ESP_LOGI(TAG, "Writing to partition subtype %d at: 0x%x",
                            update_partition->subtype,
                            update_partition->address);
            }

            /* Write the first part of data. */
            esp_ota_write(ota_handler, body_start_p, body_part_len);
            received_content += body_part_len;

        } else {
            /* Write the rest of data. */
            esp_ota_write(ota_handler, ota_buffer, recv_len);
            received_content += recv_len;
        }

    } while (recv_len > 0 && received_content < content_length);

    /* Update the partition. */
    if (esp_ota_end(ota_handler) == ESP_OK) {
        if (esp_ota_set_boot_partition(update_partition) == ESP_OK) {
            const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
            ESP_LOGI(TAG, "OTA next boot subtype: %d at: 0x%x",
                            boot_partition->subtype,
                            boot_partition->address);

            flash_successful = true;
        } else {
            ESP_LOGE(TAG, "Flashed error.");
        }
    } else {
        ESP_LOGE(TAG, "Flashed error.");
    }

    if (flash_successful == true) {
        http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESSFUL);
    } else {
        http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAILED);
    }

    return ESP_OK;
}

static esp_err_t http_server_ota_status_handler(httpd_req_t *req)
{
    char otaJSON[100];
    ESP_LOGI(TAG, "OTAstatus is requested.");

    sprintf(otaJSON,
            "{\"ota_update_status\": %d, "
            "\"compile_time\": \"%s\", \"compile_date\": \"%s\"}",
            g_fw_update_state,
            __TIME__,
            __DATE__);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, otaJSON, strlen(otaJSON));

    return ESP_OK;
}

static void http_server_monitor(void *param)
{

    http_server_message_t msg;

    while(1) {
        
        if (xQueueReceive(s_http_server_event_queue, &msg, portMAX_DELAY) 
            != pdTRUE) {
                continue;
            }

        switch (msg.msgID)
        {
            case HTTP_MSG_WIFI_CONNECT_INIT: {

            }
            break;
            case HTTP_MSG_WIFI_CONNECT_SUCCESS: {

            }
            break;
            case HTTP_MSG_WIFI_CONNECT_FAIL: {

            }
            break;
            case HTTP_MSG_OTA_UPDATE_SUCCESSFUL: {
                g_fw_update_state = OTA_UPDATE_SUCCESSFUL_STATE;
                http_server_fw_update_reset_timer();
            }
            break;
            case HTTP_MSG_OTA_UPDATE_FAILED: {
                g_fw_update_state = OTA_UPDATE_FAILED_STATE;
            }
            break;
            default:
            break;
        }
    }
}

static void http_server_fw_update_reset_timer(void)
{
    if (g_fw_update_state == OTA_UPDATE_SUCCESSFUL_STATE) {
        ESP_LOGI(TAG, "Updated firmware successful, starting Firmware update "
                    "reset timer.");

        /* Give the webpage a chance to receive an acknowledge back and
         * initialize the timer. */
        ESP_ERROR_CHECK(esp_timer_create(&g_fw_update_reset_args,
                        &s_fw_update_reset));

        ESP_ERROR_CHECK(esp_timer_start_once(s_fw_update_reset,
                        8000000));
    } else {
        ESP_LOGI(TAG, "Updated firmware unsuccessful.");
    }
}
