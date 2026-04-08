// WIFI.cpp
#include <Arduino.h>
#include <WiFi.h>
#include "wifiserver.h"
#include <WebServer.h> // ESP32 内置的轻量级 Web 服务器库

// 创建 Web 服务器对象，监听 80 端口（HTTP 默认端口）
WebServer server(80);

// SSID 和密码：ESP32 将创建一个名为 "MyRobot_AP" 的热点，密码为 "12345678"
const char *ssid = "MyRobot_AP";
const char *password = "12345678"; // 密码至少 8 位！

// 处理根路径 "/" 的请求，返回网页
void handleRoot()
{
    // 构建 HTML 页面内容（字符串形式）
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>韭韭归一</title>
    <style>
        body { font-family: Arial; text-align: center; margin-top: 50px; }
        button { width: 120px; height: 60px; font-size: 20px; margin: 10px; }
        .forward { background-color: #4CAF50; }   /* 绿色 */
        .backward { background-color: #f44336; }  /* 红色 */
        .left { background-color: #2196F3; }      /* 蓝色 */
        .right { background-color: #FF9800; }     /* 橙色 */
    </style>
</head>
<body>
    <h2>ESP32 遥控面板</h2>
    <button class="forward" onclick="sendCommand('forward')">前进</button><br>
    <button class="left" onclick="sendCommand('left')">左转</button>
    <button class="right" onclick="sendCommand('right')">右转</button><br>
    <button class="backward" onclick="sendCommand('backward')">后退</button>

    <script>
        // JavaScript 函数：点击按钮时向 ESP32 发送 GET 请求
        function sendCommand(cmd) {
            // 使用 fetch 发送请求，例如 /cmd?dir=forward
            fetch('/cmd?dir=' + cmd)
                .then(response => {
                    if (response.ok) {
                        console.log('命令已发送: ' + cmd);
                    } else {
                        alert('发送失败');
                    }
                })
                .catch(error => {
                    console.error('网络错误:', error);
                });
        }
    </script>
</body>
</html>
)rawliteral";

    // 发送 HTML 给浏览器
    server.send(200, "text/html", html);
}

// 处理 /cmd 请求（例如 /cmd?dir=forward）
void handleCommand()
{
    // 获取 URL 中的参数 dir 的值
    String command = server.arg("dir");

    // 先把所有变量设为 false（确保一次只执行一个动作）
    goForward = false;
    goBackward = false;
    turnLeft = false;
    turnRight = false;

    // 根据命令设置对应的变量为 true
    if (command == "forward")
    {
        goForward = true;
    }
    else if (command == "backward")
    {
        goBackward = true;
    }
    else if (command == "left")
    {
        turnLeft = true;
    }
    else if (command == "right")
    {
        turnRight = true;
    }

    // 可选：打印调试信息到串口
    Serial.print("收到命令: ");
    Serial.println(command);

    // 返回成功响应
    server.send(200, "text/plain", "OK");
}

// 初始化 Wi-Fi 热点和 Web 服务器
void setupWiFiAndServer()
{
    // 启动串口用于调试（可选）
    // Serial.begin(115200);

    // 设置 ESP32 为 AP 模式（热点模式）
    WiFi.mode(WIFI_AP);

    // 启动热点
    WiFi.softAP(ssid, password);

    // 打印热点 IP 地址（通常是 192.168.4.1）
    Serial.print("热点已启动，IP 地址: ");
    Serial.println(WiFi.softAPIP());

    // 设置路由：当访问根路径 "/" 时，调用 handleRoot
    server.on("/", HTTP_GET, handleRoot);

    // 设置路由：当访问 "/cmd" 时，调用 handleCommand
    server.on("/cmd", HTTP_GET, handleCommand);

    // 启动 Web 服务器
    server.begin();
    Serial.println("Web 服务器已启动");
}

// 在主循环中需要定期调用 server.handleClient() 来处理请求
// 所以建议你在 main.cpp 的 loop() 里调用：
//   server.handleClient();
// 但为了封装性，我们也可以提供一个函数：
void handleWebServer()
{
    server.handleClient(); // 处理新来的 HTTP 请求
}