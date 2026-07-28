#include <stdio.h>
#include <stdint.h>
#include "board.hpp"

//printf 输出重定向: 原 BOARD_UART_DEBUG (UART4) 已被 US-100 超声波占用,
//此处将 printf 输出置为空操作, 保留接口避免破坏调用 printf 的代码.
//如需恢复调试输出, 可将 retarget_put 改为写入其它空闲 UART.
//以上符号在 semihosting_stubs.c 中均为 weak 定义, 此处的强定义会自动覆盖.

static void retarget_put(uint8_t b)
{
    (void)b;
}

static int retarget_write(const char *buf, int len)
{
    (void)buf;
    return len;
}

extern "C" {

//标准 ARM C 库: printf -> _sys_write
int _sys_write(int d, const char *buf, int len, int mode)
{
    (void)d;
    (void)mode;
    if (len <= 0)
        return 0;
    retarget_write(buf, len);
    return 0;       //0 = 全部写入完成
}

//标准 ARM C 库: 单字符终端通道
void _ttywrch(int ch)
{
    retarget_put((uint8_t)ch);
}

//MicroLib / 直接调用 fputc 路径
int fputc(int ch, FILE *f)
{
    (void)f;
    retarget_put((uint8_t)ch);
    return ch;
}

//LLVM libc / newlib 路径
int _write(int fd, const char *buf, int len)
{
    (void)fd;
    if (len <= 0)
        return 0;
    return retarget_write(buf, len);
}

} // extern "C"
