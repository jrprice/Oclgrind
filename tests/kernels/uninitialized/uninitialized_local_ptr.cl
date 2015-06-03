kernel void uninitialized_local_ptr(local float *scratch, global float *output)
{
  int i = get_local_id(0);
  if (i != get_local_size(0)/2)
  {
    scratch[i] = i;
  }
  output[i] = scratch[i];
}
