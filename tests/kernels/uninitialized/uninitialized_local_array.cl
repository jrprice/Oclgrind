kernel void uninitialized_local_array(global float *output)
{
  local float scratch[16];

  int i = get_local_id(0);
  if (i != get_local_size(0)/2)
  {
    scratch[i] = i;
  }
  output[i] = scratch[i];
}
