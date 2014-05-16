kernel void atomic_same_workitem(global int *data)
{
  int i = get_global_id(0);
  if ((i % 2) == 0)
  {
    data[i] = 0;
    atomic_inc(data+i);
  }
  else
  {
    atomic_inc(data+i);
    data[i] = data[i] + 1;
  }
}
