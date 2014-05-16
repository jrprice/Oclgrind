kernel void async_copy_divergent(global int *data, local int *scratch)
{
  int i = get_local_id(0);
  size_t size = get_local_size(0);
  if (i == size-1)
  {
    size = 1;
  }

  event_t event = async_work_group_copy(scratch, data, size, 0);
  wait_group_events(1, &event);

  data[get_local_size(0)-i-1] = scratch[i];
}
