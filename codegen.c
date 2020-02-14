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

    // 変数へのアドレスをスタックに積む
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    gen_push("rax");
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static const char *ArgRegsiters[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

void gen_fun(Node *node) {
    static char buffer[1024];

    int ops = stackpos;

    for (int i = 0; i < node->block->len; ++i) {
        Node *argNode = (Node *)node->block->data[i];
        if (argNode->kind == ND_NUM) { // 整数定数の場合
            printf("  mov %s, %d\n", ArgRegsiters[i], argNode->val);
        } else if (argNode->kind == ND_LVAR) { // ローカル変数の場合
            gen_lval(argNode);
            gen_pop("rax");
            printf("  mov %s, [rax]\n", ArgRegsiters[i]);
        }
    }

    size_t len = MIN(node->identLength,
                  sizeof(buffer) / sizeof(buffer[0]) - 1);
    memcpy(buffer, node->ident, len);
    buffer[len] = '\0';
    printf("  call _%s\n", buffer); // RIPをスタックに置いてlabelにジャンプ
}

void gen_fun_impl(Node *node) {
    static char name[1024];
    size_t len = MIN(node->identLength,
                  sizeof(name) / sizeof(name[0]) - 1);
    memcpy(name, node->ident, len);
    name[len] = '\0';

    // 関数ラベル
    printf("_%s:\n", name);

    const bool is_main = (strcmp(name, "main") == 0);
    const int nargs = node->block->len;

    // プロローグ
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  xor eax, eax\n"); // mov eax, 0 と同じ

    const int stack_size = is_main ? 256 : (nargs * 8);
    printf("  sub rsp, %d\n", stack_size); // スタックサイズ

    // 仮引数部分
    for (int i = 0; i < node->block->len; ++i) {
        Node *arg = (Node *)node->block->data[i];
        if (arg->kind != ND_LVAR)
            error_exit("代入の左辺値が変数ではありません: %s", NodeKindDescription(arg->kind));
        printf("  mov qword ptr [rbp - %d], %s\n", arg->offset, ArgRegsiters[i]);
    }

    // ブロック部分: node->lhsにはND_BLOCKが格納されている
    for (int i = 0; i < node->lhs->block->len; ++i) {
        GenResult resutl = gen(node->lhs->block->data[i]);
        // genが結果をスタックに積んだ場合は取り除いてRAXに入れ関数の戻り値とする
        if (resutl == GEN_PUSHED_RESULT) {
            gen_pop("rax");
        }
    }

    // エピローグ
    printf("  pop rbp\n");
    printf("  ret\n");
}

GenResult gen(Node *node) {
    static int label_sequence_no = 0;
    GenResult result;

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

    case ND_ASSIGN:
        /*
         * 変数への代入
         * - 変数のアドレスをスタックに置く ~ gen_lval()
         * - 右辺を評価し結果をスタックに置く ~ gen()
         * - スタックをポップし右辺値をrdiへ
         * - スタックをポップし変数アドレスをraxへ
         * - raxアドレスにrdi値を書く
         * - rdi値をスタックに置く
         */
        gen_lval(node->lhs);
        result = gen(node->rhs);
        assert(result == GEN_PUSHED_RESULT); // ここでのgenは必ずスタックに結果をプッシュしなければならない
        gen_pop("rdi"); // 右辺
        gen_pop("rax"); // 左辺(のアドレス)
        printf("  mov [rax], rdi\n"); // 左辺(のアドレス)にrdiの値をいれる
        gen_push("rdi");
        return GEN_PUSHED_RESULT;

    case ND_RETURN:
        /*
         * return文
         * - gen()の結果はスタックに置かれる
         * - スタックポップしてraxに
         * - 関数からリターンする
         */
        result = gen(node->lhs);
        assert(result == GEN_PUSHED_RESULT); // ここでのgenは必ずスタックに結果をプッシュしなければならない
        gen_pop("rax"); // 戻り値
        printf("  mov rsp, rbp\n");
        gen_pop("rbp"); // 戻りアドレス
        printf("  ret\n"); // スタックをポップしてそのアドレスにジャンプ
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
        }
        return GEN_PUSHED_RESULT;
    case ND_FUN:
        /*
         * 関数呼び出し
         * - 関数へのジャンプを生成する
         * - 戻り値が格納されているRAXをスタックに積む
         */
        gen_fun(node);
        gen_push("rax");
        return GEN_PUSHED_RESULT;
    case ND_FUN_IMPL:
        /*
         * 関数実装
         * - 関数の本体コードを生成する
         * - 評価結果はない
         */
        gen_fun_impl(node);
        return GEN_DONT_PUSHED_RESULT;
    case ND_ADDR:
        gen_lval(node->rhs);
        return GEN_PUSHED_RESULT;
    case ND_DEREF:
        gen(node->rhs);
        gen_pop("rax");
        printf("  mov rax, [rax]\n");
        gen_push("rax");
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