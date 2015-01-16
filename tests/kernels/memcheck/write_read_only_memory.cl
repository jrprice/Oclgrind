kernel void write_read_only_memory(global int *input, global int *output)
{
  int i = get_global_id(0);
  output[i] = input[i]++;
}
