kernel void write_out_of_bounds(global int *a, global int *b, global int *c)
{
  int i = get_global_id(0);
  c[i] = a[i] + b[i];
}
