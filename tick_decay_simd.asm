; void tick_decay_simd(uint8_t *energy, uint8_t *hunger, size_t n);
;
; SysV AMD64 ABI: rdi=energy, rsi=hunger, rdx=n.
; Contract: n is a multiple of 16; energy and hunger are 16-byte aligned
; (enforced by _Alignas(16) on the matching World fields).
;
; psubusb / paddusb are saturating: the [0,255] clamp falls out for free,
; no per-element branch. 16 bytes per iteration.

bits 64
default rel

section .rodata
align 16
ones16: times 16 db 1

section .text
global tick_decay_simd

tick_decay_simd:
    test    rdx, rdx
    jz      .done
    movdqa  xmm1, [ones16]
    xor     rax, rax
.loop:
    movdqa  xmm0, [rdi + rax]
    psubusb xmm0, xmm1
    movdqa  [rdi + rax], xmm0

    movdqa  xmm0, [rsi + rax]
    paddusb xmm0, xmm1
    movdqa  [rsi + rax], xmm0

    add     rax, 16
    cmp     rax, rdx
    jb      .loop
.done:
    ret

; Mark the stack as non-executable. Without this, the linker assumes the
; object needs an executable stack, which propagates to the whole binary.
section .note.GNU-stack noalloc noexec nowrite progbits
