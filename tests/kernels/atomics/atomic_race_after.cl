kernel void atomic_race_after(global int *data, global int *output)
{
  atomic_inc(data);
  if (get_global_id(0) == get_global_size(0)-1)
  {
    *output = *data;
  }
}
