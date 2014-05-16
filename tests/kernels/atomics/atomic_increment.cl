kernel void atomic_increment(global int *data)
{
  atomic_inc(data);
}
