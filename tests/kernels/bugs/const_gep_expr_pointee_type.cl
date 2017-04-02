#pragma clang diagnostic ignored "-Wunused-value"

struct S0 {
  int d;
  long b;
} fn1() {
  struct S0 a = {3};
  a.d;
  return a;
  }
__kernel void entry() { fn1(); }
