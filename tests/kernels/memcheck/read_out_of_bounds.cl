kernel void read_out_of_bounds(global int *a, global int *b, global int *c)
{
  int i = get_global_id(0);
  if (i < 4)
  {
    c[i] = a[i] + b[i];
  }
  else
  {
    c[i] = a[0] * (a[i] + b[i]);
  }
}
