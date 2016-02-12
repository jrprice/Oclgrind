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

kernel void uninitialized_padded_nested_struct_memcpy(local int *scratch, global struct S *output)
{
  struct S s = {1, 2, 3, {4, *scratch, 5}};
  *output = s;
}
