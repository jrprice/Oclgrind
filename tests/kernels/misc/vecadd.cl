kernel void vecadd(global float *a, global float *b, global float *c)
{
  size_t i = get_global_id(0);
  c[i] = a[i] + b[i];
}
