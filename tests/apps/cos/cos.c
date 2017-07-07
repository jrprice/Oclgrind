#include "common.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 1024

int main(int argc, char *argv[])
{
  cl_int err;
  cl_kernel kernel;
  cl_mem d_f;
  float *h_f;
  size_t global = N;

  if (argc < 2)
  {
    printf("Usage: ./cos libclc/test/cos.co\n");
    exit(1);
  }

  // FIXME: Error during operation 'creating program with binary invalid program binary': -42 
  Context cl = createContext(NULL, "", argv[1]);

  kernel = clCreateKernel(cl.program, "cos", &err);
  checkError(err, "creating kernel");

  size_t dataSize = N * sizeof(cl_float);

  // Initialise host data
  srand(0);
  h_f = malloc(dataSize);
  for (unsigned i = 0; i < N; i++)
  {
    h_f[i] = rand() / (float)RAND_MAX;
  }

  d_f = clCreateBuffer(cl.context, CL_MEM_READ_ONLY, dataSize, NULL, &err);
  checkError(err, "creating d_f buffer");

  err = clEnqueueWriteBuffer(cl.queue, d_f, CL_FALSE,
                             0, dataSize, h_f, 0, NULL, NULL);
  checkError(err, "writing d_f data");

  err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_f);
  checkError(err, "setting kernel args");

  err = clEnqueueNDRangeKernel(cl.queue, kernel,
                               1, NULL, &global, NULL, 0, NULL, NULL);
  checkError(err, "enqueuing kernel");

  err = clFinish(cl.queue);
  checkError(err, "running kernel");

  free(h_f);
  clReleaseMemObject(d_f);
  clReleaseKernel(kernel);
  releaseContext(cl);

  return 0;
}
