#include "9cc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void gen_lval(Node *node) {
  if (node->kind != ND_LVAR)
    error_exit("代入の左辺値が変数ではありません");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen_fun(Node *node) {
  static char buffer[1024];
  size_t len = MIN(node->identLength,
                   sizeof(buffer) / sizeof(buffer[0]) - 1);
  memcpy(buffer, node->ident, len);
  buffer[len] = '\0';
  printf("  call _%s\n", buffer);
}

void gen(Node *node) {
  static int label_sequence_no = 0;

  switch (node->kind) {
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_LVAR:
      gen_lval(node);
      printf("  pop rax\n");
      printf("  mov rax, [rax]\n");
      printf("  push rax\n");
      return;
    case ND_ASSIGN:
      gen_lval(node->lhs);
      gen(node->rhs);
      printf("  pop rdi\n");
      printf("  pop rax\n");
      printf("  mov [rax], rdi\n");
      printf("  push rdi\n");
      return;
    case ND_RETURN:
      gen(node->lhs);
      printf("  pop rax\n");
      printf("  mov rsp, rbp\n");
      printf("  pop rbp\n");
      printf("  ret\n");
      return;
    case ND_IF:
      gen(node->condition);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      if (node->rhs) {
        // elseがある場合
        printf("  je .Lelse%08d\n", label_sequence_no);
        gen(node->lhs);
        printf("  jmp .Lend%08d\n", label_sequence_no);
        printf(".Lelse%08d:\n", label_sequence_no);
        gen(node->rhs);
        printf(".Lend%08d:\n", label_sequence_no);
      } else {
        // elseがない場合
        printf("  je .Lend%08d\n", label_sequence_no);
        gen(node->lhs);
        printf(".Lend%08d:\n", label_sequence_no);
      }
      label_sequence_no++;
      return;
    case ND_WHILE:
      printf(".Lbegin%08d:\n", label_sequence_no);
      gen(node->condition);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .Lend%08d\n", label_sequence_no);
      gen(node->lhs);
      printf("  jmp .Lbegin%08d\n", label_sequence_no);
      printf(".Lend%08d:\n", label_sequence_no);
      label_sequence_no++;
      return;
    case ND_FOR:
      if (node->block->data[0]) {
        gen(node->block->data[0]);
      }
      printf(".Lbegin%08d:\n", label_sequence_no);
      if (node->block->data[1]) {
        gen(node->block->data[1]);
      }
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .Lend%08d\n", label_sequence_no);
      gen(node->lhs);
      if (node->block->data[2]) {
        gen(node->block->data[2]);
      }
      printf("  jmp .Lbegin%08d\n", label_sequence_no);
      printf(".Lend%08d:\n", label_sequence_no);
      label_sequence_no++;
      return;
    case ND_BLOCK:
      for (int i = 0; i < node->block->len; ++i) {
        gen(node->block->data[i]);
        printf("  pop rax\n");
      }
      return;
    case ND_FUN:
      gen_fun(node);
      return;
    default:
      break;
      // through
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
    case ND_EQUAL:
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_NOT_EQUAL:
      printf("  cmp rax, rdi\n");
      printf("  setne al\n");
      printf("  movzx rax, al\n");
      break;
    default:
      break;
      // through
  }

  printf("  push rax\n");
}