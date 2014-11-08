kernel void async_copy_out_of_bounds(local int *src, global int *dst)
{
  int l  = get_local_id(0);
  src[l] = l;
  barrier(CLK_LOCAL_MEM_FENCE);
  event_t event = async_work_group_copy(dst+1, src, get_local_size(0), 0);
  wait_group_events(1, &event);
}
