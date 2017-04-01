typedef struct {
  int a;
  int b;
  int c;
} S;

S foo(S a) {
  return a;
}

kernel void llvm_memcpyopt_bug(global S *out)
{
  S a = {7,7,7};
  out[0] = foo(a);
}
