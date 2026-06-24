.data
print_fmt: .string "%ld \n"

.text

.globl main
main:
  pushq %rbp
  movq %rsp, %rbp
  subq $8, %rsp
  movq $4, %rax
  movq %rax, -8(%rbp)
  movq -8(%rbp), %rax
  imulq %rax, %rax
  imulq %rax, %rax
  pushq %rax
  movq -8(%rbp), %rax
  imulq %rax, %rax
  movq %rax, %rcx
  popq %rax
  addq %rcx, %rax
  movq %rax, %rsi
  leaq print_fmt(%rip), %rdi
  movq $0, %rax
  call printf@PLT
  movq $0, %rax
  jmp .end_main
.end_main:
  leave
  ret

.section .note.GNU-stack,"",@progbits
