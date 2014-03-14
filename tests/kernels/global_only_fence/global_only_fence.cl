kernel void global_only_fence(local int *scratch, global int *output)
{
  int l = get_local_id(0);
  int g = get_group_id(0);
  scratch[l] = l;
  barrier(CLK_GLOBAL_MEM_FENCE);
  if (get_local_id(0) == 0)
  {
    int x = 0;
    for (int i = 0; i < get_local_size(0); i++)
    {
      x += scratch[i];
    }
    output[g] = x;
  }
}
