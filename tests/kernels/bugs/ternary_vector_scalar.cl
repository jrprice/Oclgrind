kernel void ternary_vector_scalar(global int2 *data, int a, int b)
{
  int2 x = *data;
  *data = x ? a : b;
}
