.data
print_fmt: .string "%ld \n"

.text

.globl main
main:
  pushq %rbp
  movq %rsp, %rbp
  subq $16, %rsp
  movq $2, %rax
  movq %rax, -8(%rbp)
  movq $3, %rax
  movq %rax, -16(%rbp)
  movq -8(%rbp), %rax
  pushq %rax
  movq -16(%rbp), %rax
  movq %rax, %rcx
  popq %rax
  movq %rax, %rdi
  movq %rcx, %rsi
  call potencia
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
