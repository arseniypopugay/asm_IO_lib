section .data 
symbols: db "0123456789--"

section .text
 
; Принимает код возврата и завершает текущий процесс
exit: 
    mov rax, 60
    syscall

; Принимает указатель на нуль-терминированную строку, возвращает её длину
string_length: ; str_len(rdi)
    xor rax, rax

    .loop:
        cmp byte[rdi+rax], 0
        je .end
        inc rax
        jmp .loop
    .end:
        ret

; Принимает указатель на нуль-терминированную строку, выводит её в stdout
print_string:
    
    call string_length
    mov rdx, rax
    mov rsi, rdi
    mov rax, 1
    mov rdi, 1
    syscall
    xor rax, rax
    ret

; Принимает код символа и выводит его в stdout
print_char: ;print_char(rdi)
    mov rax, 1
    push rdi
    mov rsi, rsp
    mov rdi, 1
    mov rdx, 1
    syscall
    pop rdi
    xor rax, rax
    ret

; Переводит строку (выводит символ с кодом 0xA)
print_newline:
    mov rdi, 0xA
    call print_char
    xor rax, rax
    ret

; Выводит беззнаковое 8-байтовое число в десятичном формате 
; Совет: выделите место в стеке и храните там результаты деления
; Не забудьте перевести цифры в их ASCII коды.
print_uint: ;print(rdi)
    mov rax, rdi
    mov rsi, 10
    mov r8, 0
    .loop:
        xor rdx, rdx
        div rsi
        push word[symbols+rdx]
        inc r8
        cmp rax, 0
        jnz .loop

    .readloop:
        xor rdi, rdi
        pop di
        call print_char
        sub r8, 1
        cmp r8, 0
        jnz .readloop

    xor rax, rax
    ret

; Выводит знаковое 8-байтовое число в десятичном формате 
print_int: ;print_int(rdi)

    mov r8, rdi
    cmp r8, 0
    jge .abs

    neg r8
    mov rdi, [symbols+10]
    call print_char

    .abs:
        mov rdi, r8
        call print_uint
    xor rax, rax
    ret

; Принимает два указателя на нуль-терминированные строки, возвращает 1 если они равны, 0 иначе
string_equals: ;rdi, rsi

    xor rcx, rcx
    xor rdx, rdx

.loop:
    mov cl, [rsi]
    mov dl, [rdi]

    cmp cl, 0
    je .end_test

    cmp cl, dl
    jne .BAD

    inc rsi
    inc rdi
    jmp .loop

.end_test:
    cmp dl, 0
    jne .BAD 

    mov rax, 1
    ret
.BAD:
    xor rax, rax
    ret

; Читает один символ из stdin и возвращает его. Возвращает 0 если достигнут конец потока

read_char:
    
    xor rax, rax
    push rax
    xor rdi, rdi
    mov rsi, rsp
    mov rdx, 1
    syscall


    xor rax, rax
    mov al, byte[rsp]
    pop r8
    ret


; Принимает: адрес начала буфера, размер буфера
; Читает в буфер слово из stdin, пропуская пробельные символы в начале, .
; Пробельные символы это пробел 0x20, табуляция 0x9 и перевод строки 0xA.
; Останавливается и возвращает 0 если слово слишком большое для буфера
; При успехе возвращает адрес буфера в rax, длину слова в rdx.
; При неудаче возвращает 0 в rax
; Эта функция должна дописывать к слову нуль-терминатор

read_word: ;rdi, rsi
    push r12 ;will use as counter
    push r13 ;will use as link to buffer
    push r14; will use as size of buffer
    xor r12, r12
    mov r13, rdi
    mov r14, rsi

.A:
   call read_char
   cmp al, 0x20
   je .A
   cmp al, 0x9
   je .A
   cmp al, 0xA
   je .A
   cmp al, 0
   je .END
   jmp .B
.B:
   mov byte[r13+r12], al
   inc r12
   call read_char
   cmp al, 0x20
   je .END
   cmp al, 0x9
   je .END
   cmp al, 0xA
   je .END
   cmp al, 0
   je .END
   cmp r12, r14
   je .ERROR

    
   jmp .B
   ret

.END:
    mov byte[r13+r12], 0
    mov rax, r13
    mov rdx, r12
    pop r14
    pop r13
    pop r12
    ret

.ERROR:
    pop r14
    pop r13
    pop r12
    xor rax, rax
    ret
   

; Принимает указатель на строку, пытается
; прочитать из её начала беззнаковое число.
; Возвращает в rax: число, rdx : его длину в символах
; rdx = 0 если число прочитать не удалось
parse_uint: ;rdi
  xor rax, rax
  xor rcx, rcx
.A:
  mov cl, [rdi]
  inc rdi
  cmp cl, '0'
  jb .ERR
  cmp cl, '9'
  ja .ERR
;; a -> b
  mov r8, 1
  sub cl, '0'
  mov rax, rcx 
  jmp .B
.B:
  mov cl, [rdi]
  inc rdi
  cmp cl, '0'
  jb .OK
  cmp cl, '9'
  ja .OK

;; b -> b
  inc r8
  ;; rax *= 10
  ;; rax += cl - '0'
  mov r10, 10
  mul r10  ;; result = rdx:rax
  sub cl, '0'
  add rax, rcx

  jmp .B
.OK:
  mov rdx, r8
  ret   
.ERR:
  xor rdx, rdx
  ret


; Принимает указатель на строку, пытается
; прочитать из её начала знаковое число.
; Если есть знак, пробелы между ним и числом не разрешены.
; Возвращает в rax: число, rdx : его длину в символах (включая знак, если он был) 
; rdx = 0 если число прочитать не удалось
parse_int:; rdi
    push r12
    xor r12, r12
    xor rcx, rcx
    mov cl, [rdi]
    cmp cl, '-'
    jne .PARSE_NUMS
    mov r12, 1
    inc rdi

.PARSE_NUMS:
    call parse_uint

    cmp rdx, 0
    je .ERR
    
    cmp r12, 1
    jne .OK

    inc rdx
    neg rax

.OK:
    pop r12
    ret
  
.ERR:
    pop r12
    xor rdx, rdx
    ret

; Принимает указатель на строку, указатель на буфер и длину буфера
; Копирует строку в буфер
; Возвращает длину строки если она умещается в буфер, иначе 0
string_copy:     ;rdi, rsi, rdx

    call string_length ;rax => str_len
    cmp rax, rdx
    jae .finale    

    xor rcx, rcx
    .loop:
        
        mov cl, [rdi]
        mov [rsi], cl
        sub rdx, 1
        inc rsi
        inc rdi
        test rdx, rdx
        jnz .loop

    ret
    
    .finale:
        mov rax, 0
        ret