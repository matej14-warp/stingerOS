





static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void serial_putchar(char c) {

    while (!(inb(0x3FD) & 0x20)) {}

    __asm__ volatile("outb %0, $0x3F8" : : "a"(c));
}

static inline void serial_puts(const char *s) {
    while (*s) serial_putchar(*s++);
}


static inline void serial_printf(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    while (*fmt) {
        if (*fmt != '%') { serial_putchar(*fmt++); continue; }
        fmt++;
        switch (*fmt++) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            serial_puts(s);
            break;
        }
        case 'd': {
            int v = va_arg(ap, int);
            char buf[12]; int i = 0;
            if (v < 0) { serial_putchar('-'); v = -v; }
            if (!v) buf[i++] = '0';
            while (v) { buf[i++] = '0' + v % 10; v /= 10; }
            while (i > 0) serial_putchar(buf[--i]);
            break;
        }
        case 'u': {
            unsigned v = va_arg(ap, unsigned);
            char buf[12]; int i = 0;
            if (!v) buf[i++] = '0';
            while (v) { buf[i++] = '0' + v % 10; v /= 10; }
            while (i > 0) serial_putchar(buf[--i]);
            break;
        }
        case 'x': case 'X': {
            unsigned v = va_arg(ap, unsigned);
            const char *h = "0123456789abcdef";
            char buf[9]; int i = 0;
            if (!v) buf[i++] = '0';
            while (v) { buf[i++] = h[v & 0xF]; v >>= 4; }
            while (i > 0) serial_putchar(buf[--i]);
            break;
        }
        case '%': serial_putchar('%'); break;
        default: serial_putchar(*(fmt-1)); break;
        }
    }
    va_end(ap);
}






