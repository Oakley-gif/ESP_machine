#include "uart.h"
// 通信协议：#电机编号,运动模式,数值;

// ====================== 全局变量定义【和uart.h中声明对应，内部使用】======================
int motorID = 0;               // 解析出的电机编号
char motorMode = 0;            // 解析出的运动模式 D/S
int motorParam = 0;            // 解析出的距离/速度参数
static String recvBuffer = ""; // 串口接收缓冲区，静态变量，仅本文件可用
static bool isRecvingCmd = false; // 接收指令标记，仅本文件可用
void UART_Init(void)
{
    UART_PORT.begin(UART_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN); // 串口二初始化
    // 清空缓冲区，防止上电有脏数据
    recvBuffer = "";
    isRecvingCmd = false;
}

/**
 * @brief 串口发送指令函数 实现
 */
void UART_SendCmd(String cmd)
{
  if(cmd.length() > 0)
  {
    UART_PORT.println(cmd);  // 发送指令到串口
  }
}

/**
 * @brief 私有函数：指令解析函数，仅本文件调用，不用外部调用
 * @param cmd 过滤后的纯指令内容（不含#和;）
 * @return true=解析成功  false=解析失败
 */
static bool parseCmd(String cmd)
{
  // 查找第一个分隔符，分割电机编号
  int firstSep = cmd.indexOf(CMD_SEP_CHAR);
  if(firstSep == -1) return false;
  motorID = cmd.substring(0, firstSep).toInt();

  // 查找第二个分隔符，分割运动模式
  int secondSep = cmd.indexOf(CMD_SEP_CHAR, firstSep+1);
  if(secondSep == -1) return false;
  motorMode = cmd.substring(firstSep+1, secondSep).charAt(0);

  // 分割参数数值
  motorParam = cmd.substring(secondSep+1).toInt();

  // 合法性校验：电机编号不能为0，模式只能是D/S
  if(motorID <= 0 || (motorMode != 'D' && motorMode != 'S'))
  {
    return false;
  }
  return true;
}

/**
 * @brief 串口接收+协议解析核心函数 实现
 */
bool UART_RecvAndParseCmd(void)
{
  // 检测串口是否有数据可读
  if(UART_PORT.available() <= 0)
  {
    return false;
  }

  // 循环读取所有串口数据
  while(UART_PORT.available() > 0)
  {
    char recvChar = UART_PORT.read();

    // 1. 收到起始符，开始接收有效指令，清空缓冲区
    if(recvChar == CMD_START_CHAR)
    {
      recvBuffer = "";
      isRecvingCmd = true;
      continue; //收到起始信号直接跳过本次循环进入下一次循环
    }

    // 2. 收到结束符，结束接收，开始解析指令
    if(recvChar == CMD_END_CHAR && isRecvingCmd == true)
    {
      isRecvingCmd = false;
      // 解析指令，解析成功返回true，失败返回false
      if(parseCmd(recvBuffer))
      {
        recvBuffer = "";
        return true;
      }
      recvBuffer = "";
      return false;
    }

    // 3. 正在接收指令，存入缓冲区，同时做缓冲区溢出保护
    if(isRecvingCmd == true)
    {
      if(recvBuffer.length() < 50)  // 限制最大指令长度50字符，防止内存溢出
      {
        recvBuffer += recvChar;
      }
      else
      {
        recvBuffer = "";  // 溢出则清空，重新接收
        isRecvingCmd = false;
      }
    }
  }
  return false;
}