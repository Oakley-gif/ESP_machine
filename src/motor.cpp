#include "motor.h"
#include <Wire.h>                    // 必须引入Arduino自带的Wire库
#include <Adafruit_PWMServoDriver.h> // 引入Adafruit库
#include <AccelStepper.h>            // 引入AccelStepper库
#include "Arduino.h"                 // 引入Arduino核心库，确保使用正确的函数和类型定义
// ============ 1. 定义对象 ============
// 创建步进电机对象
AccelStepper stepper1(AccelStepper::DRIVER, STEP1, DIR1);
AccelStepper stepper2(AccelStepper::DRIVER, STEP2, DIR2);
AccelStepper stepper3(AccelStepper::DRIVER, STEP3, DIR3);
AccelStepper stepper4(AccelStepper::DRIVER, STEP4, DIR4);
AccelStepper stepper5(AccelStepper::DRIVER, STEP5, DIR5);

// 创建 PCA9685 对象
// 地址 0x40 是默认地址，如果板子A0接了高电平，就改成 0x41
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40, Wire);
// ============ 2. 初始化函数 ============

// 步进电机初始化
void stepper_setup(void)
{
    // 初始化步进电机参数
    stepper1.setMaxSpeed(12800);       // 设置最大速度
    stepper1.setAcceleration(128000); // 设置加速度

    stepper2.setMaxSpeed(12800);
    stepper2.setAcceleration(128000);

    stepper3.setMaxSpeed(12800);
    stepper3.setAcceleration(128000);
    
    stepper4.setMaxSpeed(3200);
    stepper4.setAcceleration(32000);

    stepper5.setMaxSpeed(12800);
    stepper5.setAcceleration(128000);

    // 设置当前位置为0
    stepper1.setCurrentPosition(0);
    stepper2.setCurrentPosition(0);
    stepper3.setCurrentPosition(0);
    stepper4.setCurrentPosition(0);
    stepper5.setCurrentPosition(0);

    Serial.println("步进电机初始化完成");
}

// PCA9685 初始化
void pca9685_setup(void)
{
    // 初始化 I2C（SDA, SCL），建议在主程序setup最先调用一次Wire.begin(6, 7)
    // 如果未提前调用，这里确保I2C初始化并设置正确的引脚和速率
    Wire.begin(6, 7);
    Wire.setClock(100000); // 设置I2C速率为标准100kHz，提升兼容性
    delay(100);            // 等待 I2C 稳定
    // 初始化 PCA9685 芯片
    if (!pca.begin())
    {
        Serial.println("PCA9685 初始化失败，请检查接线！");
    }
    else
    {
        Serial.println("PCA9685 初始化成功");
    }
    pca.setOscillatorFrequency(25600000); // 推荐使用官方默认25MHz
   // 设置 PWM 频率为 50Hz (舵机和电调的标准频率)
    pca.setPWMFreq(49);
}

// 有刷电调初始化函数
// channel: PCA9685通道号
// maxPulse: 最大油门脉宽（如2000）
// minPulse: 最小油门脉宽（如1000）
// waitTime: 每步等待时间（毫秒）
void initBrushedESC(uint8_t channel, uint16_t maxPulse, uint16_t minPulse, uint16_t waitTime)
{
    pca.writeMicroseconds(channel, minPulse);
    delay(waitTime);
    Serial.println("开始有刷电调初始化：拉到最大油门...");
    pca.writeMicroseconds(channel, maxPulse);
    delay(waitTime); // 等待用户听到电调提示音
    Serial.println("拉到最小油门，确认油门范围...");
    pca.writeMicroseconds(channel, minPulse);
    delay(waitTime); // 等待电调完成初始化
    Serial.println("电调初始化完成，可以正常使用。");
}

void motor_init()
{
    // 设置引脚模式
    pinMode(PWM1, OUTPUT);
    pinMode(dir1, OUTPUT);
    pinMode(PWM2, OUTPUT);
    pinMode(dir2, OUTPUT);
    pinMode(PWM3, OUTPUT);
    pinMode(dir3, OUTPUT);
    

    // 配置 LEDC PWM (用于控制速度)
    ledcSetup(PWM_CHANNEL1, PWM_FREQ, PWM_RESOLUTION_BITS);
    ledcSetup(PWM_CHANNEL2, PWM_FREQ, PWM_RESOLUTION_BITS);
    ledcSetup(PWM_CHANNEL3, PWM_FREQ, PWM_RESOLUTION_BITS);
    ledcAttachPin(PWM1, PWM_CHANNEL1);
    ledcAttachPin(PWM2, PWM_CHANNEL2);
    ledcAttachPin(PWM3, PWM_CHANNEL3);
     ledcWrite(PWM_CHANNEL1, 0);
    ledcWrite(PWM_CHANNEL2, 0);
    ledcWrite(PWM_CHANNEL3, 0);
}

void motorControl(uint8_t motorNum, int dir, int pwm)
{
    if (motorNum == 1)
    {
        digitalWrite(dir1, dir);
        ledcWrite(PWM_CHANNEL1, pwm);
    }
    else if (motorNum == 2)
    {
        digitalWrite(dir2, dir);
        ledcWrite(PWM_CHANNEL2, pwm);
    }
     else if (motorNum == 3)
    {
        digitalWrite(dir3, dir);
        ledcWrite(PWM_CHANNEL3, pwm);
    }
    else
    {
        Serial.printf("错误：无效的电机编号 %d\n", motorNum);
    }
}

// ============ 3. 控制函数 ============

// 控制 PCA9685 上的舵机
// channel: PCA9685 的通道号 (0-15)
// angle: 角度 (0-180)
void setPCA9685_Servo(uint8_t channel, int angle)
{
    // 限制角度
    if (angle < 0)
        angle = 0;
    if (angle > 180)
        angle = 180;

    // 将角度转换为脉冲宽度 (us)
    // 标准舵机: 0度对应0.5ms(500us), 180度对应2.5ms(2500us)
    // 但很多舵机实际是 1ms-2ms，你可以根据实际情况微调 500 和 2500 这两个数
    uint16_t pulseWidth = map(angle, 0, 180, 500, 2500);

    // 调用库函数，直接写入脉冲宽度 (us)
    pca.writeMicroseconds(channel, pulseWidth);
}

// 控制 PCA9685 上的有刷电调或电动推杆
// channel: PCA9685 的通道号
// pulseWidth: 脉冲宽度，1000-2000us
void setPCA9685_Motor(uint8_t channel, uint16_t pulseWidth)
{
    // 限制范围，防止炸机
    if (pulseWidth < 1000)
        pulseWidth = 1000;
    if (pulseWidth > 2000)
        pulseWidth = 2000;

    pca.writeMicroseconds(channel, pulseWidth);
}

// 步进电机控制函数
// 移动到绝对位置
void stepperMoveTo(uint8_t motorNum, long position)
{
    switch (motorNum)
    {
    case 1:
        stepper1.moveTo(position);
        break;
    case 2:
        stepper2.moveTo(position);
        break;
    case 3:
        stepper3.moveTo(position);
        break;
    case 4:
        stepper4.moveTo(position);
        break;
    case 5:
        stepper5.moveTo(position);
        break;
    default:
        Serial.printf("错误：无效的步进电机编号 %d\n", motorNum);
    }
}

// 相对移动
void stepperMove(uint8_t motorNum, long steps)
{
    switch (motorNum)
    {
    case 1:
        stepper1.move(steps);
        break;
    case 2:
        stepper2.move(steps);
        break;
    case 3:
        stepper3.move(steps);
        break;
    case 4:
        stepper4.move(steps);
        break;
    case 5:
        stepper5.move(steps);
        break;
    default:
        Serial.printf("错误：无效的步进电机编号 %d\n", motorNum);
    }
}

// 停止指定步进电机
void stepperStop(uint8_t motorNum)
{
    switch (motorNum)
    {
    case 1:
        stepper1.stop();
        break;
    case 2:
        stepper2.stop();
        break;
    case 3:
        stepper3.stop();
        break;
    case 4:
        stepper4.stop();
        break;
    case 5:
        stepper5.stop();
        break;
    default:
        Serial.printf("错误：无效的步进电机编号 %d\n", motorNum);
    }
}

// 停止所有步进电机
void stepperStopAll(void)
{
   // stepper1.stop();
    stepper2.stop();
    stepper3.stop();
  //  stepper4.stop();
    stepper5.stop();
}

// 设置步进电机最大速度（步/秒）
void setStepperSpeed(uint8_t motorNum, float speed)
{
    if (speed < 0)
        speed = 0;

    switch (motorNum)
    {
    case 1:
        stepper1.setMaxSpeed(speed);
        break;
    case 2:
        stepper2.setMaxSpeed(speed);
        break;
    case 3:
        stepper3.setMaxSpeed(speed);
        break;
    case 4:
        stepper4.setMaxSpeed(speed);
        break;
    case 5:
        stepper5.setMaxSpeed(speed);
        break;
    default:
        Serial.printf("错误：无效的步进电机编号 %d\n", motorNum);
    }
}

// 设置步进电机加速度（步/秒²）
void setStepperAcceleration(uint8_t motorNum, float accel)
{
    if (accel < 0)
        accel = 0;

    switch (motorNum)
    {
    case 1:
        stepper1.setAcceleration(accel);
        break;
    case 2:
        stepper2.setAcceleration(accel);
        break;
    case 3:
        stepper3.setAcceleration(accel);
        break;
    case 4:
        stepper4.setAcceleration(accel);
        break;
    case 5:
        stepper5.setAcceleration(accel);
        break;
    default:
        Serial.printf("错误：无效的步进电机编号 %d\n", motorNum);
    }
}

// 检查步进电机是否在移动
bool isStepperMoving(uint8_t motorNum)
{
    switch (motorNum)
    {
    case 1:
        return stepper1.isRunning();
    case 2:
        return stepper2.isRunning();
    case 3:
        return stepper3.isRunning();
    case 4:
        return stepper4.isRunning();
    case 5:
        return stepper5.isRunning();
    default:
        Serial.printf("错误：无效的步进电机编号 %d\n", motorNum);
        return false;
    }
}

// 获取步进电机当前位置
long getStepperPosition(uint8_t motorNum)
{
    switch (motorNum)
    {
    case 1:
        return stepper1.currentPosition();
    case 2:
        return stepper2.currentPosition();
    case 3:
        return stepper3.currentPosition();
    case 4:
        return stepper4.currentPosition();
    case 5:
        return stepper5.currentPosition();
    default:
        Serial.printf("错误：无效的步进电机编号 %d\n", motorNum);
        return 0;
    }
}

// 获取步进电机剩余步数
long getStepperDistanceToGo(uint8_t motorNum)
{
    switch (motorNum)
    {
    case 1:
        return stepper1.distanceToGo();
    case 2:
        return stepper2.distanceToGo();
    case 3:
        return stepper3.distanceToGo();
    case 4:
        return stepper4.distanceToGo();
    case 5:
        return stepper5.distanceToGo();
    default:
        Serial.printf("错误：无效的步进电机编号 %d\n", motorNum);
        return 0;
    }
}

// 步进电机回零（需要外部限位开关）
void stepperHome(uint8_t motorNum)
{
    // 简单回零：移动到碰到限位开关为止
    // 实际使用时需要连接限位开关并修改此函数
    Serial.printf("电机%d回零开始\n", motorNum);

    // 设置低速回零
    setStepperSpeed(motorNum, 200);
    setStepperAcceleration(motorNum, 200);

    // 向负方向移动直到限位（这里只是示例）
    stepperMove(motorNum, -10000);

    // 实际应该在这里检查限位开关状态
    //   while(digitalRead(LIMIT_SWITCH_PIN) == HIGH) {
    //      stepperUpdate();
    // }

    // 然后设置当前位置为0
    switch (motorNum)
    {
    case 1:
        stepper1.setCurrentPosition(0);
        break;
    case 2:
        stepper2.setCurrentPosition(0);
        break;
    case 3:
        stepper3.setCurrentPosition(0);
        break;
    case 4:
        stepper4.setCurrentPosition(0);
        break;
    case 5:
        stepper5.setCurrentPosition(0);
        break;
    }

    Serial.printf("电机%d回零完成\n", motorNum);
}

// 切割移动：根据视觉坐标移动X和Y轴
void stepperCuttingMove(float x, float y)
{
    // 假设每个像素对应1步
    long xSteps = (long)(x * 10); // 比例系数，根据实际情况调整
    long ySteps = (long)(y * 10);

    Serial.printf("切割移动到: X=%.1f(mm), Y=%.1f(mm)\n", x, y);
    Serial.printf("对应步数: X=%ld步, Y=%ld步\n", xSteps, ySteps);

    // 同时移动X和Y轴（电机2和电机3）
    stepperMoveTo(2, xSteps); // 电机2是X轴
    stepperMoveTo(3, ySteps); // 电机3是Y轴

    // 等待移动完成
    while (isStepperMoving(2) || isStepperMoving(3))
    {
        stepperUpdate();
        delay(1);
    }

    Serial.println("切割移动完成");
}

// 送料：送料电机前进指定长度
void stepperFeedMaterial(int length_mm)
{
    // 假设丝杆导程5mm，电机200步/转
    // 1mm = 200/5 = 40步
    long steps = length_mm * 40;

    Serial.printf("送料 %dmm，对应 %ld 步\n", length_mm, steps);

    // 电机1是送料电机
    stepperMove(1, steps);
}

// 传送带移动
void stepperConveyorMove(int length_mm)
{
    // 假设传送带每mm对应步数
    long steps = length_mm * 50; // 根据实际调整

    Serial.printf("传送带移动 %dmm，对应 %ld 步\n", length_mm, steps);

    // 电机5是传送带电机
    stepperMove(5, steps);
}

// 更新所有步进电机状态（必须在主循环中定期调用）
void stepperUpdate(void)
{
    stepper1.run();
    stepper2.run();
    stepper3.run();
    stepper4.run();
    stepper5.run();
}
