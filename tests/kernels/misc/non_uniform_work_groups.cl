kernel void non_uniform_work_groups(global int *output)
{
  int i = get_global_linear_id();
  output[i] = get_local_linear_id();

  int end = get_global_size(0) * get_global_size(1) * get_global_size(2);
  if (i == end-1)
  {
    output[end]   = get_local_size(0);
    output[end+1] = get_local_size(1);
    output[end+2] = get_local_size(2);
    output[end+3] = get_enqueued_local_size(0);
    output[end+4] = get_enqueued_local_size(1);
    output[end+5] = get_enqueued_local_size(2);
  }
}
