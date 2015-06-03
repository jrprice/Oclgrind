kernel void vector_argument(int4 vector, global int4 *output)
{
  *output = vector + 42;
}
