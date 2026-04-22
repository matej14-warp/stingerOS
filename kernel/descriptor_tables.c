








static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static gdt_entry_t gdt_entries[5];
static gdt_ptr_t   gdt_ptr;

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;
    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt_entries[num].access      = access;
}

void gdt_init(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;
    gdt_set_gate(0, 0, 0,          0x00, 0x00);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    gdt_flush((uint32_t)&gdt_ptr);
}

static idt_entry_t idt_entries[256];
static idt_ptr_t   idt_ptr;
static isr_t       interrupt_handlers[256];

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_low  = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].sel       = sel;
    idt_entries[num].always0   = 0;
    idt_entries[num].flags     = flags;
}

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

static const char *exc_names[] = {
    "Division By Zero","Debug","Non-Maskable Interrupt","Breakpoint",
    "Overflow","Bound Range Exceeded","Invalid Opcode","Device Not Available",
    "Double Fault","Coprocessor Segment Overrun","Invalid TSS","Segment Not Present",
    "Stack Segment Fault","General Protection Fault","Page Fault","Reserved",
    "x87 FPU Exception","Alignment Check","Machine Check","SIMD Floating-Point",
};

static void panic_puthex(uint32_t v) {
    static const char h[] = "0123456789ABCDEF";
    terminal_writestring("0x");
    for (int i = 28; i >= 0; i -= 4)
        terminal_putchar(h[(v >> i) & 0xF]);
}

void kernel_panic(const char *msg, registers_t *regs) {
    __asm__ volatile("cli");
    volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
    uint16_t blank = (uint16_t)((VGA_COLOR_RED << 12) | (VGA_COLOR_WHITE << 8) | ' ');
    for (int i = 0; i < 80 * 25; i++) vga[i] = blank;
    terminal_initialize();
    terminal_setcolor(terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    for (int i = 0; i < 80 * 25; i++) vga[i] = blank;
    terminal_setcolor(terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    terminal_writestring(
        "                                                                                "
        "          >>>>>  stinger OS  --  KERNEL PANIC  <<<<<                         "
        "                                                                                "
    );
    terminal_setcolor(terminal_make_color(VGA_COLOR_YELLOW, VGA_COLOR_RED));
    terminal_writestring("\n  *** ");
    terminal_writestring(msg ? msg : "unknown error");
    terminal_writestring(" ***\n");
    if (regs) {
        uint32_t vec = regs->int_no;
        const char *ename = (vec < 20) ? exc_names[vec] : "Unknown Exception";
        terminal_setcolor(terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
        terminal_writestring("\n  Exception : ");
        terminal_setcolor(terminal_make_color(VGA_COLOR_YELLOW, VGA_COLOR_RED));
        terminal_putchar('0' + (char)((vec / 10) % 10));
        terminal_putchar('0' + (char)(vec % 10));
        terminal_writestring("  ("); terminal_writestring(ename); terminal_writestring(")");
        terminal_setcolor(terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
        terminal_writestring("\n  Error code: ");
        terminal_setcolor(terminal_make_color(VGA_COLOR_YELLOW, VGA_COLOR_RED));
        panic_puthex(regs->err_code);
        terminal_setcolor(terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
        terminal_writestring("\n\n  Registers:\n");
        terminal_setcolor(terminal_make_color(VGA_COLOR_YELLOW, VGA_COLOR_RED));
        terminal_writestring("    EIP="); panic_puthex(regs->eip);
        terminal_writestring("  CS=");   panic_puthex(regs->cs);
        terminal_writestring("  EFLAGS="); panic_puthex(regs->eflags);
        terminal_writestring("\n    EAX="); panic_puthex(regs->eax);
        terminal_writestring("  EBX="); panic_puthex(regs->ebx);
        terminal_writestring("  ECX="); panic_puthex(regs->ecx);
        terminal_writestring("  EDX="); panic_puthex(regs->edx);
        terminal_writestring("\n    ESI="); panic_puthex(regs->esi);
        terminal_writestring("  EDI="); panic_puthex(regs->edi);
        terminal_writestring("  EBP="); panic_puthex(regs->ebp);
        terminal_writestring("  ESP="); panic_puthex(regs->esp);
        terminal_writestring("\n");
        if (vec == 14) {
            uint32_t cr2;
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
            terminal_setcolor(terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
            terminal_writestring("    CR2="); panic_puthex(cr2);
            terminal_writestring("  (");
            if (regs->err_code & 1)  terminal_writestring("present ");
            else                      terminal_writestring("not-present ");
            if (regs->err_code & 2)  terminal_writestring("write ");
            if (regs->err_code & 4)  terminal_writestring("user ");
            if (regs->err_code & 8)  terminal_writestring("reserved-bit ");
            if (regs->err_code & 16) terminal_writestring("ifetch");
            terminal_writestring(")\n");
        }
    }
    terminal_setcolor(terminal_make_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    terminal_writestring("\n  System halted.  Reset to restart.\n");
    __asm__ volatile("cli; hlt");
    for (;;) __asm__ volatile("hlt");
}

registers_t* isr_handler(registers_t *regs) {
    if (interrupt_handlers[regs->int_no]) {
        interrupt_handlers[regs->int_no](regs);
        if (regs->int_no == 32) return schedule(regs);
        return regs;
    }
    if (regs->int_no < 32) {
        if (g_in_yaoigui) { yaoigui_crash_exit(); }
        kernel_panic("CPU exception", regs);
    }
    return regs;
}

registers_t* irq_handler(registers_t *regs) {
    if (interrupt_handlers[regs->int_no])
        interrupt_handlers[regs->int_no](regs);

    if (regs->int_no >= 40) outb(0xA0, 0x20);
    outb(0x20, 0x20);

    if (regs->int_no == 32) {
        return schedule(regs);
    }
    return regs;
}

static void ap_ipi_handler(registers_t *regs) {
    (void)regs;
    extern void lapic_eoi(void);
    lapic_eoi();
}

void idt_init(void) {
    idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;
    uint8_t *p = (uint8_t*)idt_entries;
    for (size_t i = 0; i < sizeof(idt_entries); i++) p[i] = 0;
    for (int i = 0; i < 256; i++) interrupt_handlers[i] = NULL;
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0x00); outb(0xA1, 0x00);
    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
    idt_set_gate(0x40, (uint32_t)irq_ap_ipi, 0x08, 0x8E);
    register_interrupt_handler(0x40, ap_ipi_handler);
    idt_flush((uint32_t)&idt_ptr);
}

void ap_load_idt(void) {
    idt_flush((uint32_t)&idt_ptr);
}




