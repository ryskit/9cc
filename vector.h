#include <stdbool.h>

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

extern Vector *new_vec(void);
extern void vec_push(Vector *v, void *elem);
extern void vec_pushi(Vector *v, int val);
extern void *vec_pop(Vector *v);
extern void *vec_last(Vector *v);
extern bool vec_contains(Vector *v, void *elem);
extern bool vec_union1(Vector *v, void *elem);
extern bool vec_empty(Vector *v);