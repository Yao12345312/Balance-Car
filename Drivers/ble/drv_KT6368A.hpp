#pragma once

#include "board.hpp"
#include "cmsis_os2.h"
#include <stdint.h>

// 蓝牙模块驱动
class BluetoothDriver
{
public:
    BluetoothDriver(BoardUartPort port);

    // 基础初始化 (不跑波特率扫描, 不阻塞)
    bool init();

    // 自动波特率扫描 + 模块配置 (阻塞, 应在任务上下文调用)
    // 成功后模块工作在 460800 bps, m_initialized 置位
    bool autoBaudScan();

    // 发送一条 AT 指令, 等待并读取一行响应 (以 '\n' 结束或超时)
    // resp 末尾自动追加 '\0', 返回读取到的字节数 (不含 '\0')
    uint16_t sendAT(const char *cmd, char *resp, uint16_t len);

    // 读取一行 (以 '\n' 结束或超时), 透传模式也可用
    uint16_t readLine(char *buf, uint16_t buf_len, uint32_t timeout_ms);

    // 透传发送
    uint16_t write(const uint8_t *data, uint16_t len);

    // 透传接收, 阻塞读取最多 len 字节, 第一个字节最多等 timeout_ms
    uint16_t read(uint8_t *buf, uint16_t len, uint32_t timeout_ms);

    bool getStatus() const { return m_initialized; }

private:
    BoardUartPort m_port;
    volatile bool m_initialized = false;
};

extern BluetoothDriver *g_bt;

void init_drv_bluetooth();
BluetoothDriver *bluetooth_driver();
