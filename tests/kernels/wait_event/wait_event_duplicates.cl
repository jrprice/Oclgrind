kernel void wait_event_duplicates(global int *data, local int *scratch)
{
  event_t events[4];
  events[0] = async_work_group_copy(scratch, data, 1, 0);
  events[1] = events[0];
  events[2] = async_work_group_copy(scratch+1, data+1, 3, 0);
  events[3] = events[0];

  wait_group_events(4, events);

  int i = get_local_id(0);
  data[get_local_size(0)-i-1] = scratch[i];
}
