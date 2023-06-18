struct S
{
  int a;
  int b;
  int c;
  int d;
};

kernel void pointers(global int *gi, global char *gc,
                     local int *li, constant int *ci)
{
  int i = -7;
  private int *pi = &i;

  *li = 37;

  struct S s = {-1, 231, 123, -1};
  private struct S *ps = &s;
  private int *psm = &s.c;
}
