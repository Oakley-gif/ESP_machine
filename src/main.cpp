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
#include <driver/gpio.h>
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
QueueHandle_t command_to_action_Queue = NULL;
QueueHandle_t sensor_to_action_Queue = NULL;
int a = 10;
void setup(void)
{
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);   // 步进电机1
    Serial.begin(115200); // 电脑串口初始化
    sensor_to_action_Queue = xQueueCreate(10, sizeof(8));
    command_to_action_Queue = xQueueCreate(10, sizeof(char *));
    if (sensor_to_action_Queue != NULL && command_to_action_Queue != NULL)
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
    // initBrushedESC(0, 2000, 1000, 5000); // 初始化通道0的有刷电调
    // setPCA9685_Motor(3, 1000); // 设置通道0的脉宽为1000微秒

    stepper_setup(); // 步进电机初始化
    motor_init();
    speak_init(1, 2, 47);
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

    if (Serial.available() > 0)
    {
        int rx_char = Serial.read(); // 读取一个字符

        // 根据字符执行相应动作
        switch (rx_char)
        {

        case '1':
            setPCA9685_Motor(4, 1000); // 多轴联动
            break;
        case '2':
            setPCA9685_Motor(4, 1400); // 多轴联动
            break;
        case 'q':
            setPCA9685_Motor(4, 1300); // 多轴联动
            break;
        case '3':
            motorControl(2, 0, 0); // 八连杆关闭
            break;
        case '4':
            motorControl(2, 0, 250); // 八连杆开启4
            break;
        case '5':
            setPCA9685_Servo(7, 0); // 铰接门
            break;
        case '6':
            setPCA9685_Servo(7, 90); // 铰接门6
            break;
        case 't':
            setPCA9685_Servo(7, 180); // 铰接门
            break;
        case '7':
            stepperMove(4, 6500); // 风刀向左移动
            break;

        case '8':
            stepperMove(4, -6500); // 风刀向右移动
            break;
        case '9':
            stepperMove(1, 100); // 夹具松开
            break;
        case '0':
            stepperMove(1, -100); // 夹具夹紧
            break;
        case 'a':
            motorControl(1, 0, 120); // 风刀向上移动
            break;
        case 's':
            motorControl(1, 1, 0);
            break;
        case 'd':
            motorControl(1, 1, 120); // 风刀向下移动
            break;
        case 'f':
            setPCA9685_Motor(8, 1000); // 传送带
            break;
        case 'g':
            Serial.println("  -> 执行动作6: 传送带启动");
            setPCA9685_Motor(8, 1200); // 传送带
            break;

        case 'h':
            setPCA9685_Motor(8, 1400); // 传送带
            break;
        case 'j':
            Serial.println("  -> 执行动作6: 传送带启动");
            setPCA9685_Motor(8, 1600); // 传送带
            break;

        case 'o':
            systemEnabled = 1; // 切换系统使能状态
            Serial.printf("视觉状态切换: %s\n", systemEnabled ? "已启用" : "已禁用");
            break;
        case 'p':
            systemEnabled = 0; // 切换系统使能状态
            Serial.printf("视觉状态切换: %s\n", systemEnabled ? "已启用" : "已禁用");
            break;

        default:
            Serial.printf("未知命令: %c (输入 'h' 查看帮助)\n", rx_char);
            break;
        }
    }

    stepperUpdate();
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

void speak_init(uint8_t a, uint8_t b, uint8_t c)
{
    // 先释放引脚的默认功能（如 JTAG）
    gpio_reset_pin((gpio_num_t)a);
    gpio_reset_pin((gpio_num_t)b);
    gpio_reset_pin((gpio_num_t)c);

    // 再配置为下拉输入
    pinMode(a, INPUT_PULLDOWN);
    pinMode(b, INPUT_PULLDOWN);
    pinMode(c, INPUT_PULLDOWN);
}