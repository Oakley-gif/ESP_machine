#ifndef _KEY_H_
#define _KEY_H_

#include <Arduino.h>

// 定义4个按键引脚（15 11 21 14）
#define KEY1_PIN 15
#define KEY2_PIN 11
#define KEY3_PIN 21
#define KEY4_PIN 14

// 按键消抖时间（20ms，硬件抖动一般<10ms，可微调）
#define KEY_DEBOUNCE_TIME 20
// 按键状态枚举
typedef enum {
    KEY_RELEASE = 1,  // 释放（上拉高电平）
    KEY_PRESS   = 0   // 按下（拉低电平）
} KeyState;

// 按键结构体（非阻塞核心：每个按键独立的状态/时间戳）
typedef struct {
    uint8_t pin;          // 按键引脚
    KeyState curState;    // 当前稳定状态
    KeyState lastState;   // 上一次状态
    unsigned long time;   // 电平变化时间戳（用于消抖）
} Key_t;

// 声明4个按键全局对象（供外部调用）
// 每个对象包含引脚、当前/上一次状态及时间戳，
// 底层驱动在 key.cpp 中更新这些结构。
extern Key_t key1;
extern Key_t key2;
extern Key_t key3;
extern Key_t key4;

// 函数声明
// 初始化所有按键引脚为输入上拉，并设置初始状态
void Key_Init(void);

// 扫描所有按键电平并处理消抖状态机
// 应在主循环最前面多次调用以保证准确检测
void Key_Scan(void);

// 以下四个函数用于检测按键单次按下事件
// 如果按键从未按下则返回false，一旦检测到按下则返回true并只在下一次扫描后才会再次返回true
bool Key1_Press_Once(void);
bool Key2_Press_Once(void);
bool Key3_Press_Once(void);
bool Key4_Press_Once(void);

// 以下函数直接返回指定按键当前稳定状态
// 可用于持续判断按键是否按下或释放
KeyState Key1_Get_State(void);
KeyState Key2_Get_State(void);
KeyState Key3_Get_State(void);
KeyState Key4_Get_State(void);

#endif  // _KEY_H_