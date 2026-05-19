global find_best_set

extern printf

extern item_count   ; u16
extern capacity     ; u16
extern sizes        ; u16 [16 * item_count]
extern values       ; u16 [16 * item_count] 

section .data
numfmt: db "%d", 10, 0 ; 3 characters total, 4 bytes

section .text

; u32 find_best_set(u64, u64, void*)
find_best_set:
    ; save base pointer, stack alloc
    push rbp
    mov rbp, rsp

    push rax
    push r8
    push r9
    push r10

    ; rdi - i_min ( also i current )
    ; rsi - i_max
    ; rdx - output (16 * u64 + 16 * u16)
    ; 32 bytes for vector unrolling
    sub rsp, 64
    mov [ rbp - 8 ], rdx

    ; rdi holds 64 bits (of which, only item_count bits we care about) 
    ; each bit is an element of the full set, so in a set like
    ; d = { A B C D }, and i = 0b0110, the subset(i, d) = { B, C }

    vxorpd ymm4, ymm4, ymm4 ; ymm4 - max_value  ymm4 = 0

    movzx eax, word [ rel capacity ] ; ymm0 - total_capacity. filled with extern value
    add eax, 1 ; we cannot do <= or >= easily, so instead we add one..
    movd xmm0, eax
    vpbroadcastw ymm0, xmm0

.iloop:
    ; if i >= i_max
    cmp rdi, rsi
    jge .end
    
    vxorpd ymm1, ymm1, ymm1 ; ymm1 - total_size   ymm1 = 0
    vxorpd ymm2, ymm2, ymm2 ; ymm2 - total_value  ymm2 = 0

    lea r8, [ rel values ]
    lea r9, [ rel sizes ]
    mov r10, rdi ; temporary mask (our iteration variable)

.jloop: ; jloop calculates the size and value of subset i
    tzcnt rcx, r10 ; count trailing zeros
    shl rcx, 5

    vmovdqu ymm3, [ r9 + rcx ] ; ymm3 - current_size
    vpaddw ymm1, ymm3   ; total_size  += current_size

    vpcmpgtw ymm3, ymm1, ymm0
    vpmovmskb eax, ymm3
    cmp eax, 0xFFFFFFFF
    je .writeend ; maybe can jump further?? (skip more)


    vmovdqu ymm3, [ r8 + rcx ] ; ymm3 - current_value
    vpaddw ymm2, ymm3   ; total_value += current_value

    mov rcx, r10
    sub rcx, 1
    and r10, rcx
    jnz .jloop ; not below zero

.iloopc:
    ; check whether the size is below max
    vpcmpgtw ymm1, ymm0, ymm1   ; ymm1 = (ymm0 > ymm1) ? 0xFFFF : 0x0000 per dimension

    ; if value greater than max for this one, save it
    vpcmpgtw ymm3, ymm2, ymm4   ; ymm3 = (ymm2 > ymm4) ? 0xFFFF : 0x0000 per dimension

    vpand ymm1, ymm1, ymm3 ; which ones we found a better set for. this set is our i (rdi)

    ; for each dimension, update the total max value and and setid based on ymm1 as the mask

    vpand ymm2, ymm1, ymm2 ; mask out the total_value only of the ones which we update

    vpcmpeqq ymm8, ymm8, ymm8 ; ymm8 = all 1s
    vpxor ymm8, ymm8, ymm1    ; ymm8 = ~ymm1
    vpand ymm4, ymm4, ymm8    ; ymm4 &= ~ymm1, essentially clear out the ones which we want to change
    vpor ymm4, ymm4, ymm2     ; then write max values

    ; write the best sets
    vmovdqu [ rbp - 64 ], ymm1

    mov rax, [ rbp - 8 ]    ; pointer to the resulting array of u64 results[16]
    mov rcx, 15 
    mov rdx, rbp
    sub rdx, 34

.writeloop:
    cmp word [ rdx ], 0
    jz .writeloopc

    mov [ rax + rcx * 8 ], rdi

.writeloopc:
    sub rdx, 2
    sub rcx, 1
    jnb .writeloop
    
.writeend:
    add rdi, 1
    jmp .iloop


.end:
    ; spread out the ymm4 (max value) vector
    mov rcx, [ rbp - 8 ]

    sub rsp, 64
    vmovdqu [ rbp - 64 ], ymm4

    mov rax, [ rbp - 64 ]
    mov [ rcx + 128 ], rax

    mov rax, [ rbp - 56 ]
    mov [ rcx + 136 ], rax

    mov rax, [ rbp - 48 ]
    mov [ rcx + 144 ], rax

    mov rax, [ rbp - 40 ]
    mov [ rcx + 152 ], rax


    ; finalize
    add rsp, 64
    add rsp, 64

    pop r10
    pop r9
    pop r8
    pop rax
    mov rsp, rbp
    pop rbp

    mov rax, 0
    ret
    

