#ifndef MAIN_H
#define MAIN_H

// 声明 MQTT 回调函数
// 注意：这里只是告诉编译器“有这个函数”，并没有写函数的具体内容
void mqtt_callback(char *topic, byte *payload, unsigned int length);
void handleSerialCommands(void);

extern QueueHandle_t Sensor_to_action_Queue; 
extern QueueHandle_t command_to_action_Queue; 

// 在 main.h 中定义命令 ID 枚举
typedef enum {
    CMD_UNKNOWN = 0,
    CMD_M1_ON,
    CMD_M1_OFF,
    CMD_M2_ON,
    CMD_M2_OFF,
    CMD_M3_ON,
    CMD_M3_OFF,
    CMD_M4_ON,
    CMD_M4_OFF,
    CMD_M5_ON,
    CMD_M5_OFF,
    CMD_M6_ON,
    CMD_M6_OFF,
    CMD_ON
} CommandID_t;


#endif