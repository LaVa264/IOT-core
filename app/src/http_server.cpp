#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <esp_http_server.h>
#include <esp_err.h>
#include <esp_log.h>

#include "config.hpp"
#include "http_server.hpp"

/* Private variables ---------------------------------------------------------*/

/**
 * @brief   Tag used for ESP serial console messages.
 */
static const char TAG[] = "http_server";
static httpd_handle_t s_http_server_handler = NULL;

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
}

/* Private function definition -----------------------------------------------*/
static httpd_handle_t http_server_configure(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* 1. Create HTTP server monitor task. */

    /* 2. Create message queue. */

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

        httpd_register_uri_handler(s_http_server_handler, &jquery_js);
        httpd_register_uri_handler(s_http_server_handler, &index_html);
        httpd_register_uri_handler(s_http_server_handler, &app_css);
        httpd_register_uri_handler(s_http_server_handler, &app_js);
        httpd_register_uri_handler(s_http_server_handler, &favicon_ico);

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
