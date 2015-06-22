struct S
{
  char a;
  int  b;
  char c;
};

kernel void padded_struct_memcpy_fp(local struct S *scratch,
                                    global struct S *output)
{
  int lid = get_local_id(0);

  struct S s;
  s.a = 42;
  s.b = -7;
  s.c = 255;

  if (lid == 0)
  {
    *scratch = s;
  }
  barrier(CLK_LOCAL_MEM_FENCE);
  if (lid == 1)
  {
    *output = *scratch;
  }
}
