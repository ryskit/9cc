typedef struct Node {
    int ty;           // 演算子かND_NUM
    struct Node *lhs; // 左辺
    struct Node *rhs; // 右辺
    int val;          // tyがND_NUMの場合のみ使う
} Node;

enum {
    ND_NUM = 256,     // 整数のノードの型
};

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_NUM,      // 整数トークン
    TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

// トークンの型
typedef struct Token {
    TokenKind kind;	// トークン種別
    struct Token *next;	// 次のトークン
    int val;     	// tyがTK_NUMの場合、その数値
    char *str;		// トークン文字列
    char *input; 	// トークン文字列（エラーメッセージ用）
} Token;

extern Token *token;

extern Token *tokenize(char *p);
extern void error_exit(char *fmt, ...);
extern Node *expr();
extern void gen(Node *node);