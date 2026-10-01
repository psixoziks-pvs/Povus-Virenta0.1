; ============================================================
;  Povus:Virenta — Bootloader
;  Первый код после BIOS
; ============================================================

[BITS 16]
[ORG 0x7C00]

start:
    ; Настройка сегментов
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Очистить экран
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    ; Вывести сообщение
    mov si, msg_boot
    call print_string

    mov si, msg_status
    call print_string

hang:
    hlt
    jmp hang

; --- Функция вывода строки ---
print_string:
    mov ah, 0x0E
.next_char:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .next_char
.done:
    ret

; --- Данные ---
msg_boot:    db "Povus:Virenta v0.1", 13, 10, 0
msg_status:  db "Boot OK. Kernel not loaded yet.", 13, 10, 0

; --- Сигнатура загрузчика ---
times 510 - ($ - $$) db 0
dw 0xAA55