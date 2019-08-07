typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // 整数
  ND_GREATER, // >
  ND_GREATER_EQUAL, // >=
  ND_EQUAL, // ==
  ND_NOT_EQUAL, //
  ND_ASSIGN, // =
  ND_LVAR, // ローカル変数
} NodeKind;

typedef struct Node {
  NodeKind kind;
  struct Node *lhs; // 左辺
  struct Node *rhs; // 右辺
  int val;          // tyがND_NUMの場合のみ使う
  int offset;    // kindがND_LVARの場合のみ使う
} Node;

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_IDENT,    // 識別子
  TK_NUM,      // 整数トークン
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

// トークンの型
typedef struct Token {
  TokenKind kind;    // トークン種別
  struct Token *next;    // 次のトークン
  int val;        // tyがTK_NUMの場合、その数値
  char *str;        // トークン文字列
  char *input;    // トークン文字列（エラーメッセージ用）
  int len;        // トークンの長さ
} Token;

extern Token *token;

extern Token *tokenize(char *p);

extern void error_exit(char *fmt, ...);

extern void program();

extern void gen(Node *node);

extern Node *code[];
