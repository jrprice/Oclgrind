kernel void broadcast(global int *value, global int *output)
{
  int i = get_global_id(0);
  output[i] = value[0];
}
