struct S
{
  char a;
  int  b;
  char c;
};

kernel void padded_struct_alloca_fp(global struct S *output)
{
  struct S s;
  s.a = 42;
  s.b = 0xF9FFFFF9;
  s.c = 127;

  *output = s;
}
