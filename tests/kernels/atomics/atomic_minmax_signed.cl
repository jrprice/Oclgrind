kernel void atomic_minmax_signed(global int *data)
{
  atomic_min(data+0, -8);
  atomic_min(data+1, -6);
  atomic_min(data+2, 3);
  atomic_min(data+3, -3);
  atomic_min(data+4, 6);
  atomic_min(data+5, 8);

  atomic_max(data+6, -8);
  atomic_max(data+7, -6);
  atomic_max(data+8, 3);
  atomic_max(data+9, -3);
  atomic_max(data+10, 6);
  atomic_max(data+11, 8);
}
