void bar(int *x)
{
  *x += 1;
}

int foo()
{
  int x = 0;
  bar(&x);
  return x;
}

kernel void many_alloca(global int *data, int n)
{
  int x = 0;
  for (int i = 0; i < n; i++)
  {
    x += foo();
  }
  data[get_global_id(0)] = x;
}
