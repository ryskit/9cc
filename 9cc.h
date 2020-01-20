#include "vector.h"

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
  ND_NUM, // 整数
} NodeKind;

static inline const char* NodeKindDescription(NodeKind kind) {
  static const char* description[] = {
    "ND_ADD", // +
    "ND_SUB", // -
    "ND_MUL", // *
    "ND_DIV", // /
    "ND_GREATER", // >
    "ND_GREATER_EQUAL", // >=
    "ND_EQUAL", // ==
    "ND_NOT_EQUAL", // !=
    "ND_ASSIGN",  // =
    "ND_RETURN", // return
    "ND_IF", // if
    "ND_ELSE", // else
    "ND_WHILE", // while
    "ND_FOR", // for
    "ND_LVAR",    // ローカル変数
    "ND_BLOCK",    // ブロック
    "ND_FUN", // 関数
    "ND_NUM", // 整数
  };
  return description[kind];
}

typedef struct Node {
  NodeKind kind;      // 演算子かND_NUM
  struct Node *lhs;   // 左辺
  struct Node *rhs;   // 右辺
  struct Node *condition; // 条件(ifの場合のみ)
  Vector *block;   // ブロック
  int val;            // kindがND_NUMの場合のみ使う
  char *ident;    // kindがND_FUNの婆愛のみ使う(関数名)
  int identLength;    // 上記の長さ
  int offset;         // kindがND_LVARの場合のみ使う
} Node;

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
  TK_EOF,         // 入力の終わりを表すトークン
} TokenKind;

// トークンの型
typedef struct Token {
  TokenKind kind;     // トークン種別
  struct Token *next; // 次のトークン
  int val;            // tyがTK_NUMの場合、その数値
  char *str;          // トークン文字列
  int len;            // トークン文字列の長さ
  char *input;        // トークン文字列（エラーメッセージ用）
} Token;

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