kernel void uninitialized_global_buffer(global float *input,
                                        global float *output)
{
  output[get_global_id(0)] = *input;
}
