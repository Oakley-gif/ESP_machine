#ifndef MQTT_H
#define MQTT_H
#include <Arduino.h>

// 初始化 Wi-Fi 并连接到 MQTT 服务器
// 在 setup() 中调用
void mqtt_setup(void);

// 必须在 loop 中循环调用，保持心跳并处理订阅消息
void mqtt_loop(void);

// 通过已连接的 MQTT 客户端发布数据
// data: 要发送的字符串消息
void SendMqttData(const char* data);

// 返回当前 MQTT 是否已连接
bool getMqttConnectedSign(void);

#endif