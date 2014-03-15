kernel void atomic_intergroup_race(global int *data)
{
  int i = get_global_id(0);
  if (i == 0)
  {
    *data = 0;
  }
  barrier(CLK_GLOBAL_MEM_FENCE);
  atomic_inc(data);
}
