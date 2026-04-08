#ifndef _FREERTOS_H_
#define _FREERTOS_H_

#include <Arduino.h>
extern bool systemEnabled;
// FreeRTOS 任务原型，供 xTaskCreate 使用
// pvParameters: 传递给任务的参数（通常为 NULL）
void vision_task(void *pvParameters);
void sensor_task(void *pvParameters);
void command_task(void *pvParameters);
void action_task(void *pvParameters);   

// 创建并配置所有 FreeRTOS 任务，应在 setup() 中调用
void FreeRTOS_Setup(void);


// 串口命令处理
void handleSerialCommands();
#endif