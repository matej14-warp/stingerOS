; scorpion OS - boot/boot.asm
; Multiboot header + mini-kernel stub written in assembly
; Jumps into the main C kernel after basic setup

MBALIGN     equ 1 << 0              ; align loaded modules on page boundaries
MEMINFO     equ 1 << 1              ; provide memory map
VBEMODE     equ 1 << 2              ; request VBE framebuffer info
FLAGS       equ MBALIGN | MEMINFO | VBEMODE
MAGIC       equ 0x1BADB002          ; multiboot magic
CHECKSUM    equ -(MAGIC + FLAGS)    ; checksum

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    ; address fields (unused, set to 0)
    dd 0, 0, 0, 0, 0
    ; framebuffer request: mode_type=0 (linear), 1920x1080x32
    dd 0            ; mode_type: 0 = linear framebuffer
    dd 1920         ; width
    dd 1080         ; height
    dd 32           ; depth (bpp)

; -------------------------------------------------------
; Stack
; -------------------------------------------------------
section .bss
align 16
stack_bottom:
    resb 65536          ; 64 KiB kernel stack
stack_top:

; -------------------------------------------------------
; Entry point
; -------------------------------------------------------
section .text
global _start:function (_start.end - _start)

_start:
    ; Set up stack
    mov esp, stack_top

    ; Save multiboot info pointer (EBX) and magic (EAX)
    push ebx            ; multiboot info struct pointer
    push eax            ; multiboot magic number

    ; Clear EFLAGS
    push 0
    popf

    ; Call the C kernel main
    extern kernel_main
    call kernel_main

    ; Halt if kernel_main ever returns
    cli
.hang:
    hlt
    jmp .hang
.end:

; -------------------------------------------------------
; GDT Setup (flat 32-bit protected mode)
; -------------------------------------------------------
global gdt_flush
gdt_flush:
    mov eax, [esp+4]    ; get GDT pointer argument
    lgdt [eax]          ; load GDT
    mov ax, 0x10        ; data segment selector (index 2)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush     ; far jump to code segment
.flush:
    ret

; -------------------------------------------------------
; IDT flush
; -------------------------------------------------------
global idt_flush
idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret

; -------------------------------------------------------
; ISR stubs (CPU exceptions)
; -------------------------------------------------------
extern isr_handler

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push byte 0         ; dummy error code
    push byte %1        ; interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    push byte %1
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

isr_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp             ; pass pointer to registers_t as argument
    call isr_handler
    add esp, 4           ; pop the pointer
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

; -------------------------------------------------------
; IRQ stubs (hardware interrupts)
; -------------------------------------------------------
extern irq_handler

%macro IRQ 2
global irq%1
irq%1:
    cli
    push byte 0
    push byte %2
    jmp irq_common_stub
%endmacro

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

irq_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp             ; pass pointer to registers_t as argument
    call irq_handler
    add esp, 4           ; pop the pointer
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

; -------------------------------------------------------
; AP wakeup IPI stub (vector 0x40)
; Sent by BSP via smp_dispatch_ap() to wake a halted AP.
; Uses irq_common_stub so irq_handler dispatches it normally.
; -------------------------------------------------------
global irq_ap_ipi
irq_ap_ipi:
    cli
    push byte 0
    push byte 0x40
    jmp irq_common_stub
