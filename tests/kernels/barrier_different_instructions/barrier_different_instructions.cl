kernel void barrier_different_instructions(global int *data)
{
  int i = get_global_id(0);
  if (i != 0)
  {
    barrier(CLK_GLOBAL_MEM_FENCE);
  }
  else
  {
    barrier(CLK_GLOBAL_MEM_FENCE);
  }
  data[i] = i;
}
