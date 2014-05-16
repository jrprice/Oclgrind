kernel void async_copy_unwaited(global int *data, local int *scratch)
{
  event_t event = async_work_group_copy(scratch, data, get_local_size(0), 0);

  int i = get_local_id(0);
  data[get_local_size(0)-i-1] = i;
}
