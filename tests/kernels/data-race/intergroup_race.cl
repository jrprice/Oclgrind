kernel void intergroup_race(global int *data)
{
  int g = get_group_id(0);
  if (get_local_id(0) == 0)
  {
    data[g] = g;
  }
  barrier(CLK_GLOBAL_MEM_FENCE);
  if (get_global_id(0) == 0)
  {
    int x = 0;
    for (int i = 0; i < get_num_groups(0); i++)
    {
      x += data[i];
    }
    data[0] = x;
  }
  barrier(CLK_GLOBAL_MEM_FENCE);
}
