#include "drv_KT6368A.hpp"
#include <string.h>

BluetoothDriver *g_bt = nullptr;

BluetoothDriver::BluetoothDriver(BoardUartPort port)
    : m_port(port)
{
    g_bt = this;
}

bool BluetoothDriver::init()
{
    if (m_port == BOARD_UART_NONE)
        return false;
    return true;
}

uint16_t BluetoothDriver::write(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0)
        return 0;
    BoardResult r = board_uart_write(m_port, data, len, 0.1, 0.1);
    return (r == BOARD_OK) ? len : 0;
}

uint16_t BluetoothDriver::read(uint8_t *buf, uint16_t len, uint32_t timeout_ms)
{
    if (!buf || len == 0)
        return 0;
    double rc = (double)timeout_ms * 0.001;
    return board_uart_read(m_port, buf, len, rc, rc);
}

uint16_t BluetoothDriver::readLine(char *buf, uint16_t buf_len, uint32_t timeout_ms)
{
    if (!buf || buf_len == 0)
        return 0;

    uint32_t start = osKernelGetTickCount();
    uint16_t idx = 0;

    while ((osKernelGetTickCount() - start) < timeout_ms)
    {
        uint8_t ch;
        if (board_uart_read(m_port, &ch, 1, 0.01, 0.01) == 1)
        {
            if (idx < buf_len - 1)
                buf[idx++] = (char)ch;
            if (ch == '\n')
                break;
        }
    }

    buf[idx] = '\0';
    return idx;
}

uint16_t BluetoothDriver::sendAT(const char *cmd, char *resp, uint16_t len)
{
    if (!cmd || !resp || len == 0)
        return 0;

    // 清空接收缓冲, 丢弃上一条残留响应
    board_uart_reset_rx(m_port);

    // 发送 AT 指令
    size_t cmdlen = strlen(cmd);
    if (board_uart_write(m_port, (const uint8_t *)cmd, cmdlen, 0.1, 0.1) != BOARD_OK)
        return 0;

    // 读取一行响应 (最多等 500ms)
    uint32_t start = osKernelGetTickCount();
    uint16_t idx = 0;

    while ((osKernelGetTickCount() - start) < 500 && idx < len - 1)
    {
        uint8_t ch;
        if (board_uart_read(m_port, &ch, 1, 0.05, 0.05) == 1)
        {
            resp[idx++] = (char)ch;
            if (ch == '\n')
                break;
        }
    }

    resp[idx] = '\0';
    return idx;
}

bool BluetoothDriver::autoBaudScan()
{
	//波特率对频表
    const uint32_t baud_table[] = {
        2400, 4800, 9600, 19200, 38400, 57600,
        115200, 230400, 460800, 921600};

    char resp[64];

    for (uint8_t i = 0; i < sizeof(baud_table) / sizeof(uint32_t); i++)
    {
        uint32_t baud = baud_table[i];

        if (!board_uart_set_baudrate(m_port, baud))
            continue;

        osDelay(300);

        uint16_t rlen = sendAT("AT+QT\r\n", resp, sizeof(resp));

        if (rlen > 0 && strstr(resp, "QT+") != nullptr)
        {
            // 关闭重传
            sendAT("AT+CR00\r\n", resp, sizeof(resp));
            osDelay(200);
            // 设置波特率 460800 (CT09)
            sendAT("AT+CT09\r\n", resp, sizeof(resp));
            osDelay(500);
            // 设置模块 BLE 名
            sendAT("AT+BMAC-6368\r\n", resp, sizeof(resp));
            osDelay(500);
            // 芯片复位
            sendAT("AT+CZ\r\n", resp, sizeof(resp));
            osDelay(300);

            // MCU 端设为460800波特率
            board_uart_set_baudrate(m_port, 460800);

            m_initialized = true;
            return true;
        }
    }

    return false;
}

// =============================================================================
// 全局访问函数
// =============================================================================

static BluetoothDriver *g_drv_bt = nullptr;

void init_drv_bluetooth()
{
    if (g_drv_bt)
        return;
	
    g_drv_bt = new BluetoothDriver(BOARD_UART_BT);
    if (g_drv_bt)
        g_drv_bt->init();
}

BluetoothDriver *bluetooth_driver()
{
    return g_drv_bt;
}
