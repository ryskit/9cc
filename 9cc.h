enum {
  TK_NUM = 256,
  TK_EOF,
};

enum {
  ND_NUM = 256,     // 整数のノードの型
};

typedef struct Node {
  int ty;           // 演算子かND_NUM
  struct Node *lhs; // 左辺
  struct Node *rhs; // 右辺
  int val;          // tyがND_NUMの場合のみ使う
} Node;

extern char *user_input;
extern void tokenize();
extern void error_at(char *loc, char *msg);
extern Node *expr();
extern void gen(Node *node);

