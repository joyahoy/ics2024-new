#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// 辅助函数：将整数转换为字符串
static int itoa(int num, char *str, int base) {
    int i = 0;
    int is_negative = 0;

    // 处理0的特殊情况
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return i;
    }

    // 处理负数（仅十进制）
    if (num < 0 && base == 10) {
        is_negative = 1;
        num = -num;
    }

    // 转换数字（逆序）
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    // 添加负号
    if (is_negative) {
        str[i++] = '-';
    }

    // 反转字符串
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
        start++;
        end--;
    }

    str[i] = '\0';
    return i;
}

static int format_string(char *buf, const char *fmt, va_list args) {
    char *p = buf;
    char num_buf[32];
    
    for (; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            *p++ = *fmt;
            continue;
        }
        
        fmt++; // 跳过'%'
        switch (*fmt) {
            case 's': {
                char *s = va_arg(args, char *);
                while (*s) {
                    *p++ = *s++;
                }
                break;
            }
            case 'd': {
                int num = va_arg(args, int);
                int len = itoa(num, num_buf, 10);
                for (int i = 0; i < len; i++) {
                    *p++ = num_buf[i];
                }
                break;
            }
            case 'x': {
                unsigned int num = va_arg(args, unsigned int);
                int len = itoa(num, num_buf, 16);
                for (int i = 0; i < len; i++) {
                    *p++ = num_buf[i];
                }
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                *p++ = c;
                break;
            }
            case '%': {
                *p++ = '%';
                break;
            }
            default: {
                *p++ = '%';
                *p++ = *fmt;
                break;
            }
        }
    }
    
    *p = '\0';
    return p - buf;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
    return format_string(out, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsprintf(out, fmt, args);
    va_end(args);
    return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
    int ret = format_string(out, fmt, ap);
    if (n > 0) {
        if ((size_t)ret >= n) {
            out[n-1] = '\0';
            ret = n-1;
        }
    }
    return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(out, n, fmt, args);
    va_end(args);
    return ret;
}

int printf(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int ret = vsprintf(buf, fmt, args);
    va_end(args);
    
    for (int i = 0; i < ret; i++) {
        putch(buf[i]);
    }
    return ret;
}

#endif
