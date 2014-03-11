kernel void vecadd(global int *a, global int *b, global int *c)
{
  int i = get_global_id(0);
  c[i] = a[i] + b[i];
}
