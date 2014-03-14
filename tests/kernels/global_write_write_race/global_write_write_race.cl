kernel void global_write_write_race(global int *data)
{
  data[0] = get_global_id(0);
}
