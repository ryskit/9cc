#include "9cc.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// 現在のパース位置がトップレベルかどうか
static int nest_level = 0;

// 型
typedef struct Type Type;
struct Type {
    enum { INT, PTR } type; // 型の種別
    Type *ptr_to;           // typeがPTRの時だけ有効
    int num_pointers;       // ポインタの数
};

// ローカル変数の型
typedef struct LVar LVar;
struct LVar {
    LVar *next; // 次の変数かNULL
    char *name; // 変数の名前
    int len;    // 名前の長さ
    int offset; // RBPからのオフセット
    Type *type;  // 型情報
};

LVar *locals;   // ローカル変数のリストの先頭

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_lvar(Token *token) {
    for (LVar *var = locals; var; var = var->next)
        if (var->len == token->len && memcmp(token->str, var->name, var->len) == 0)
            return var;
    return NULL;
}

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

bool consume_and_next(char* op, bool must_to_next) {
    if (token->kind == TK_RESERVED &&
        strlen(op) == token->len &&
        memcmp(token->str, op, token->len) == 0) {
        if (must_to_next) {
            token = token->next;
        }
        return true;
    }
    return false;
}


bool consume(char* op) {
    return consume_and_next(op, true);
}

Token* consume_by_kind(TokenKind kind) {
    if (token->kind == kind) {
        Token* t = token;
        token = token->next;
        return t;
    }
    return NULL;
}

Token* consume_ident() {
    return consume_by_kind(TK_IDENT);
}

Node *reference_local_var(Token* t) {
    Node *node = new_node(ND_LVAR, NULL, NULL);
    LVar *lvar = find_lvar(t);
    if (lvar == NULL) {
        error_exit("変数が定義されていません: %s\n", t->str);
    }
    node->offset = lvar->offset;
    node->ident = lvar->name;
    node->identLength = lvar->len;

    Type *type_root = lvar->type;
    node->num_pointers = (type_root && type_root->type == PTR) ? type_root->num_pointers : 0;
    return node;
}

Node *define_local_var() {
    // （連続する）ポインタ修飾をパースする
    Type *type_root = NULL;
    Type *type_current = NULL;
    int num_pointers = 0;
    while (consume("*")) {
        Type *ti = calloc(1, sizeof(Type));
        ti->type = PTR;
        if (!type_root) {
            type_root = ti;
        }
        if (type_current) {
            type_current->ptr_to = ti;
        }
        type_current = ti;
        type_root->num_pointers += 1;
    }

    Token *t = consume_ident();
    if (!t) {
        error_exit("識別子がありません: %s\n", token_description(token));
    }

    LVar *lvar = find_lvar(t);
    if (lvar) {
        error_exit("同名の変数が定義されています: %s\n", t->str);
    }
    Node *node = new_node(ND_LVAR, NULL, NULL);
    lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = t->str;
    lvar->len = t->len;
    lvar->offset = (locals == NULL ? 0 : locals->offset) + 8;
    lvar->type = type_root;

    locals = lvar;

    node->offset = lvar->offset;
    node->ident = lvar->name;
    node->identLength = lvar->len;
    node->num_pointers = (type_root && type_root->type == PTR) ? type_root->num_pointers : 0;

    return node;
}

bool is_reserved_with(Token *token, char c) {
    return token->kind == TK_RESERVED && token->str[0] == c;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char op) {
    if (!is_reserved_with(token, op))
        error_exit("'%c'ではありません", op);
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM)
        error_exit("数ではありません: %s", token_description(token));
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    if (!token) {
        return false;
    }
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

Node *relational() {
    Node * node = add();

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
        }
        else if (consume(">=")) {
            node = new_node(ND_GREATER_EQUAL, node, add());
            Node *tmp = node->lhs;
            node->lhs = node->rhs;
            node->rhs = tmp;
        }
        else
            return node;
    }
}

Node *equality() {
    Node * node = relational();

    for (;;) {
        if (consume("=="))
            node = new_node(ND_EQUAL, node, relational());
        else if (consume("!="))
            node = new_node(ND_NOT_EQUAL, node, relational());
        else
            return node;
    }
}

Node *assign() {
    Node * node = equality();
    if (consume("="))
        node = new_node(ND_ASSIGN, node, equality());
    return node;
}

Node *expr() {
    return assign();
}

Token* peek(TokenKind kind) {
    for (Token* t = token; t && t->kind != TK_EOF; t = t->next) {
        if (t->kind == kind) {
            return t;
        }
    }
    return NULL;
}

Token* equal(Token* t, TokenKind kind, const char *str) {
    if (t->kind == kind && memcmp(t->str, str, strlen(str)) == 0) {
        return t;
    }
    return NULL;
}

Node *term();

Node *calling_function(Token *indentifier) {
    if (indentifier) {
        assert(token == indentifier->next);
        // `()`を先読みしてあれば関数ノードを作成する
        Token* open_paren = equal(indentifier->next, TK_RESERVED, "(");
        if (open_paren) {
            Token *close_paren = NULL;
            Vector *args = new_vec();
            for (Token* at = open_paren->next; at; at = at->next) {
                if (token != at) {
                    token = at;
                }
                close_paren = equal(at, TK_RESERVED, ")"); // peek
                if (close_paren) {
                    break;
                }
                if (equal(at, TK_RESERVED, ",")) { // peek
                    continue;
                }
                Node* node = term();
                vec_push(args, node);
                at = token;
                // termでtokenが進んでしまうのでここでもう一度")"をチェックしないと...
                close_paren = equal(at, TK_RESERVED, ")"); // peek
                if (close_paren) {
                    break;
                }
            }
            token = close_paren->next;
            Node *node = new_node(ND_FUN, NULL, NULL);
            node->ident = indentifier->str;
            node->identLength = indentifier->len;
            node->block = args;
            return node;
        }
    }
    return NULL;
}

Node *stmt();

Node *define_function(Token *indentifier) {
    if (indentifier) {
        // `()`を先読みしてあれば関数定義ノードを作成する
        Token* openParen = equal(indentifier->next, TK_RESERVED, "(");
        if (openParen) {
            Token *closeParen = NULL;
            Vector *args = new_vec();
            int type_declared = 0;
            for (Token* at = openParen->next; at; at = at->next) {
                if (at->kind == TK_INT) {
                    // 引数の型は単なる読み捨て
                    type_declared = 1;
                } else if (is_reserved_with(at, '*') || at->kind == TK_IDENT) {
                    if (!type_declared)
                        error_exit("関数定義シンタックスエラー: %s\n", at->str);
                    type_declared = 0;
                    token = at; // Ad-Hocすぎる...
                    vec_push(args, define_local_var());
                } else {
                    type_declared = 0;
                    closeParen = equal(at, TK_RESERVED, ")");
                    if (closeParen) {
                        break;
                    } else if (equal(at, TK_RESERVED, ",") == NULL) {
                        error_exit("関数定義シンタックスエラー: %s\n", at->str);
                    }
                }
            }
            token = closeParen->next; // Ad-Hocすぎる...
            Node *node = new_node(ND_FUN_IMPL, NULL, NULL);
            node->ident = indentifier->str;
            node->identLength = indentifier->len;
            node->block = args;

            // 次がブロックかどうか
            if (!consume_and_next("{", false)) {
                error_exit("関数定義シンタックスエラー: %s\n", token->str);
            }

            node->lhs = stmt();
            return node;
        }
    }
    return NULL;
}

Node *stmt() {
    nest_level++;

    Node *node;
    if (consume_by_kind(TK_IF)) { // if
        node = new_node(ND_IF, NULL, NULL);
        node->condition = expr();
        node->lhs = stmt();
        Token* maybe_else = peek(TK_ELSE);
        if (maybe_else) { // else
            token = maybe_else->next; // elseトークンをスキップ: consume関数がやってること
            node->rhs = stmt();
        }
        return node;
    } else if (consume_by_kind(TK_WHILE)) { // while
        node = new_node(ND_WHILE, NULL, NULL);
        node->condition = expr();
        node->lhs = stmt();
        return node;
    } else if (consume_by_kind(TK_FOR)) { // for
        if (consume("(")) {
            Vector *v = new_vec();
            Node * e = NULL;

            e = expr();
            vec_push(v, e);
            if (!consume(";")) {
                error_exit("';'ではないトークンです: %d %s", token->kind, token->str);
            }
            e = expr();
            vec_push(v, e);
            if (!consume(";")) {
                error_exit("';'ではないトークンです: %d %s", token->kind, token->str);
            }
            e = expr();
            vec_push(v, e);
            if (!consume(")")) {
                error_exit("')'ではないトークンです: %d %s", token->kind, token->str);
            }

            node = new_node(ND_FOR, NULL, NULL);
            node->block = v;
            node->lhs = stmt();
        }
        return node;
    } else if (consume("{")) { // ブロック
        Vector *vec = new_vec();
        do {
            vec_push(vec, stmt());
        } while (!consume("}"));
        node = new_node(ND_BLOCK, NULL, NULL);
        node->block = vec;
        return node; // ここでreturnするので文末の';'は不要
    } else if (consume_by_kind(TK_RETURN)) {
        node = new_node(ND_RETURN, expr(), NULL);
    } else if (consume_by_kind(TK_INT)) {
        if (nest_level == 1) {
            // 戻り値としてのintなので関数定義としてパースする
            return define_function(consume_ident());
        }
        else {
            // ローカル変数定義としてパースする
            node = define_local_var();
        }
    } else {
        node = expr();
    }

    if (!consume(";")) {
        error_exit("';'ではないトークンです: %d %s", token->kind, token->str);
    }

    return node;
}

// 文を格納する配列
Node *code[100];
static int statement_index = 0;

void program() {
    statement_index = 0;
    while (!at_eof()) {
        nest_level = 0;
        Node *node = stmt();
        code[statement_index] = node;
        statement_index++;
    }
    code[statement_index] = NULL;
}

// 前方宣言
Node *unary();
Node *pointer();

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

    // 識別子
    Token *t = consume_ident();
    if (t) {
        // 関数呼び出しノード
        Node *node = calling_function(t);
        if (!node) {
            // ローカル変数
            node = reference_local_var(t);
        }
        return node;
    }

    // ポインタ
    Node *node = pointer();
    if (node) {
        return node;
    }

    // そうでなければ数値のはず
    return new_node_num(expect_number());
}

// 構文木に紐づいている型がintならば4、ポインタならば8を返す
int sizeof_ast(Node *node) {
    if (!node) {
        return 0;
    }
    if (node->kind == ND_LVAR) {
        return node->num_pointers > 0 ? 8 : 4;
    }
    if (node->kind == ND_DEREF || node->kind == ND_NUM) {
        return 4;
    }
    if (node->kind == ND_ADDR) {
        return 8;
    }
    int l = sizeof_ast(node->lhs);
    int r = sizeof_ast(node->rhs);
    return l > r ? l : r;
}

Node *unary() {
    if (consume_by_kind(TK_SIZEOF)) {
        Node *node = term();
        int s = sizeof_ast(node);
        assert(s == 4 || s == 8);
        return new_node_num(s);
    }
    if (consume("+"))
        return term();
    if (consume("-"))
        return new_node(ND_SUB, new_node_num(0), term());
    return term();
}

Node *pointer() {
    if (consume("&")) {
        // オペランドについてruiさんの文書ではlhsだが他の演算子との整合性を考慮
        // してrhsにする
        return new_node(ND_ADDR, NULL, unary());
    } else if (consume("*")) {
        Node *node = unary();
        if (node->kind == ND_LVAR) {
            node->num_pointers = 1; // TODO: あれ？これ必要だっけ？
        }
        return new_node(ND_DEREF, NULL, node);
    }
    return NULL;
}

int is_alnum(char c) {
    return ('a' <= c && c <= 'z') ||
        ('A' <= c && c <= 'Z') ||
        ('0' <= c && c <= '9') ||
        (c == '_');
}

// 入力文字列pをトークナイズしてそれを返す
Token* tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        // return文
        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_RETURN, cur, p, 6);
            p += 6;
            continue;
        }

        // if文
        if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
            cur = new_token(TK_IF, cur, p, 2);
            p += 2;
            continue;
        }

        // else文
        if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
            cur = new_token(TK_ELSE, cur, p, 4);
            p += 4;
            continue;
        }

        // while文
        if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
            cur = new_token(TK_WHILE, cur, p, 5);
            p += 5;
            continue;
        }

        // for文
        if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
            cur = new_token(TK_FOR, cur, p, 3);
            p += 3;
            continue;
        }

        // int型
        if (strncmp(p, "int", 3) == 0 && !is_alnum(p[3])) {
            cur = new_token(TK_INT, cur, p, 3);
            p += 3;
            continue;
        }

        // sizeof演算子
        if (strncmp(p, "sizeof", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_SIZEOF, cur, p, 6);
            p += 6;
            continue;
        }

        // 関係演算子
        char *q = p + 1;
        if (*q) {
            if (*p == '>' || *p == '<') {
                if (*q == '=') {
                    cur = new_token(TK_RESERVED, cur, p++, 2);
                } else {
                    cur = new_token(TK_RESERVED, cur, p, 1);
                }
                p++;
                continue;
            }
            if (strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0) {
                cur = new_token(TK_RESERVED, cur, p, 2);
                p += 2;
                continue;
            }
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '=' || *p == ';' || *p == '{' || *p == '}' || *p == ',' || *p == '&') {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        // 数値
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 1);
            char *q = p;
            cur->val = strtol(p, &q, 10);
            cur->len = q - p;
            p = q;
            continue;
        }

        // ローカル変数
        char *s = p;
        while (('a' <= *s && *s <= 'z') ||
               ('0' <= *s && *s <= '9') ||
               ('_' == *s)) {
            s++;
        }
        if (s != p) {
            const int length = s - p;
            cur = new_token(TK_IDENT, cur, p, length);
            cur->len = length;
            p = s;
            continue;
        }

        error_exit("トークナイズできません");
    }

    new_token(TK_EOF, cur, p, 1);
    return head.next;
}