#include "vector.h"
#include <stdio.h>
#include <string.h>

// MINマクロ
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_GREATER, // >
    ND_GREATER_EQUAL, // >=
    ND_EQUAL, // ==
    ND_NOT_EQUAL, // !=
    ND_ASSIGN,  // =
    ND_RETURN, // return
    ND_IF, // if
    ND_ELSE, // else
    ND_WHILE, // while
    ND_FOR, // for
    ND_LVAR,    // ローカル変数
    ND_BLOCK,    // ブロック
    ND_FUN, // 関数
    ND_FUN_IMPL, // 関数定義
    ND_NUM, // 整数
    ND_ADDR, // アドレス取得演算子
    ND_DEREF, // アドレス参照演算子
} NodeKind;

static inline const char* node_kind_descripion(NodeKind kind) {
    static const char* description[] = {
        "ADD", // +
        "SUB", // -
        "MUL", // *
        "DIV", // /
        "GREATER", // >
        "GREATER_EQUAL", // >=
        "EQUAL", // ==
        "NOT_EQUAL", // !=
        "ASSIGN",  // =
        "RETURN", // return
        "IF", // if
        "ELSE", // else
        "WHILE", // while
        "FOR", // for
        "LVAR",    // ローカル変数
        "BLOCK",    // ブロック
        "FUN", // 関数
        "FUN_IMPL", // 関数定義
        "NUM", // 整数
        "ADDR", // アドレス取得演算子
        "DEREF", // アドレス参照演算子
    };
    return description[kind];
}

typedef struct Node {
    NodeKind kind;      // 種別
    struct Node *lhs;   // 左辺
    struct Node *rhs;   // 右辺
    struct Node *condition; // 条件(ifの場合のみ)
    Vector *block;      // ブロック
    int val;            // kindがND_NUMの場合はその値、kindがND_FUNの場合、関数呼び出し確定済かどうかを示すフラグ値
    char *ident;        // kindがND_FUNの場合のみ使う(関数名)
    int identLength;    // 上記の長さ   
    int offset;         // kindがND_LVARの場合のみ使う
    int num_pointers;    // ポインタかどうか(ND_LVARの場合の有効)
} Node;

static inline const char* node_description(Node *node) {
    static char buffer[1024];
    static char tmp[1024];

    tmp[0] = '\0';
    if (node->kind == ND_FUN || node->kind == ND_FUN_IMPL || node->kind == ND_LVAR) {
        const int n = MIN(sizeof(tmp) - 1, node->identLength);
        memcpy(tmp, node->ident, n);
        tmp[n] = '\0';
    } else if (node->kind == ND_NUM) {
        const int n = sprintf(tmp, "%d", node->val);
        tmp[n] = '\0';
    }

    sprintf(buffer, "%-8s '%-6s' %3d %14p/%14p/%14p",
            node_kind_descripion(node->kind),
            tmp,
            node->offset,
            node->lhs,
            node->rhs,
            node->condition);
    return buffer;
}

static inline bool node_is_pointer_variable(Node *node) {
    return node->kind == ND_LVAR && node->num_pointers > 0;
}

static inline bool node_is_pointer_variable_many(Node *node) {
    return node->kind == ND_LVAR && node->num_pointers > 1;
}

// トークンの種類
typedef enum {
    TK_RESERVED,    // 記号
    TK_IDENT,       // 識別子
    TK_RETURN,      // return文
    TK_IF,          // if文
    TK_ELSE,        // else文
    TK_WHILE,       // while文
    TK_FOR,         // for文
    TK_NUM,         // 整数トークン
    TK_INT,         // "int"と言う名前の型
    TK_EOF,         // 入力の終わりを表すトークン
} TokenKind;

static inline const char *TokenKindDescription(TokenKind kind) {
    switch (kind) {
    case TK_RESERVED:    // 記号
        return "RESERVED";
    case TK_IDENT:       // 識別子
        return "IDENT";
    case TK_RETURN:      // return文
        return "RETURN";
    case TK_IF:          // if文
        return "IF";
    case TK_ELSE:        // else文
        return "ELSE";
    case TK_WHILE:       // while文
        return "WHILE";
    case TK_FOR:         // for文
        return "FOR";
    case TK_NUM:         // 整数トークン
        return "NUM";
    case TK_INT:         // "int"と言う名前の型
        return "INT";
    case TK_EOF:         // 入力の終わりを表すトークン
        return "EOF";
    }
    return "*Unrecoginzed*";
}

// トークンの型
typedef struct Token {
    TokenKind kind;     // トークン種別
    struct Token *next; // 次のトークン
    int val;            // tyがTK_NUMの場合、その数値
    char *str;          // トークン文字列
    int len;            // トークン文字列の長さ
    char *input;        // トークン文字列（エラーメッセージ用）
} Token;

static inline const char *TokenDescription(Token *token) {
    static char buffer[1024];
    static char tmp[1024];

    if (!token) {
        return "null";
    }

    const int n = MIN(sizeof(tmp) - 1, token->len);
    memcpy(tmp, token->str, n);
    tmp[n] = '\0';
    
    sprintf(buffer, "Token: %s, `%s`, %p",
            TokenKindDescription(token->kind),
            tmp,
            token->next);
    return buffer;
}

typedef enum {
    GEN_PUSHED_RESULT,
    GEN_DONT_PUSHED_RESULT,
} GenResult;

extern Token *token;

extern Token *tokenize(char *p);
extern void error_exit(char *fmt, ...);
extern void program();
extern GenResult gen(Node *node);
extern Node *code[];

#define D(fmt, ...) \
    fprintf(stderr, ("🐝 %s[%d] " fmt "\n"), __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)

#define D_INT(v) \
    do { D(#v "=%d", (v)); fflush(stderr); } while (0)
