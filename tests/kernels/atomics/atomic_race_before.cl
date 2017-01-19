kernel void atomic_race_before(global int *data)
{
  if (get_global_id(0) == 0)
  {
    *data = 0;
  }
  atomic_dec(data);
}
