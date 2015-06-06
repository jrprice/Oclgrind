kernel void wait_event_divergent(global int *data, local int *scratch)
{
  int i = get_local_id(0);
  scratch[i] = 0;
  barrier(CLK_LOCAL_MEM_FENCE);

  event_t events[2];
  events[0] = async_work_group_copy(scratch, data, 1, 0);
  events[1] = async_work_group_copy(scratch+1, data+1, 1, 0);

  wait_group_events(1, events+i);

  data[get_local_size(0)-i-1] = scratch[i];
}
