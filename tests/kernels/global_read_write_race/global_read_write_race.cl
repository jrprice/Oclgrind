kernel void global_read_write_race(global int *data)
{
  int i = get_global_id(0);
  if (i > 0)
  {
    data[i] = data[i-1];
  }
}
