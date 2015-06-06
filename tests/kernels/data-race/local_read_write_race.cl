kernel void local_read_write_race(global int *data, local int *scratch)
{
  int l = get_local_id(0);
  scratch[l] = 0;
  barrier(CLK_LOCAL_MEM_FENCE);

  scratch[l] = l;
  if (l == 0)
  {
    int x = 0;
    for (int i = 0; i < get_local_size(0); i++)
    {
      x += scratch[i];
    }
    *data = x;
  }
}
