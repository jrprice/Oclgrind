kernel void uninitialized_private_array(global uint  *indices,
                                        global float *input,
                                        global float *output)
{
  float scratch[4];

  for (int i = 0; i < 4; i++)
  {
    scratch[indices[i]] = i;
  }

  for (int i = 0; i < 4; i++)
  {
    output[i] = scratch[i];
  }
}
