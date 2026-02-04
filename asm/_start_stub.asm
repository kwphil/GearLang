[bits 64]
global _start
extern _entry

section .text
_start:
    mov     rdi,    rsp
    and     rsp,    -16
    call    _entry    