#ifndef __UART_H__
#define __UART_H__
#include <Arduino.h>
#include <driver/uart.h>   // ESP32硬件串口驱动库，串口引脚重映射必须加！！！


// ====================== 串口硬件配置【核心，固定无需修改】======================
#define UART_RX_PIN         17          // 串口接收引脚 17
#define UART_TX_PIN         18          // 串口发送引脚 18
#define UART_BAUD_RATE      115200      // 串口波特率，双机必须一致
#define UART_PORT           Serial2     // 使用ESP32的硬件串口2，绑定17/18引脚

// ====================== 自定义通信协议【和之前完全一致，不用改】======================
#define CMD_START_CHAR      '#'         // 指令起始符，标记指令开始
#define CMD_END_CHAR        ';'         // 指令结束符，标记指令结束
#define CMD_SEP_CHAR        ','         // 指令分隔符，分割参数

// ====================== 解析后的指令参数【全局可用，解析成功后赋值】======================
extern int motorID;         // 解析出的目标电机编号 (1、2、3...)
extern char motorMode;      // 解析出的运动模式 D=定距离移动  S=定速度保持
extern int motorParam;      // 解析出的参数值 距离/速度数值

// ====================== 函数声明【外部调用，不用修改】======================
/**
 * @brief 串口初始化函数
 * @note  调用一次即可，初始化硬件串口2，绑定17/18引脚，配置波特率
 */
void UART_Init(void);

/**
 * @brief 串口发送指令函数（上位机专用）
 * @param cmd 要发送的完整指令字符串，比如 "#1,D,80;"
 */
void UART_SendCmd(String cmd);


/**
 * @brief 串口接收+协议解析核心函数（下位机专用）
 * @note  放在loop()中循环调用，自动检测串口数据、筛选有效指令、解析指令参数
 * @return true=解析到一条有效指令并解析成功  false=无有效指令/解析失败
 */
bool UART_RecvAndParseCmd(void);

#endif
