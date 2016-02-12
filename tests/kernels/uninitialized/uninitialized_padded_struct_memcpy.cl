struct S
{
  char a;
  int  b;
  char c;
};

kernel void uninitialized_padded_struct_memcpy(local int *scratch, global struct S *output)
{
  struct S s = {1, *scratch, 2};
  *output = s;
}
