#include <Arduino.h>
#include <WiFi.h> // Arduino 原生 Wi-Fi 库
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "main.h"
extern Adafruit_NeoPixel strip;

// ================= 配置区 =================
#define MQTT_SERVER "bemfa.com"
#define MQTT_PORT 9501
#define MQTT_USER "" // 巴法云通常不需要用户名
#define WIFI_SSID "Xiaomi13"
#define WIFI_PASS "11111111"
#define MQTT_KEY "6a51cf8f297040f9b8551ebee8020c1c" // 客户端ID/Key
#define MQTT_TOPIC "machine006"
#define MQTT_SEND_TOPIC "machine006/set"

// ==========================================
WiFiClient espClient;           // 创建底层 TCP 客户端
PubSubClient client(espClient); // 将 TCP 客户端注入 MQTT 客户端
bool s_is_mqtt_connected = false;




// 连接 MQTT
void mqtt_reconnect()
{
    // 循环直到连接成功
    while (!client.connected())
    {
        Serial.print("尝试连接 MQTT 服务器...");

        // clientId, username, password
        // 巴法云通常只需要 clientId (即您的密钥)
        if (client.connect(MQTT_KEY, MQTT_USER, ""))
        {
            Serial.println("连接成功");
            s_is_mqtt_connected = true;
            // 订阅主题
            client.subscribe(MQTT_TOPIC);
        }
        else
        {
            Serial.print("连接失败，状态码: ");
            Serial.print(client.state());
            Serial.println(" 2秒后重试...");
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

// ================= 对外接口实现 =================
void mqtt_setup(void)
{
    // 初始化 NeoPixel
    strip.begin();
    strip.show(); // 初始化所有像素为“关”

    // 连接 Wi-Fi
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(pdMS_TO_TICKS(500)); 
        Serial.print(".");
    }

    Serial.println("\r\nWi-Fi 连接成功");

    // 配置 MQTT 服务器
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqtt_callback);

    // 尝试连接 MQTT
    mqtt_reconnect();
}

void mqtt_loop(void)
{
    // 如果断线，自动重连
    if (!client.connected())
    {
        s_is_mqtt_connected = false;
        mqtt_reconnect();
    }
    // 保持心跳
    client.loop();
}

bool getMqttConnectedSign(void)
{
    return s_is_mqtt_connected;
}

void SendMqttData(const char *data)
{
    if (s_is_mqtt_connected)
    {
        // 参数：主题, 长度, 是否保留
        client.publish(MQTT_SEND_TOPIC, data, true);
    }
}