__kernel void gvn_arbitrary_integers(__global int *source,
                                     __global int *dest)
{
  size_t i = get_global_id(0);
  int3 tmp = 0;
  tmp.S2 = source[i];
  vstore3(tmp, 0, dest);
}
