#!/bin/bash

try() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s extern/foo.o extern/alloc4.o extern/alloc_ptr3.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "💮 $input => $actual"
  else
    echo "❎ $expected expected, but got $actual"
    exit 1
  fi
}

#try 0 '0;'
#try 42 '42;'
#try 21 '5+20-4;'
#try 41 ' 12 + 34 - 5 ;'
#try 47 "5+6*7;"
#try 15 "5*(9-6);"
#try 4 "(3+5)/2;"
#try 0 '-1+1;'
#try 2 '+1*2;'
#try 1 '9>3;'
#try 0 '4>7;'
#try 1 '9>=3;'
#try 1 '9>=9;'
#try 0 '4>=7;'
#try 1 '0<1;'
#try 0 '1<0;'
#try 1 '4<=9;'
#try 1 '4<=4;'
#try 0 '4<=1;'
#try 1 '0 == 0;'
#try 0 '0 == 1;'
#try 0 '5 != 5;'
#try 1 '3 != 9;'
#try 99 'a = 99;'
#try 7 'foo = 4; bar = 7;'
#try 14 'a = 3; b = 5 * 6 - 8; return a + b / 2;'
#try 0 'foo = 4; return 0; bar = 7;'
#try 7 'if (1 == 1) 7;' # ブロックが未サポートなので1文だけ
#try 0 'if (1 == 0) 7; else 0;' # 2文以上書けるが実行がおかしい
#try 4 '{ 3; 4; }' # ブロックは2を返す
#try 9 '{ 1; 2; return 9; }'
#try 3 '{ x = 1; y = 2; return x + y; }'
#try 3 'if (1 == 1) { return 3; } else { return 6; }'
#try 6 'if (0 == 1) { return 3; } else { return 6; }'
#try 2 'if (0 == 1) { return 3; } else { a = 1; b = 0; return (a + b) * 2; }'
#try 0 'i = 0; while (0) i = 1; return i;'
#try 5 'i = 0; while (i < 5) i = i + 1; return i;'
#try 8 'x = 2; for (i = 0; i < 2; i = i + 1) x = x * 2; x;'
#try 10 'x = 0; for (i = 0; i < 10; i = i + 1) { x = x + 1; } x;'
#try 0 'foo();'
#try 0 'foo(7);'
#try 0 'foo(7, 8, 9, 10, 11);'
#try 0 'i = 9; j = 99; foo(i, j);'
#try 0 'bar(i, j) { return i * j; }'
#
# ここから
try 7 '
int fun() {
	return 7;
}
int main() {
	int i;
	i = fun();
	return i;
}
'
try 3 '
int fun() {
	return 1 + 1;
}
int main() {
	return fun() + 1;
}
'
try 12 '
int fun(int i) {
	return i + 3;
}
int main() {
	return fun(9);
}
'
try 55 '
int fib(int i) {
	if (i < 2) {
		return i;
	} else {
		int j;
		j	= i - 2;
		int k;
		k = i - 1;
		return fib(j) + fib(k);
	}
}
int main() {
	return fib(10);
}
'
try 28 '
int main() {
	int i;
	i = 28;
	int a;
	a = &i;
	return *a;
}
'
try 111 '
int fun(int a, int b, int c) {
	return a + b + 1;
}
int main() {
	return fun(10, 100);
}
'
try 3 '
int main() {
	int x;
	int *y;
	int **z;
	y = &x;
	z = &y;
	**z = 3;
	return x;
}
'
try 8 '
int main() {
	int *p;
	alloc4(&p, 1, 2, 4, 8);
	int *q;
	q = p + 3;
	return *q;
}
'
try 2 '
int main() {
	int *p;
	alloc4(&p, 1, 2, 4, 8);
	int *q;
	q = p + 3;
	q = q - 2;
	return *q;
}
'
try 9 '
int main() {
	int x;
	int y;
	int z;
	int **p;
	int **q;
	int *r;

	x = 3;
	y = 6;
	z = 9;
	alloc_ptr3(&p, &x, &y, &z);

	q = p + 2;
	r = *q;
	return *r;
}
'

echo DONE