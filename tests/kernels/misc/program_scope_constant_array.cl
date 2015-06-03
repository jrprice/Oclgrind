constant int data[4] = {7, 42, 0, -1};

kernel void program_scope_constant_array(global int *output)
{
  int i = get_global_id(0);
  output[i] = data[i];
}
