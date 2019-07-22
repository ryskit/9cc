#include "9cc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// 入力プログラム
char *user_input;

int main(int argc, char **argv) {
    if (argc != 2) {
        error_exit("引数の個数が正しくありません");
        return 1;
    }

    // トークナイズしてパースする
    token = tokenize(argv[1]);
    Node *node = expr();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global _main\n");
    printf("_main:\n");

    // 抽象構文木を下りながらコード生成
    gen(node);

    // スタックトップに式全体の値が残っているはずなので
    // それをRAXにロードして関数からの返り値とする
    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}

// エラー箇所を報告するための関数
//  printfと同じ引数を取る
void error_exit(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}
