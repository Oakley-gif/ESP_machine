#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include <AccelStepper.h>
// 步进电机引脚定义
#define STEP1 4 // 电机1脉冲
#define DIR1 5  // 电机1方向
#define STEP2 8
#define DIR2 9
#define STEP3 10
#define DIR3 11
#define STEP4 12
#define DIR4 13
#define STEP5 21
#define DIR5 20
#define STEP6 14
#define DIR6 35


// 直流电机引脚定义
#define PWM1  41
#define dir1  40
#define PWM2  39
#define dir2  38
// PWM 通道和频率设置
#define PWM_CHANNEL1  1
#define PWM_CHANNEL2  2
#define PWM_FREQ     20000  // 20kHz
#define PWM_RESOLUTION_BITS 8 // 8位分辨率 (0-255)
// 创建步进电机对象

extern AccelStepper stepper1;
extern AccelStepper stepper2;
extern AccelStepper stepper3;
extern AccelStepper stepper4;
extern AccelStepper stepper5;

// ============ 步进电机控制函数 ============
// 初始化所有步进电机的速度、加速度并将位置设为0
void stepper_setup(void);



void motor_init(void);
void motorControl(uint8_t motorNum, int dir, int pwm);

// 将指定编号的步进电机移动到绝对位置
// motorNum: 1-5 对应步进电机编号
// position: 目标位置（步数）
void stepperMoveTo(uint8_t motorNum, long position);

// 让指定编号的步进电机相对当前位置移动指定步数
// motorNum: 1-5, steps: 要移动的步数（正向/负向）
void stepperMove(uint8_t motorNum, long steps);

// 停止指定编号的步进电机运动
// motorNum: 要停止的电机编号
void stepperStop(uint8_t motorNum);

// 停止所有步进电机
void stepperStopAll(void);

// 查询指定编号的步进电机是否正在运动
// 返回 true 表示正在移动
bool isStepperMoving(uint8_t motorNum);

// 必须在主循环中定期调用，以让步进电机继续驱动
void stepperUpdate(void);

// ============ PCA9685 控制的设备 (新增) ============
// 这些设备不再占用主控IO，而是占用PCA9685的通道

// 初始化 PCA9685 芯片，设置 I2C 并配置频率等
void pca9685_setup(void); // PCA9685 初始化函数

// 控制 PCA9685 上的舵机
// channel: PCA9685 的通道号 (0-15)
// angle: 目标角度 0-180 度
void setPCA9685_Servo(uint8_t channel, int angle);

// 控制 PCA9685 上的有刷电调或推杆
// channel: PCA9685 的通道号
// pulseWidth: 脉宽，范围 1000-2000 微秒
void setPCA9685_Motor(uint8_t channel, uint16_t pulseWidth);

// 有刷电调初始化函数声明
// channel: PCA9685通道号
// maxPulse: 最大油门脉宽（如2000）
// minPulse: 最小油门脉宽（如1000）
// waitTime: 每步等待时间（毫秒）
void initBrushedESC(uint8_t channel, uint16_t maxPulse = 2000, uint16_t minPulse = 1000, uint16_t waitTime = 2000);
             


#endif