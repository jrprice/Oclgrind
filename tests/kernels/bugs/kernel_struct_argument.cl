typedef struct
{
  float a;
  float b;
  float c;
} Structure;

kernel void kernel_struct_argument(Structure x, global float *out)
{
  *out = x.a * x.b + x.c;
}
