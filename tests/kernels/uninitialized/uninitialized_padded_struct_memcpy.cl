struct S
{
  char a;
  int  b;
  char c;
};

kernel void uninitialized_padded_struct_memcpy(global struct S *output)
{
  struct S s;
  *output = s;
}
