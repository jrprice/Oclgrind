kernel void atomic_cmpxchg_read_race(global int *data)
{
  int i = get_global_id(0);
  if (i == 0)
  {
    *data = 0;
  }
  else
  {
    atomic_cmpxchg(data, 0, 0x01000001);
  }
}
