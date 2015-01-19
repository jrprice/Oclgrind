kernel void intragroup_hidden_race(global int *data, global int *output)
{
  int id = get_local_id(0);
  output[id] = data[0];
  barrier(CLK_LOCAL_MEM_FENCE);
  if (id == 0)
  {
    data[0] = -1;
  }
}
