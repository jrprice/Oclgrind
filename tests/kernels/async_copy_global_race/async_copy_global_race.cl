kernel void async_copy_global_race(global int *data, local int *scratch)
{
  int i = get_local_id(0);
  scratch[i] = i;
  barrier(CLK_LOCAL_MEM_FENCE);

  data[i] = 0;

  event_t event = async_work_group_copy(data, scratch, get_local_size(0), 0);
  wait_group_events(1, &event);
}
