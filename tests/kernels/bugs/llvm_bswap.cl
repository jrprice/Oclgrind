kernel void test(global uint *input, global uint *output)
{
  for (unsigned int i = 0; i < 4; i++)
  {
    uint word = input[i];
    output[i] = ((word & 0xff) << 24) | ((word & 0xff00) << 8) | ((word & 0xff0000) >> 8) | ((word & 0xff000000) >> 24);
  }
}
