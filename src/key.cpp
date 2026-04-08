#include "key.h"

// 定义4个按键全局对象，初始化引脚和默认状态
Key_t key1 = {KEY1_PIN, KEY_RELEASE, KEY_RELEASE, 0};
Key_t key2 = {KEY2_PIN, KEY_RELEASE, KEY_RELEASE, 0};
Key_t key3 = {KEY3_PIN, KEY_RELEASE, KEY_RELEASE, 0};
Key_t key4 = {KEY4_PIN, KEY_RELEASE, KEY_RELEASE, 0};

// 按键初始化：配置引脚为输入上拉模式
void Key_Init(void)
{
    pinMode(key1.pin, INPUT_PULLUP);
    pinMode(key2.pin, INPUT_PULLUP);
    pinMode(key3.pin, INPUT_PULLUP);
    pinMode(key4.pin, INPUT_PULLUP);
}

// 单个按键扫描（内部函数，供Key_Scan调用）
static void Key_Scan_Single(Key_t *key)
{
    KeyState tempState = (KeyState)digitalRead(key->pin);  // 读取当前引脚电平
    unsigned long now = millis();                          // 获取当前时间戳

    // 检测到电平变化：记录首次变化的时间
    if (tempState != key->lastState)
    {
        key->time = now;
        key->lastState = tempState;
    }

    // 电平持续稳定超过消抖时间：更新为有效稳定状态
    if (now - key->time >= KEY_DEBOUNCE_TIME)
    {
        key->curState = tempState;
    }
}

// 按键全局扫描（非阻塞，必须在loop()中循环调用！）
void Key_Scan(void)
{
    Key_Scan_Single(&key1);
    Key_Scan_Single(&key2);
    Key_Scan_Single(&key3);
    Key_Scan_Single(&key4);
}

// 按键1 单次按下触发：按下一次仅返回true一次（释放后可再次触发）
bool Key1_Press_Once(void)
{
    static KeyState preState = KEY_RELEASE;  // 静态变量：保存上一次稳定状态
    bool isPress = false;

    // 状态由释放→按下：判定为单次有效按下
    if (key1.curState == KEY_PRESS && preState == KEY_RELEASE)
    {
        isPress = true;
    }
    preState = key1.curState;  // 更新上一次状态
    return isPress;
}

// 按键2 单次按下触发（同key1逻辑）
bool Key2_Press_Once(void)
{
    static KeyState preState = KEY_RELEASE;
    bool isPress = false;
    if (key2.curState == KEY_PRESS && preState == KEY_RELEASE)
    {
        isPress = true;
    }
    preState = key2.curState;
    return isPress;
}

// 按键3 单次按下触发
bool Key3_Press_Once(void)
{
    static KeyState preState = KEY_RELEASE;
    bool isPress = false;
    if (key3.curState == KEY_PRESS && preState == KEY_RELEASE)
    {
        isPress = true;
    }
    preState = key3.curState;
    return isPress;
}

// 按键4 单次按下触发
bool Key4_Press_Once(void)
{
    static KeyState preState = KEY_RELEASE;
    bool isPress = false;
    if (key4.curState == KEY_PRESS && preState == KEY_RELEASE)
    {
        isPress = true;
    }
    preState = key4.curState;
    return isPress;
}

// 获取按键1 实时稳定状态（按下/释放）
KeyState Key1_Get_State(void)
{
    return key1.curState;
}

// 获取按键2 实时稳定状态
KeyState Key2_Get_State(void)
{
    return key2.curState;
}

// 获取按键3 实时稳定状态
KeyState Key3_Get_State(void)
{
    return key3.curState;
}

// 获取按键4 实时稳定状态
KeyState Key4_Get_State(void)
{
    return key4.curState;
}