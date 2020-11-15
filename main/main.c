/*
Pinos-WeMos             Função                  Pino-ESP-8266
        TX                      TXD                             TXD/GPIO1
        RX                      RXD                             RXD/GPIO3
        D0                      IO                              GPIO16  
        D1                      IO, SCL                 GPIO5
        D2                      IO, SDA                 GPIO4
        D3                      IO, 10k PU              GPIO0
        D4                      IO, 10k PU, LED 		GPIO2
        D5                      IO, SCK                 GPIO14
        D6                      IO, MISO                GPIO12
        D7                      IO, MOSI                GPIO13
        D0                      IO, 10k PD, SS  		GPIO15
        A0                      Inp. AN 3,3Vmax A0
*/
#include "stdio.h"
#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "dht.h"
#include "ultrasonic.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define TRUE  1
#define FALSE 0
#define DEBUG TRUE

#define LED_BUILDING    GPIO_NUM_2
#define BUTTON          GPIO_NUM_0

#define EXAMPLE_ESP_WIFI_SSID   "Piati"
#define EXAMPLE_ESP_WIFI_PASS   "core1234"

#define WIFI_CONNECTING_BIT BIT0
#define WIFI_CONNECTED_BIT  BIT1
#define WIFI_FAIL_BIT       BIT2

#define MAX_DISTANCE_CM 500
#define TRIGGER_GPIO 4
#define ECHO_GPIO 5

#define PORT CONFIG_EXAMPLE_PORT

typedef struct
{
  int16_t temperatura;
  int16_t umidade;
  int32_t distancia;
} Data_t;

static const char * TAG = "wifi station: ";
static EventGroupHandle_t s_wifi_event_group;

static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
static const gpio_num_t dht_gpio = 16;

QueueHandle_t queue;
Data_t data;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifi_init_sta(void);

void task_sinalizarConexaoWifi(void *pvParameter);
void task_reconectarWifi(void *pvParameter);
void task_CriarTCPServer(void *pvParameters);
void task_LerTemperaturaUmidade(void *pvParameter);
void task_lerDistancia(void *pvParameter);
void app_main(void);

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Conectando...");

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTING_BIT);

        esp_wifi_connect();

        xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT | WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Conexão falhou...");

        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);

        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTING_BIT | WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Conectado...");

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTING_BIT | WIFI_FAIL_BIT);

        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado!! O IP atribuido é:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void task_sinalizarConexaoWifi(void *pvParameter)
{
    bool estadoLED = 0;

    gpio_set_direction(LED_BUILDING, GPIO_MODE_OUTPUT);

    while(TRUE)
    {
        EventBits_t eventBitsWifi = xEventGroupGetBits(s_wifi_event_group);

        if(eventBitsWifi & WIFI_CONNECTING_BIT)
        {
            gpio_set_level(LED_BUILDING, estadoLED);

            estadoLED = !estadoLED;

            vTaskDelay(500 / portTICK_RATE_MS);
        }
        else if(eventBitsWifi & WIFI_FAIL_BIT)
        {
            gpio_set_level(LED_BUILDING, estadoLED);

            estadoLED = !estadoLED;

            vTaskDelay(100 / portTICK_RATE_MS);
        }
        else if(eventBitsWifi & WIFI_CONNECTED_BIT)
        {
            gpio_set_level(LED_BUILDING, estadoLED);

            estadoLED = !estadoLED;

            vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
}

void task_reconectarWifi(void *pvParameter)
{
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON, GPIO_PULLUP_ONLY);


    while(TRUE)
    {   
        xEventGroupWaitBits(s_wifi_event_group, WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        if(!gpio_get_level(BUTTON))
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTING_BIT);

            esp_wifi_connect();
        }

        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

void task_LerTemperaturaUmidade(void *pvParameters)
{
	int16_t temperatura;
	int16_t umidade;

    while (1)
    {
    	if(dht_read_data(sensor_type, dht_gpio, &umidade, &temperatura) == ESP_OK)
    	{
    		data.temperatura = temperatura / 10;
	    	data.umidade = umidade / 10;

	        xQueueOverwrite(queue, &data);
    	}

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void task_lerDistancia(void *pvParamters)
{
    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO
    };

    ultrasonic_init(&sensor);

    uint32_t distancia;

    while (1)
    {
        esp_err_t res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distancia);

        if (res != ESP_OK)
        {
            printf("Error: ");

            switch (res)
            {
                case ESP_ERR_ULTRASONIC_PING:
                    printf("Cannot ping (device is in invalid state)\n");
                    break;
                case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
                    printf("Ping timeout (no device found)\n");
                    break;
                case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
                    printf("Echo timeout (i.e. distance too big)\n");
                    break;
                default:
                    printf("%d\n", res);
            }
        }
        else
        {
        	data.distancia = distancia;

        	xQueueOverwrite(queue, &data);
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void task_CriarTCPServer(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    char temperatura[128];
    char umidade[128];
    char distancia[128];
    char menu[128];
    int addr_family;
    int ip_protocol;

    while (1) {
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        #ifdef CONFIG_EXAMPLE_IPV4
            struct sockaddr_in destAddr;
            destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            destAddr.sin_family = AF_INET;
            destAddr.sin_port = htons(PORT);
            addr_family = AF_INET;
            ip_protocol = IPPROTO_IP;
            inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
        #else
            struct sockaddr_in6 destAddr;
            bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
            destAddr.sin6_family = AF_INET6;
            destAddr.sin6_port = htons(PORT);
            addr_family = AF_INET6;
            ip_protocol = IPPROTO_IPV6;
            inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
        #endif

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);

        if (listen_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        ESP_LOGI(TAG, "Socket created");

        int err = bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));

        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            break;
        }

        ESP_LOGI(TAG, "Socket binded");

        err = listen(listen_sock, 1);

        if (err != 0) {
            ESP_LOGE(TAG, "Error occured during listen: errno %d", errno);
            break;
        }

        ESP_LOGI(TAG, "Socket listening");

        #ifdef CONFIG_EXAMPLE_IPV6
            struct sockaddr_in6 sourceAddr;
        #else
            struct sockaddr_in sourceAddr;
        #endif

        uint addrLen = sizeof(sourceAddr);
        int sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);

        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        ESP_LOGI(TAG, "Socket accepted");

        sprintf(menu, "DIGITE:\r\n          -> TEMP (Obter temperatura)\r\n          -> UMID (Obter umidade)\r\n          -> DIST (Obter distância)\r\n\n");

        send(sock, menu, strlen(menu), 0);

        while (1) {
        	int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            else if (len == 0) {
                ESP_LOGI(TAG, "Connection closed");
                break;
            }
            else {
                #ifdef CONFIG_EXAMPLE_IPV6
                    if (sourceAddr.sin6_family == PF_INET) {
                        inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                    } else if (sourceAddr.sin6_family == PF_INET6) {
                        inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                    }
                #else
                    inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                #endif

                rx_buffer[len] = 0;
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);

                if(xQueueReceive(queue, &data, pdMS_TO_TICKS(0)) == true)
                {
                	if(rx_buffer[0] == 'T' && rx_buffer[1] == 'E' && rx_buffer[2] == 'M' && rx_buffer[3] == 'P')
                	{
                		sprintf(temperatura, "Temperatura: %d°C\n", data.temperatura);
                		send(sock, temperatura, strlen(temperatura), 0);
                	}

                	if(rx_buffer[0] == 'U' && rx_buffer[1] == 'M' && rx_buffer[2] == 'I' && rx_buffer[3] == 'D')
                	{
                		sprintf(umidade, "Umidade: %d%%\n", data.umidade);
                		send(sock, umidade, strlen(umidade), 0);
                	}

                	if(rx_buffer[0] == 'D' && rx_buffer[1] == 'I' && rx_buffer[2] == 'S' && rx_buffer[3] == 'T')
                	{
                		sprintf(distancia, "Distância: %dcm\n", data.distancia);
                		send(sock, distancia, strlen(distancia), 0);
                	}
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }

    vTaskDelete(NULL);
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    wifi_init_sta();

    queue = xQueueCreate(10, sizeof(Data_t));

    xTaskCreate(task_sinalizarConexaoWifi, "task_sinalizarConexaoWifi", 2048, NULL, 5, NULL );
    xTaskCreate(task_reconectarWifi, "task_reconectarWifi", 2048, NULL, 5, NULL );

    xTaskCreate(task_CriarTCPServer, "task_CriarTCPServer", 4096, NULL, 5, NULL);

    xTaskCreate(task_LerTemperaturaUmidade, "task_LerTemperaturaUmidade", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    xTaskCreate(task_lerDistancia, "task_lerDistancia", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}