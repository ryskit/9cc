#include "vector.h"
#include <stdlib.h>
#include <assert.h>

const int INITIAL_CAPACITY = 16;

Vector *new_vec() {
  Vector *v = malloc(sizeof(Vector));
  v->data = malloc(sizeof(void *) * INITIAL_CAPACITY);
  v->capacity = INITIAL_CAPACITY;
  v->len = 0;
  return v;
}

void vec_push(Vector *v, void *elem) {
  if (v->len == v->capacity) {
    v->capacity *= 2;
    v->data = realloc(v->data, sizeof(void *) * v->capacity);
  }
  v->data[v->len++] = elem;
}

void vec_pushi(Vector *v, int val) {
  vec_push(v, (void *)(intptr_t)val);
}

void *vec_pop(Vector *v) {
  assert(v->len);
  return v->data[--v->len];
}

void *vec_last(Vector *v) {
  assert(v->len);
  return v->data[v->len - 1];
}

bool vec_contains(Vector *v, void *elem) {
  for (int i = 0; i < v->len; i++)
    if (v->data[i] == elem)
      return true;
  return false;
}

bool vec_union1(Vector *v, void *elem) {
  if (vec_contains(v, elem))
    return false;
  vec_push(v, elem);
  return true;
}

bool vec_empty(Vector *v) {
  return v->len == 0;
}