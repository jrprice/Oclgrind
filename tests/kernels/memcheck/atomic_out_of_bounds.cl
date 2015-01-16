kernel void atomic_out_of_bounds(global int *counters)
{
  int i = get_global_id(0);
  atomic_inc(counters+i);
}
