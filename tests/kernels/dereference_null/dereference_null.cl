kernel void dereference_null(global int *ptr, global int *output)
{
  global int *input = (global int*)*ptr;
  output[0] *= input[0];
}
