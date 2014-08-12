struct __attribute__((packed)) Foo
{
  char a;
  int b;
};

kernel void packed(global int *out)
{
  struct Foo x = {1, 2};
  *out = x.b;
}
