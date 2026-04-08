#include "uart.h"
#include "key.h"
#include "wifiserver.h"
#include "MQTT.h"
#include <WiFi.h>
#include <Arduino.h>
#include <FreeRTOS.h>
#include <motor.h>
#include <AccelStepper.h>
#include "driver/timer.h"
#include "driver/pcnt.h"
#include <Adafruit_NeoPixel.h>
#include "HX711.h"
#include "main.h"
#define RXD 18
#define TXD 17
#define foot_y 3
#define foot_x 2
#define foot_z 5
HardwareSerial uart2(2);

Adafruit_NeoPixel strip(1, 48, NEO_GRB + NEO_KHZ800); // RGB灯

int servo = 0;
long x = 0;
long y = 0;
// 创建队列
QueueHandle_t Sensor_to_action_Queue = NULL;
QueueHandle_t command_to_action_Queue = NULL;

void setup(void)
{
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);   // 步进电机1
    Serial.begin(115200); // 电脑串口初始化
    Sensor_to_action_Queue = xQueueCreate(10, sizeof(32));
    command_to_action_Queue = xQueueCreate(10, sizeof(char *));
    if (Sensor_to_action_Queue != NULL && command_to_action_Queue != NULL)
    {
        printf("队列创建成功!\n");
    }
    else
    {
        printf("队列创建失败!\n");
    }
    uart2.begin(115200, SERIAL_8N1, RXD, TXD); // openmv串口通信
    delay(100);
    // UART_Init();
    // tft.init();  // 屏幕初始化
    // Key_Init(); // 按键初始化
    // setupWiFiAndServer(); // 启动 Wi-Fi 和网页服务器
    mqtt_setup(); // 启动 MQTT
    // pinMode(10, OUTPUT); // 初始化LED引脚为输出
    strip.begin();   // 初始化RGB灯珠
    strip.show();    // 初始化RGB为关闭状态
    pca9685_setup(); // PCA9685 初始化
    delay(100);
    // setPCA9685_Servo(0, 0);
    //  initBrushedESC(8, 2000, 1000, 2000); // 初始化通道0的有刷电调
    // setPCA9685_Motor(3, 1000); // 设置通道0的脉宽为1000微秒

    stepper_setup(); // 步进电机初始化
    motor_init();
    FreeRTOS_Setup(); // FreeRTOS初始化
    //  Init_Hx711(); // 初始化 HX711
    //  Get_Maopi();  // 获取毛皮重量（皮重）
    Serial.println("所有模块初始化完成");

    // 发送示例数据到巴法云 MQTT 服务器
    // if (getMqttConnectedSign()) {
    //   Serial.println("发送: Hello ");
    //   SendMqttData("Hello ");
    // } else {
    //   Serial.println("MQTT 未连接，无法发送消息");
    // }
}

// ====================== 主循环函数（循环展示所有效果） ======================
void loop()
{
    if (Serial.available())
    {
        char cmd = Serial.read();

        switch (cmd)
        {
        case '1':
            setPCA9685_Motor(4, 1000); // 多轴联动
            break;
        case '2':
            setPCA9685_Motor(4, 1250); // 多轴联动
            break;
        case '3':
            setPCA9685_Motor(4, 1500); // 多轴联动
            break;
        case '4':
            stepperMove(4, 500); // 风刀移动
            break;
        case '5':
            stepperMove(4, -500); // 风刀移动
            break;
        case '6':
            setPCA9685_Motor(0, 1000); // 八连杆
            break;
        case '7':
            setPCA9685_Motor(0, 1250); // 八连杆
            break;
        case '8':
            setPCA9685_Motor(0, 1500); // 八连杆
            break;
        case '9':
            stepperMove(1, 500); // 风刀移动
            break;
        case '0':
            stepperMove(1, -500); // 风刀移动
            break;
        case 'o':
            systemEnabled = !systemEnabled;
            Serial.printf("系统%s\n", systemEnabled ? "启用" : "禁用");
            break;

        case 'd':
            motorControl(1, 1, 120); // 风刀上下移动
            break;
        case 's':
            motorControl(1, 0, 0); // 风刀上下移动
            break;
        case 'a':
            motorControl(1, 0, 120); // 风刀上下移动
            break;
        case 'q':
            stepperMoveTo(foot_x, -7000);
            stepperMoveTo(foot_y, 6000);
            setPCA9685_Servo(1, 60);
            break;
        case 'w':
            stepperMoveTo(foot_z, 500);
            break;
        case 'e':
            setPCA9685_Servo(1, 30);
            break;
        case 'r':
            stepperMoveTo(foot_z, 0);
            stepperMoveTo(foot_x, -20000);
            break;
        case 't':
            setPCA9685_Servo(1, 80);
            break;

        case 'y':
            stepperMoveTo(foot_y, 0);
            stepperMoveTo(foot_x, 0);
            break;
        case 'z':
            stepperMove(foot_x, 500);
            break;
        case 'x':
            stepperMove(foot_x, -500);
            break;
        case 'c':
            stepperMove(foot_y, 500);
            break;
        case 'v':
            stepperMove(foot_y, -500);
            break;
        case 'b':
            stepperMove(foot_z, 100);
            break;
        case 'n':
            stepperMove(foot_z, -100);
            break;
        case 'm':
            setPCA9685_Servo(1, 80);
            break;
        case ',':
            setPCA9685_Servo(1, 30);
            break;
        default:
            Serial.printf("未知命令%c\n", cmd);
            break;
        }
    }
    stepperUpdate();
    delay(1);

    /*-----------------------------------PCA9685测试代码-----------------------------------*/
    // setPCA9685_Servo(0, 120); // 测试：将通道0的舵机设置到120度
    //  delay(1000); // 等待1秒
    //  setPCA9685_Servo(0, 0); // 测试：将通道0的舵机设置到0度
    //  delay(1000); // 等待1秒
    /*-----------------------------------PCA9685测试代码-----------------------------------*/
    /*-----------------------------------上下位机串口通信测试代码-----------------------------------*/
    //  UART_SendCmd("#1,s,2;");
    //  delay(10);  // 1ms延时
    //  UART_RecvAndParseCmd();
    //  Serial.printf(" 电机%d | 模式%c | 参数%d \r\n", motorID, motorMode, motorParam);
    /*-----------------------------------上下位机串口通信测试代码-----------------------------------*/

    /*-----------------------------------按键测试代码-----------------------------------*/
    // Key_Scan();  // 【核心】非阻塞按键扫描，必须放在loop最前面循环调用！
    //  // 2. 按键单次按下检测（按下一次仅打印一次，无阻塞）
    // if (Key1_Press_Once()) {
    // Serial.println("【KEY1】单次按下！(引脚15)");
    //   digitalWrite(10, !digitalRead(10)); // 翻转LED状态
    //       servo+=180;
    //       setPCA9685_Servo(0, servo); // 测试：将通道0的舵机设置到servo度
    //       delay(500); // 等待0.5秒
    //       Serial.printf("当前舵机角度: %d\n", servo);
    //  }
    // if (Key2_Press_Once()) {
    //   Serial.println("【KEY2】单次按下！(引脚11)");
    //   digitalWrite(10, !digitalRead(10)); // 翻转LED状态
    // servo-=180;
    // setPCA9685_Servo(0, servo); // 测试：将通道0的舵机设置到servo度
    // delay(500); // 等待0.5秒
    // Serial.printf("当前舵机角度: %d\n", servo);
    //  }
    // if (Key3_Press_Once()) {
    //   Serial.println("【KEY3】单次按下！(引脚21)");
    //   digitalWrite(10, !digitalRead(10)); // 翻转LED状态
    // }
    // if (Key4_Press_Once()) {
    //   Serial.println("【KEY4】单次按下！(引脚14)");
    //   digitalWrite(10, !digitalRead(10)); // 翻转LED状态
    // }
    /*-----------------------------------按键测试代码-----------------------------------*/
    /*-----------------------------------PCA9685测试代码-----------------------------------*/
    // setPCA9685_Servo(0, 120); // 测试：将通道0的舵机设置到120度
    //  delay(1000); // 等待1秒
    //  setPCA9685_Servo(0, 0); // 测试：将通道0的舵机设置到0度
    //  delay(1000); // 等待1秒
    /*-----------------------------------PCA9685测试代码-----------------------------------*/
    /*-----------------------------------网页测试代码-----------------------------------*/
    //   if(goForward||goBackward||turnLeft||turnRight)
    //   {
    //     digitalWrite(10, !digitalRead(10)); // 翻转LED状态
    //     goForward=0;
    //     goBackward=0;
    //     turnLeft=0;
    //     turnRight=0;
    //   }

    //   handleWebServer(); //网页调用
    //   delay(10);
    /*-----------------------------------网页测试代码-----------------------------------*/
    // mqtt_loop(); // MQTT 循环处理

    // 例如每隔5秒发送一次 Hello World
    // static unsigned long lastSend = 0;
    // if (millis() - lastSend > 5000) {
    //   lastSend = millis();
    //   if (getMqttConnectedSign()) {
    //     SendMqttData("Hello ");
    //   }
    // }

    // 更新步进电机状态
    // stepperUpdate();

    // delayMicroseconds(1);  // 1微秒延时
    // delay(10);
    /*-----------------------------------RGB灯珠WS2812代码-----------------------------------*/
    // strip.setPixelColor(0, strip.Color(255, 0, 0)); // 红色
    //   strip.show();
    //   delay(500);

    //   strip.setPixelColor(0, strip.Color(0, 255, 0)); // 绿色
    //   strip.show();
    //    delay(500);

    //   strip.setPixelColor(0, strip.Color(0, 0, 255)); // 蓝色
    //   strip.show();
    //    delay(500);
    /*-----------------------------------RGB灯珠WS2812代码-----------------------------------*/
    /*-----------------------------------压力传感器称重代码-----------------------------------*/
    // long weight = Get_Weight();
    // Serial.print("重量: ");
    // Serial.print(weight);
    // Serial.println(" g");
    // delay(500);
    /*-----------------------------------压力传感器称重代码-----------------------------------*/
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    // 为消息内容动态分配内存（末尾加 '\0'）
    char *msg = (char *)malloc(length + 1);
    if (msg == NULL)
    {
        Serial.println("内存分配失败，丢弃消息");
        return;
    }
    memcpy(msg, payload, length);
    msg[length] = '\0';

    // 打印接收到的原始消息（调试用）
    Serial.printf("MQTT 收到: %s\n", msg);

    // 发送指针到队列（队列需要存储指针类型）
    if (xQueueSend(command_to_action_Queue, &msg, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        Serial.printf("队列发送失败，释放内存: %s\n", msg);
        free(msg);
    }
}