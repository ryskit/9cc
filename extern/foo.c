#include <stdio.h>

extern int foo(int a1, int a2, int a3) {
  printf("hello! from external function. a1=%d a2=%d a3=%d\n", a1, a2, a3);
  return a1;
}