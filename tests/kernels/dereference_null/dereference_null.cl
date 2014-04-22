kernel void dereference_null(global int *input, global int *output)
{
  output[0] *= input[0];
}
