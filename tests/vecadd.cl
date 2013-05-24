__kernel void vecadd(__global float *a,
                     __global float *b,
                     __global float *c)
{
  int i = get_global_id(0);
  c[i] = a[i] + b[i];
}

__kernel void vecadd_guarded(__global float *a,
                             __global float *b,
                             __global float *c,
                             int n)
{
  int i = get_global_id(0);
  if (i >= n)
  {
    return;
  }

  c[i] = a[i] + b[i];
}
