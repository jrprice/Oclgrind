kernel void write_vector_write_only_fp(global int4 *output)
{
  int i = get_global_id(0);
  output[i].x = 42;
}
