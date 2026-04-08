// WIFI.h
#ifndef WIFISERVER_H
#define WIFISERVER_H

#include <Arduino.h>

// 声明四个外部变量（这些变量需要在你的主程序中定义）
// 例如：bool goForward = false; 等等
extern bool goForward;
extern bool goBackward;
extern bool turnLeft;
extern bool turnRight;

// 初始化 Wi-Fi 热点并启动 HTTP 服务器
// 调用后 ESP32 会创建一个AP并监听端口80
void setupWiFiAndServer();

// 在主循环中定期调用，处理到来的 HTTP 请求
void handleWebServer();
#endif 
