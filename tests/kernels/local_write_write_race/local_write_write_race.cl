kernel void local_write_write_race(global int *data, local int *scratch)
{
  int i = get_global_id(0);
  *scratch = i;
  barrier(CLK_LOCAL_MEM_FENCE);
  data[i] = *scratch;
}
