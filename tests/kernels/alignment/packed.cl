struct __attribute__((packed)) Foo
{
  char a;
  int b;
};

kernel void packed(struct Foo x, global int *out)
{
  *out = x.b;
}
