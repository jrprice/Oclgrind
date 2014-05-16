kernel void local_only_fence(global int *scratch, global int *output)
{
  int i = get_global_id(0);
  int g = get_group_id(0);
  scratch[i] = i;
  barrier(CLK_LOCAL_MEM_FENCE);
  if (get_local_id(0) == 0)
  {
    int x = 0;
    for (int l = 0; l < get_local_size(0); l++)
    {
      x += scratch[get_local_size(0)*g + l];
    }
    output[g] = x;
  }
}
