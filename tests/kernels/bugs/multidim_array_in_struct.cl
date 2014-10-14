//
// Issue #64 on GitHub:
// https://github.com/jrprice/Oclgrind/issues/64
//
// Required alignment for multi-dimensional arrays was incorrect.
//

struct S0
{
  uchar a;
  ulong b[2][3][1];
};

kernel void multidim_array_in_struct(global ulong *output)
{
  struct S0 s =
  {
    1UL,
    {
      {
        {1L},
        {1L},
        {1L}
      },
      {
        {1L},
        {1L},
        {1L}
      }
    },
  };

  ulong c = 0UL;
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 3; j++)
      for (int k = 0; k < 1; k++)
        c += s.b[i][j][k];

  *output = c;
}
