#include "9cc.h"
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

// 現在着目しているトークン
Token *token;

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

bool consume(char* op) {
  if (token->kind == TK_RESERVED &&
      strlen(op) == token->len &&
      memcmp(token->str, op, token->len) == 0) {
    token = token->next;
    return true;
  }
  return false;
}

Token* consume_ident() {
  if (token->kind == TK_IDENT) {
    Token* t = token;
    token = token->next;
    return t;
  }
  return NULL;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    error_exit("'%c'ではありません", op);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
  if (token->kind != TK_NUM)
    error_exit("数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

// 前方宣言
Node *code[100];

Node *stmt();

void program() {
  int i = 0;
  while (!at_eof())
    code[i++] = stmt();
  code[i] = NULL;
}

Node *expr();

Node *stmt() {
  Node *node = expr();
  expect(";");
  return node;
}

Node *assign();

Node *expr() {
  return assign();
}

Node *equality();

Node *assign() {
  Node * node = equality();

  if (consume("="))
    node = new_node(ND_ASSIGN, node, equality());

  return node;
}

Node *relational();

Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQUAL, node, relational());
    else if (consume("!="))
      node = new_node(ND_NOT_EQUAL, node, relational());
    else
      return node;
  }
}

Node *add();

Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_node(ND_GREATER, node, add());
    else if (consume("<="))
      node = new_node(ND_GREATER_EQUAL, node, add());
    else if (consume(">")) {
      node = new_node(ND_GREATER, node, add());
      Node *tmp = node->lhs;
      node->lhs = node->rhs;
      node->rhs = tmp;
    } else if (consume(">=")) {
      node = new_node(ND_GREATER_EQUAL, node, add());
      Node *tmp = node->lhs;
      node->lhs = node->rhs;
      node->rhs = tmp;
    } else
      return node;
  }
}

Node *mul();

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

// 前方宣言
Node *unary();

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

Node *term() {
  // 次のトークンが'('なら、"(" expr ")"のはず
  if (consume("(")) {
    Node *node = expr();
    expect(')');
    return node;
  }

  // ローカル変数
  Token* token = consume_ident();
  if (token != NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    node->offset = (token->str[0] - 'a' + 1) * 8;
    return node;
  }

  // そうでなければ数値のはず
  return new_node_num(expect_number());
}

Node *unary() {
  if (consume("+"))
    return term();
  if (consume("-"))
    return new_node(ND_SUB, new_node_num(0), term());
  return term();
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    if ('a' <= *p && *p <= 'z') {
      cur = new_token(TK_IDENT, cur, p++, 1);
      cur->len = 1;
      continue;
    }

    char *q = p + 1;
    if (*q) {
      if (*p == '>' || *p == '<') {
        if (*q == '=') {
          cur = new_token(TK_RESERVED, cur, p, 2);
          p += 2;
        } else {
          cur = new_token(TK_RESERVED, cur, p, 1);
          p++;
        }
        continue;
      }
      if (strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0) {
        cur = new_token(TK_RESERVED, cur, p, 2);
        p += 2;
        continue;
      }
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '=') {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 1);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_exit("トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 1);
  return head.next;
}