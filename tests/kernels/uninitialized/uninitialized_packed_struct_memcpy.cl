struct __attribute__ ((packed)) S
{
  char a;
  int  b __attribute__ ((packed));
  char c;
};

kernel void uninitialized_packed_struct_memcpy(local int *scratch, global struct S *output)
{
  struct S s = {1, *scratch, 2};
  *output = s;
}
