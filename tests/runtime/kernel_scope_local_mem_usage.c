#include "common.h"

#include <stdio.h>

const char *KERNEL_SOURCE =
"kernel void kernel1(global int *data)     \n"
"{                                         \n"
"  local int scratch[10];                  \n"
"  size_t lid = get_local_id(0);           \n"
"  scratch[lid] = data[lid];               \n"
"  barrier(CLK_LOCAL_MEM_FENCE);           \n"
"  int sum = 0;                            \n"
"  for (int i = 0; i < 10; i++)            \n"
"    sum += scratch[i];                    \n"
"  data[0] = sum;                          \n"
"}                                         \n"
"                                          \n"
"kernel void kernel2(global int *data)     \n"
"{                                         \n"
"  local int scratch[20];                  \n"
"  size_t lid = get_local_id(0);           \n"
"  scratch[lid] = data[lid];               \n"
"  barrier(CLK_LOCAL_MEM_FENCE);           \n"
"  int sum = 0;                            \n"
"  for (int i = 0; i < 20; i++)            \n"
"    sum += scratch[i];                    \n"
"  data[0] = sum;                          \n"
"}                                         \n"
"                                          \n"
;

int main(int argc, char *argv[])
{
  cl_int err;
  cl_kernel kernel1, kernel2;
  cl_ulong localSize;

  Context cl = createContext(KERNEL_SOURCE, "");

  kernel1 = clCreateKernel(cl.program, "kernel1", &err);
  checkError(err, "creating kernel1");

  kernel2 = clCreateKernel(cl.program, "kernel2", &err);
  checkError(err, "creating kernel2");

  err = clGetKernelWorkGroupInfo(kernel1, cl.device, CL_KERNEL_LOCAL_MEM_SIZE,
                                 sizeof(cl_ulong), &localSize, NULL);
  checkError(err, "getting kernel1 local mem size");
  if (localSize != 40)
  {
    fprintf(stderr, "Incorrect kernel1 local memory size %llu (expected 40)\n",
            localSize);
    return 1;
  }

  err = clGetKernelWorkGroupInfo(kernel2, cl.device, CL_KERNEL_LOCAL_MEM_SIZE,
                                 sizeof(cl_ulong), &localSize, NULL);
  checkError(err, "getting kernel2 local mem size");
  if (localSize != 80)
  {
    fprintf(stderr, "Incorrect kernel2 local memory size %llu (expected 80)\n",
            localSize);
    return 1;
  }

  clReleaseKernel(kernel1);
  clReleaseKernel(kernel2);
  releaseContext(cl);

  return 0;
}
