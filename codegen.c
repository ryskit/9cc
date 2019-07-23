#include "9cc.h"
#include <stdio.h>

void gen(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    case ND_GREATER:
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzx rax, al\n");
        break;
    case ND_GREATER_EQUAL:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzx rax, al\n");
        break;
    case ND_NUM:
        error_exit("unrecognize");
        break;
  }

  printf("  push rax\n");
}
