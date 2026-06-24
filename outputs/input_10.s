.data
print_fmt: .string "%ld \n"

.text

.globl main
main:
  pushq %rbp
  movq %rsp, %rbp
  subq $32, %rsp
  movq $2, %rax
  pushq %rax
  movq $2, %rax
  popq %rcx
  imulq %rcx, %rax
  salq $3, %rax
  movq %rax, %rdi
  call malloc@PLT
  movq %rax, -8(%rbp)
  movq $1, %rax
  pushq %rax
  movq $0, %rax
  pushq %rax
  movq $0, %rax
  movq %rax, %rdi
  popq %rax
  movq $2, %rcx
  imulq %rcx, %rax
  addq %rdi, %rax
  movq %rax, %rdi
  popq %rcx
  movq -8(%rbp), %rax
  movq %rcx, (%rax, %rdi, 8)
  movq $2, %rax
  pushq %rax
  movq $0, %rax
  pushq %rax
  movq $1, %rax
  movq %rax, %rdi
  popq %rax
  movq $2, %rcx
  imulq %rcx, %rax
  addq %rdi, %rax
  movq %rax, %rdi
  popq %rcx
  movq -8(%rbp), %rax
  movq %rcx, (%rax, %rdi, 8)
  movq $3, %rax
  pushq %rax
  movq $1, %rax
  pushq %rax
  movq $0, %rax
  movq %rax, %rdi
  popq %rax
  movq $2, %rcx
  imulq %rcx, %rax
  addq %rdi, %rax
  movq %rax, %rdi
  popq %rcx
  movq -8(%rbp), %rax
  movq %rcx, (%rax, %rdi, 8)
  movq $4, %rax
  pushq %rax
  movq $1, %rax
  pushq %rax
  movq $1, %rax
  movq %rax, %rdi
  popq %rax
  movq $2, %rcx
  imulq %rcx, %rax
  addq %rdi, %rax
  movq %rax, %rdi
  popq %rcx
  movq -8(%rbp), %rax
  movq %rcx, (%rax, %rdi, 8)
  movq $0, %rax
  movq %rax, -16(%rbp)
  movq $0, %rax
  movq %rax, -32(%rbp)
while_0:
  movq -16(%rbp), %rax
  pushq %rax
  movq $2, %rax
  movq %rax, %rcx
  popq %rax
  cmpq %rcx, %rax
  movq $0, %rax
  setl %al
  movzbq %al, %rax
  cmpq $0, %rax
  je endwhile_0
  movq $0, %rax
  movq %rax, -24(%rbp)
while_1:
  movq -24(%rbp), %rax
  pushq %rax
  movq $2, %rax
  movq %rax, %rcx
  popq %rax
  cmpq %rcx, %rax
  movq $0, %rax
  setl %al
  movzbq %al, %rax
  cmpq $0, %rax
  je endwhile_1
  movq -16(%rbp), %rax
  pushq %rax
  movq -24(%rbp), %rax
  movq %rax, %rdi
  popq %rax
  movq $2, %rcx
  imulq %rcx, %rax
  addq %rdi, %rax
  movq %rax, %rdi
  movq -8(%rbp), %rax
  movq (%rax, %rdi, 8), %rax
  pushq %rax
  movq $3, %rax
  movq %rax, %rcx
  popq %rax
  movq %rax, %rdi
  movq %rcx, %rsi
  call potencia
  pushq %rax
  movq -32(%rbp), %rax
  movq %rax, %rcx
  popq %rax
  xchgq %rax, %rcx
  addq %rcx, %rax
  movq %rax, -32(%rbp)
  movq -24(%rbp), %rax
  pushq %rax
  movq $1, %rax
  movq %rax, %rcx
  popq %rax
  addq %rcx, %rax
  movq %rax, -24(%rbp)
  jmp while_1
endwhile_1:
  movq -16(%rbp), %rax
  pushq %rax
  movq $1, %rax
  movq %rax, %rcx
  popq %rax
  addq %rcx, %rax
  movq %rax, -16(%rbp)
  jmp while_0
endwhile_0:
  movq -32(%rbp), %rax
  movq %rax, %rsi
  leaq print_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  movq $0, %rax
  jmp .end_main
.end_main:
  leave
  ret

.globl potencia
potencia:
  pushq %rbp
  movq %rsp, %rbp
  cmpq $0, %rsi
  je potencia_n_zero
  cmpq $1, %rsi
  je potencia_n_one
  pushq %rdi
  movq %rsi, %rdx
  andq $1, %rdx
  pushq %rdx
  movq %rdi, %rax
  imulq %rdi, %rax
  movq %rax, %rdi
  sarq $1, %rsi
  call potencia
  popq %rdx
  popq %rcx
  cmpq $0, %rdx
  je potencia_end
  imulq %rcx, %rax
  jmp potencia_end
potencia_n_zero:
  movq $1, %rax
  jmp potencia_end
potencia_n_one:
  movq %rdi, %rax
  jmp potencia_end
potencia_end:
  leave
  ret

.section .note.GNU-stack,"",@progbits
