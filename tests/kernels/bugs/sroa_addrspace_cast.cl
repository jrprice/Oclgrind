typedef struct
{
  float x;
} DataStruct;

__kernel void sroa_addrspace_cast(__global DataStruct *input,
                                  __global float *output)
{
  size_t i = get_global_id(0);
  DataStruct s = input[i];
  output[i] = s.x;
}
