kernel void private_array_initializer_list(global float *output)
{
  float scratch[4] = {7.f, 42.f, -1.f, 0.f};

  for (int i = 0; i < 4; i++)
  {
    output[i] = scratch[i];
  }
}
