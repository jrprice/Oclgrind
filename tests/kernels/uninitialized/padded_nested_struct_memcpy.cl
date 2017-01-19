struct T
{
  char a;
  int  b;
  char c;
};

struct S
{
  char a;
  int  b;
  char c;
  struct T d;
};

kernel void padded_nested_struct_memcpy(global struct S *output)
{
  struct S s;
  s.a = 1;
  s.b = 0x02000002;
  s.c = 3;
  s.d.a = 4;
  s.d.b = 0x05000005;
  s.d.c = 6;

  *output = s;
}
