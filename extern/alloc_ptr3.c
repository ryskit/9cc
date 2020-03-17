#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef unsigned long long T; // レジスタの型 = 8byte

#define N (3)
static int* buffer[N];

extern T alloc_ptr3(int ***r, int *a, int *b, int *c) {
    assert(sizeof(int ***) == sizeof(T));
    buffer[0] = a;
    buffer[1] = b;
    buffer[2] = c;
    //fprintf(stderr, ">> [%d, %d, %d, %d]\n", buffer[0], buffer[1], buffer[2], buffer[3]);
    *r = buffer;
    return N;
}