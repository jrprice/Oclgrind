kernel void atomic_local_fence(global int *data, local int *scratch)
{
  int i = get_global_id(0);
  int l = get_local_id(0);
  int g = get_group_id(0);
  if (l == 0)
  {
    *scratch = 0;
  }
  barrier(CLK_LOCAL_MEM_FENCE);
  atomic_add(scratch, i);
  barrier(CLK_LOCAL_MEM_FENCE);
  if (l == 0)
  {
    data[g] = *scratch;
  }
}
