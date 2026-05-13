#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <motor.h>
#include <Adafruit_NeoPixel.h>
#include "main.h"
#include "MQTT.h"
#include <driver/gpio.h>
extern Adafruit_NeoPixel strip;
#define USE_MULTCORE 1 // 是否使用多核
#define MOTOR_X 2      // X轴步进电机
#define MOTOR_Y 3      // Y轴步进电机
#define MOTOR_Z 5      // Z轴升降电机   



// 系统参数
#define PIXEL_TO_STEP_RATIO 33.0f // 像素到步数的比例系数（需要校准）
#define GRIPPER_OPEN_ANGLE 100    // 夹爪张开角度
#define GRIPPER_CLOSE_ANGLE 0     // 夹爪闭合角度
#define GRIPPER_DELAY_MS 50       // 舵机动作延时

// 预设位置（像素坐标）
#define HOME_X 0
#define HOME_Y 0
#define TRASH_X -13000 // 垃圾区X坐标
#define TRASH_Y -10000 // 垃圾区Y坐标
#define Yerr 4400      // Y轴摄像头与夹爪偏差
#define Xerr 800       // X轴摄像头与夹爪偏差     中心是偏800，现在往左偏0
// 系统状态
enum SystemState
{
  STATE_IDLE,           // 空闲等待视觉数据
  STATE_POSITIONING_XY, // XY定位到目标上方
  STATE_DESCENDING_Z,   // Z轴下降
  STATE_GRIPPING,       // 夹取物体
  STATE_ASCENDING_Z,    // Z轴上升
  STATE_MOVING_TO_Y,
  STATE_MOVING_TO_X,
  STATE_DROPPING,        // 丢弃物体
  STATE_RETURNING_HOMEX, // 返回原点
  STATE_RETURNING_HOMEY
};

// 全局变量
SystemState currentState = STATE_IDLE;
int targetX = 0, targetY = 0; // 目标位置（像素）
bool systemEnabled = true;    // 系统使能标志
String errorMessage = "";     // 错误信息

extern HardwareSerial uart2;
// 定义全局变量存储X、Y偏移距离
int offsetX = 0;
int offsetY = 0;
uint8_t sensor_state = 0; // 用于发送的状态码（1~6）
long i = 0;

bool uart_solve(const String &data, String &partx, String &party);
void sensor_task(void *xTask3)
{
  int level_a, level_b, level_c;

  while (1)
  {
    level_a = digitalRead(GPIO_NUM_1);  // PB5
    level_b = digitalRead(GPIO_NUM_2);  // PB6
    level_c = digitalRead(GPIO_NUM_47); // PA2
                                        // Serial.printf("传感器状态: A=%d, B=%d, C=%d\n", level_a, level_b, level_c);

    // 根据传感器状态设置 sensor_state

    if (level_a == LOW && level_b == LOW && level_c == HIGH)
    {
     // setPCA9685_Motor(8, 1500);
      vTaskDelay(pdMS_TO_TICKS(5000));
     // setPCA9685_Motor(8, 1000);
    }
    else if (level_a == LOW && level_b == HIGH && level_c == LOW)
    {
    //  motorControl(2, 0, 250);
      vTaskDelay(pdMS_TO_TICKS(5000));
     // motorControl(2, 0, 0);
    }
    else if (level_a == HIGH && level_b == LOW && level_c == LOW)
    {
     // setPCA9685_Motor(4, 1400); // 多轴联动
      vTaskDelay(pdMS_TO_TICKS(5000));
    //  setPCA9685_Motor(4, 1000); // 多轴联动
    }
    else
    { // 1 1 1
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void command_task(void *xTask4)
{
  while (1)
  {
    mqtt_loop(); // MQTT 循环处理

    vTaskDelay(pdMS_TO_TICKS(1)); // 100ms
  }
}

// 辅助函数：字符串转命令 ID
CommandID_t parseCommand(const char *cmdStr)
{
  if (strcmp(cmdStr, "M1_ON") == 0)
    return CMD_M1_ON;
  if (strcmp(cmdStr, "M1_OFF") == 0)
    return CMD_M1_OFF;
  if (strcmp(cmdStr, "M2_ON") == 0)
    return CMD_M2_ON;
  if (strcmp(cmdStr, "M2_OFF") == 0)
    return CMD_M2_OFF;
  if (strcmp(cmdStr, "M3_ON") == 0)
    return CMD_M3_ON;
  if (strcmp(cmdStr, "M3_OFF") == 0)
    return CMD_M3_OFF;
  if (strcmp(cmdStr, "M4_ON") == 0)
    return CMD_M4_ON;
  if (strcmp(cmdStr, "M4_OFF") == 0)
    return CMD_M4_OFF;
  if (strcmp(cmdStr, "M5_ON") == 0)
    return CMD_M5_ON;
  if (strcmp(cmdStr, "M5_OFF") == 0)
    return CMD_M5_OFF;
  if (strcmp(cmdStr, "M6_ON") == 0)
    return CMD_M6_ON;
  if (strcmp(cmdStr, "M6_OFF") == 0)
    return CMD_M6_OFF;
  if (strcmp(cmdStr, "on") == 0)
    return CMD_ON;
  return CMD_UNKNOWN;
}

// 修改后的电机控制任务
void action_task(void *xTask5)
{
  char *received_msg = NULL;
  int sensor = 0; // 用于接收传感器状态
  Serial.println("电机控制任务已启动");

  while (1)
  {
    // 接收队列中的消息指针（阻塞等待）
    if (xQueueReceive(command_to_action_Queue, &received_msg, pdMS_TO_TICKS(10)) == pdPASS)
    {
      if (received_msg == NULL)
        continue;

      Serial.printf("收到命令：%s\n", received_msg);

      // 解析格式 "M{id}_{value}"
      if (received_msg[0] == 'M')
      {
        // 查找下划线分隔符
        char *underscore = strchr(received_msg, '_');
        if (underscore != NULL)
        {
          int device_id = atoi(received_msg + 1); // 设备编号 1~6
          int value = atoi(underscore + 1);       // 数值 0~100

          // Serial.printf("解析结果: 设备=%d, 数值=%d\n", device_id, value);
          // 根据设备ID执行控制
          switch (device_id)
          {
          case 1: // 多轴联动
            if (value == 0)
            {
              Serial.println("设备1 关闭");
              setPCA9685_Motor(4, 1000); // 多轴联动
                                         // TODO: 停止多轴联动
            }
            else
            {
              Serial.printf("设备1 开启，速度 = %d\n", value);
              setPCA9685_Motor(4, 1000 + value * 10); // 多轴联动
            }
            break;

          case 2: // 八连杆工作台
            if (value == 0)
            {
              Serial.println("设备2 关闭");
            //  setPCA9685_Motor(0, 1000); // 八连杆
            motorControl(2, 0, 0);
            }
            else
            {
              Serial.printf("设备2 开启，速度 = %d\n", value);
             motorControl(2, 0, value*2.50);
            }
            break;

          case 3: // 风刀去土
            if (value == 0)
            {
              Serial.println("设备3 关闭");
              stepperMove(4, -1000); // 风刀移动
            }
            else
            {
              Serial.printf("设备3 开启，风量 = %d\n", value);
              stepperMove(4, 1000); // 风刀移动
            }
            break;

          case 4: // 视觉模块
            if (value == 0)
            {
              Serial.println("设备4 关闭");
              systemEnabled = 0; // 系统使能标志
            }
            else
            {
              Serial.printf("设备4 开启");
              systemEnabled = 1; // 系统使能标志
            }
            break;

          case 5: // 打捆机构
            if (value == 0)
            {
              Serial.println("执行：设备5 关闭");
            }
            else
            {
              Serial.printf("执行：设备5 打捆力度 = %d\n", value);
            }
            break;

          case 6: // 传送带
            if (value == 0)
            {
              Serial.println("执行：设备6 停止");
              setPCA9685_Motor(8, 1000); // 八连杆
            }
            else
            {
              Serial.printf("执行：设备6 传送速度 = %d\n", value);
              setPCA9685_Motor(8, 1000 + value * 10); // 八连杆
            }
            break;

          default:
            Serial.printf("未知设备ID: %d\n", device_id);
            break;
          }
        }
        else
        {
          Serial.printf("命令格式错误（缺少下划线）: %s\n", received_msg);
        }
      }
      else
      {
        Serial.printf("未知命令前缀: %s\n", received_msg);
      }

      // 释放动态分配的内存
      free(received_msg);
      received_msg = NULL;
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // 避免任务过度占用CPU
  }
}
// ============ 视觉抓取主任务 ============
void vision_task(void *pvParameters)
{
  String receivedString;
  String partX, partY;
  setPCA9685_Servo(1, 100);
  vTaskDelay(pdMS_TO_TICKS(200));
  systemEnabled = false;

  while (1)
  {
    // 首先更新所有步进电机状态
    stepperUpdate();

    if (systemEnabled)
    {
      // 系统状态机
      switch (currentState)
      {
      // ============ 状态1：空闲等待 ============
      case STATE_IDLE:
      {
        while (uart2.available()) uart2.read();
        vTaskDelay(pdMS_TO_TICKS(100)); // 等待数据稳定
        if (uart2.available())
        {
          receivedString = uart2.readStringUntil('\n');
          Serial.println(receivedString);
        }
        Serial.println(receivedString);
        if (uart_solve(receivedString, partX, partY))
        {
          receivedString = "";
          // 转换为整数并检查
          int tempX = partX.toInt();
          int tempY = partY.toInt();
          targetX = tempX;
          targetY = tempY;
          currentState = STATE_POSITIONING_XY;
        }
        else
        {
          Serial.println("无效视觉数据，继续等待...");
        }
        break;
      }
      // ============ 状态2：XY定位 ============
      case STATE_POSITIONING_XY:
      {
        Serial.println("状态：XY轴定位");

        // 计算需要移动的步数（相对当前位置）
        long stepsX = (long)((0 - targetX) * PIXEL_TO_STEP_RATIO + Xerr);
        long stepsY = (long)((targetY) * PIXEL_TO_STEP_RATIO + Yerr);

        Serial.println(" X=" + String(stepsX) + ", Y=" + String(stepsY));
        // 同时移动X和Y轴
        if (stepsX != 0)
          stepperMove(MOTOR_X, stepsX);
        if (stepsY != 0)
          stepperMove(MOTOR_Y, stepsY);

        while ((stepsX != 0 && isStepperMoving(MOTOR_X)) ||
               (stepsY != 0 && isStepperMoving(MOTOR_Y)))
        {
          stepperUpdate();
          // vTaskDelay(pdMS_TO_TICKS(1));
        }
        vTaskDelay(pdMS_TO_TICKS(300)); // 稍作停顿，确保稳定
        currentState = STATE_DESCENDING_Z;
        break;
      }

      // ============ 状态3：Z轴下降 ============
      case STATE_DESCENDING_Z:
      {

        stepperMoveTo(MOTOR_Z, -400);
        while (isStepperMoving(MOTOR_Z))
        {
          stepperUpdate();
          // vTaskDelay(pdMS_TO_TICKS(1));
        }
        vTaskDelay(pdMS_TO_TICKS(300)); // 稍作停顿，确保稳定
        currentState = STATE_GRIPPING;
        break;
      }

      // ============ 状态4：夹取物体 ============
      case STATE_GRIPPING:
      {
        setPCA9685_Servo(1, 30);
        vTaskDelay(pdMS_TO_TICKS(1000)); // 抓取需要更多时间
        currentState = STATE_ASCENDING_Z;
        break;
      }

      // ============ 状态5：Z轴上升 ============
      case STATE_ASCENDING_Z:
      {

        stepperMoveTo(MOTOR_Z, 0);

        while (isStepperMoving(MOTOR_Z))
        {
          stepperUpdate();
          // vTaskDelay(pdMS_TO_TICKS(1));
        }
        currentState = STATE_MOVING_TO_Y;
        break;
      }

      case STATE_MOVING_TO_Y:
      {
        stepperMoveTo(MOTOR_Y, TRASH_Y);

        // 等待移动完成
        while (isStepperMoving(MOTOR_Y))
        {
          stepperUpdate();
          // vTaskDelay(pdMS_TO_TICKS(1));
        }

        currentState = STATE_MOVING_TO_X;
        break;
      }
      case STATE_MOVING_TO_X:
      {
        stepperMoveTo(MOTOR_X, TRASH_X);
        // 等待移动完成
        while (isStepperMoving(MOTOR_X))
        {
          stepperUpdate();
          // vTaskDelay(pdMS_TO_TICKS(1));
        }
        currentState = STATE_DROPPING;
        break;
      }

      // ============ 状态7：丢弃物体 ============
      case STATE_DROPPING:
      {
        setPCA9685_Servo(1, 100);
        vTaskDelay(GRIPPER_DELAY_MS);
        // 等待物体落下
        vTaskDelay(500);
        currentState = STATE_RETURNING_HOMEX;
        break;
      }

      // ============ 状态8：返回原点 ============
      case STATE_RETURNING_HOMEX:
      {
        stepperMoveTo(MOTOR_X, HOME_X);

        while (isStepperMoving(MOTOR_X))
        {
          stepperUpdate();
          // vTaskDelay(pdMS_TO_TICKS(1));
        }

        currentState = STATE_RETURNING_HOMEY;
        break;
      }
      case STATE_RETURNING_HOMEY:
      {
        stepperMoveTo(MOTOR_Y, HOME_Y);
        while (isStepperMoving(MOTOR_Y))
        {
          stepperUpdate();
          // vTaskDelay(pdMS_TO_TICKS(1));
        }
        receivedString = "";

        Serial.println("返回原点完成");
        Serial.println("抓取流程完成，等待新目标...");
        Serial.println("========================================");
        vTaskDelay(pdMS_TO_TICKS(3000));
         while (uart2.available())
        {
          uart2.read();
        }
        Serial.println("缓冲结束，准备接收新视觉数据");
        currentState = STATE_IDLE;
        break;
      }
      }
    }
    else
    {
      stepperStopAll(); // 系统未使能，停止所有电机
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ============ FreeRTOS设置函数 ============
void FreeRTOS_Setup(void)
{
  Serial.println("初始化FreeRTOS任务...");

  TaskHandle_t vision_taskHandle;
  TaskHandle_t sensor_taskHandle;
  TaskHandle_t command_taskHandle;
  TaskHandle_t action_taskHandle;

  // 应用核 Core 1 (核心1)
  xTaskCreatePinnedToCore(sensor_task, "sensor_task", 4096, NULL, 1, &sensor_taskHandle, 1);
  xTaskCreatePinnedToCore(action_task, "action_task", 4096, NULL, 4, &action_taskHandle, 1);
  xTaskCreatePinnedToCore(command_task, "command_task", 4096, NULL, 1, &command_taskHandle, 1);
  xTaskCreatePinnedToCore(vision_task, "vision_task", 8192, NULL, 2, &vision_taskHandle, 1);

  Serial.println("所有任务创建完成");
}

bool uart_solve(const String &data, String &partx, String &party)
{
  // 检查是否为空字符串
  if (data.length() == 0)
  {
    return false;
  }
  // 检查是否检测到目标
  if (data.indexOf("No blob detected") != -1)
  {
    return false;
  }

  // 解析坐标数据
  int splitindex = data.indexOf(' '); // 坐标数据格式假设为 "x y"
  if (splitindex != -1 && splitindex < data.length() - 1)
  {
    partx = data.substring(0, splitindex);  // 获取X坐标部分
    party = data.substring(splitindex + 1); // 获取Y坐标部分
    return true;                            // 解析成功
  }
  else
  {
    return false;
  }
}