kernel void async_copy_single_wi(global int *data, local int *scratch)
{
  int i = get_local_id(0);
  event_t event = async_work_group_copy(scratch, data, get_local_size(0), 0);
  if (i == 0)
  {
    // An extra copy that will only be registered by one work-item
    event = async_work_group_copy(scratch, data, 1, event);
  }
  wait_group_events(1, &event);

  data[get_local_size(0)-i-1] = scratch[i];
}
