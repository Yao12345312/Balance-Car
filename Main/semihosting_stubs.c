// semihosting_stubs.c
#include <stdio.h>

__asm(".global __use_no_semihosting\n\t"); // 注释本行, 方法1

// 所有 _sys_* 必须用 weak
__attribute__((weak)) int _sys_open(const char *name, int openmode)
{
    (void) name;
    (void) openmode;
    return -1;
}

__attribute__((weak)) void _sys_close(int d)
{
    (void) d;
}

__attribute__((weak)) int _sys_write(int d, const char *buf, int len, int mode)
{
    (void) d;
    (void) buf;
    (void) mode;
    return len;
}

__attribute__((weak)) int _sys_read(int d, char *buf, int len, int mode)
{
    (void) d;
    (void) buf;
    (void) mode;
    return 0;
}

__attribute__((weak)) int _sys_istty(int d)
{
    (void) d;
    return 0;
}
__attribute__((weak)) int _sys_seek(int d, int ptr, int dir)
{
    (void) d;
    (void) ptr;
    (void) dir;
    return -1;
}
__attribute__((weak)) int _sys_flen(int d)
{
    (void) d;
    return -1;
}
__attribute__((weak)) int _sys_ensure(int d)
{
    (void) d;
    return -1;
}
__attribute__((weak)) char *_sys_command_string(char *cmd, int len)
{
    (void) cmd;
    (void) len;
    return 0;
}
__attribute__((weak)) void _sys_exit(int code)
{
    (void) code;
}
__attribute__((weak)) void _ttywrch(int ch)
{
    (void) ch;
}

// 可选：_write 也重定向
__attribute__((weak)) int _write(int fd, const char *buf, int len)
{
    return 0;
}