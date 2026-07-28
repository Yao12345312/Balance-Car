#pragma once

#include "board.hpp"
#include "cmsis_os2.h"
#include "u8g2.h"
#include <stdint.h>

// OLED 显示驱动 (SSD1306 128x64, SPI 接口)
// SPI 数据传输走 board_spi_write, CS/DC/RES 由本驱动以 GPIO 直接控制
class DrvOLED
{
public:
    DrvOLED(BoardSpiPort port);

    // 初始化 GPIO(CS/DC/RES) + 硬件复位 + u8g2 建屏
    bool init();

    // ---- SSD1306 原始命令/数据 (CS/DC 由内部管理) ----
    void writeCMD(uint8_t cmd);
    void writeData(uint8_t data);
    void writeDataBuffer(const uint8_t *data, uint8_t count);

    // ---- u8g2 绘图接口 ----
    void clear();
    void update();

    void showString(uint8_t x, uint8_t y, const char *str);
    void showImage(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *image);
    void drawPixel(uint8_t x, uint8_t y);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    void drawCircle(int16_t x, int16_t y, uint8_t radius, uint8_t is_filled);
    void drawRectangle(int16_t x, int16_t y, uint8_t width, uint8_t height, uint8_t is_filled);
    void showChinese(int16_t x, int16_t y, const char *chinese);
    void showFloat(uint8_t x, uint8_t y, float num);

    // ---- 预置图标 ----
    void displayLogo();
    void showBLEON();
    void showBLEOFF();
    void lowBattery();
    void middleBattery();
    void highBattery();
    void fullBattery();
    void warn();

    u8g2_t *u8g2() { return &m_u8g2; }

    // ===== 调试: 绕过 u8g2 直接打原始命令, 定位黑屏根因 =====
    // step=1: 仅翻转 CS/DC/RES (万用表测电平)
    // step=2: 原始 SSD1306 初始化 + 全屏点亮 (不经过 u8g2)
    void debugTest(uint8_t step);

private:
    // GPIO 控制
    void csLow();
    void csHigh();
    void dcLow();
    void dcHigh();
    void rstLow();
    void rstHigh();

    void spiSend(const uint8_t *data, uint16_t len);   // SPI 批量发送
    void initGPIO();
    void hardwareReset();
    void initSSD1306();        // 手动 SSD1306 初始化命令序列 (与 u8g2 等价, 保留备用)

    // u8g2 回调 (静态, 路由到单例)
    static uint8_t u8x8ByteCb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
    static uint8_t u8x8GpioDelayCb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

    BoardSpiPort m_port;
    u8g2_t m_u8g2;
    bool m_inited;

    static DrvOLED *s_active;
};

void init_drv_oled();
DrvOLED *drv_oled();
