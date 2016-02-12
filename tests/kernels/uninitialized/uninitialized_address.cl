__kernel void uninitialized_address(__global ulong *output)
{
  int a[] = {1, 2, 3};
  volatile int i, j;

  a[i] = 4;

  output[0] = a[j];
}
