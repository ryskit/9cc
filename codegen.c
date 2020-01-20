#include "9cc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static int stackpos = 0;

void gen_push(char *reg) {
  printf("  push %s\n", reg);
  stackpos += 8;
}

void gen_push_num(int v) {
  printf("  push %d\n", v);
  stackpos += 8;
}

void gen_pop(char *reg) {
  printf("  pop %s\n", reg);
  stackpos -= 8;
}

void gen_lval(Node *node) {
  if (node->kind != ND_LVAR)
    error_exit("代入の左辺値が変数ではありません");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  gen_push("rax");
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void gen_fun(Node *node) {
  static char buffer[1024];
  static char *registers[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

  int ops = stackpos;
  bool padding = (stackpos % 16 != 0);
  if (padding) {
    printf("  sub rsp, 8\n"); // RSPを16でアラインする
    stackpos += 8;
  }

  for (int i = 0; i < node->block->len; ++i) {
    Node *argNode = (Node *)node->block->data[i];
    if (argNode->kind == ND_NUM) { // 整数定数の場合
      printf("  mov %s, %d\n", registers[i], argNode->val);
    } else if (argNode->kind == ND_LVAR) { // ローカル変数の場合
      gen_lval(argNode);
      gen_pop("rax");
      printf("  mov %s, [rax]\n", registers[i]);
    }
  }

  size_t len = MIN(node->identLength,
                   sizeof(buffer) / sizeof(buffer[0]) - 1);
  memcpy(buffer, node->ident, len);
  buffer[len] = '\0';
  printf("  call _%s\n", buffer);

  if (padding) {
    printf("  add rsp, 8\n"); // 16バイトアラインの後始末
    stackpos -= 8;
  }
}

GenResult gen(Node *node) {
  static int label_sequence_no = 0;

  switch (node->kind) {
    case ND_NUM:
      gen_push_num(node->val);
      return GEN_PUSHED_RESULT;
    case ND_LVAR: // 変数の参照
      gen_lval(node);
      gen_pop("rax");
      printf("  mov rax, [rax]\n");
      gen_push("rax");
      return GEN_PUSHED_RESULT;
    case ND_ASSIGN: // 変数への代入
      gen_lval(node->lhs);
      gen(node->rhs);
      gen_pop("rdi");
      gen_pop("rax");
      printf("  mov [rax], rdi\n");
      gen_push("rdi");
      return GEN_PUSHED_RESULT;
    case ND_RETURN:
      gen(node->lhs);
      gen_pop("rax");
      printf("  mov rsp, rbp\n");
      gen_pop("rbp");
      printf("  ret\n");
      return GEN_DONT_PUSHED_RESULT;
    case ND_IF:
      gen(node->condition);
      gen_pop("rax");
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
      return GEN_PUSHED_RESULT;
    case ND_WHILE:
      printf(".Lbegin%08d:\n", label_sequence_no);
      gen(node->condition);
      gen_pop("rax");
      printf("  cmp rax, 0\n");
      printf("  je .Lend%08d\n", label_sequence_no);
      gen(node->lhs);
      printf("  jmp .Lbegin%08d\n", label_sequence_no);
      printf(".Lend%08d:\n", label_sequence_no);
      label_sequence_no++;
      return GEN_PUSHED_RESULT;
    case ND_FOR:
      if (node->block->data[0]) {
        gen(node->block->data[0]);
      }
      printf(".Lbegin%08d:\n", label_sequence_no);
      if (node->block->data[1]) {
        gen(node->block->data[1]);
      }
      gen_pop("rax");
      printf("  cmp rax, 0\n");
      printf("  je .Lend%08d\n", label_sequence_no);
      gen(node->lhs);
      if (node->block->data[2]) {
        gen(node->block->data[2]);
      }
      printf("  jmp .Lbegin%08d\n", label_sequence_no);
      printf(".Lend%08d:\n", label_sequence_no);
      label_sequence_no++;
      return GEN_PUSHED_RESULT;
    case ND_BLOCK:
      for (int i = 0; i < node->block->len; ++i) {
        gen(node->block->data[i]);
        //gen_pop("rax");
      }
      return GEN_PUSHED_RESULT;
    case ND_FUN:
      gen_fun(node);
      return GEN_PUSHED_RESULT;
    default:
      break;
      // through
  }

  gen(node->lhs);
  gen(node->rhs);

  // 二項演算子系
  gen_pop("rdi");
  gen_pop("rax");

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

  gen_push("rax");
  return GEN_PUSHED_RESULT;
}