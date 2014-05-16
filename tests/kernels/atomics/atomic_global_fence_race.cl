kernel void atomic_global_fence_race(global int *data, global int *scratch)
{
  int i = get_global_id(0);
  int l = get_local_id(0);
  int g = get_group_id(0);
  atomic_add(scratch, i);
  barrier(CLK_GLOBAL_MEM_FENCE);
  if (l == 0)
  {
    data[g] = *scratch;
  }
}
