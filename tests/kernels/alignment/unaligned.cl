kernel void unaligned(global int *in, global int *out)
{
  global char *char_ptr = (global char*)in + 2;
  global int *address   = (global int*)char_ptr;
  *out = *address;
}
