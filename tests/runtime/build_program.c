#include "common.h"

#include <stdio.h>
#include <stdlib.h>

#define TOL 1e-8
#define MAX_ERRORS 8
#define N 16

const char* SOURCE_INCLUDE = "#include \"header.h\"                    \n"
                             "kernel void test_kernel(global int *out) \n"
                             "{                                        \n"
                             "  *out = VALUE;                          \n"
                             "}                                        \n";

const char* SOURCE_MACRO = "#define _STR(ARG) #ARG                   \n"
                           "#define STR(ARG) _STR(ARG)               \n"
                           "kernel void test_kernel(global int *out) \n"
                           "{                                        \n"
                           "  printf(\"MSG = %s\\n\", STR(MSG));     \n"
                           "}                                        \n";

void run(const char* source, const char* options)
{
  cl_int err;
  cl_kernel kernel;
  cl_mem d_out;

  Context cl = createContext(source, options);

  kernel = clCreateKernel(cl.program, "test_kernel", &err);
  checkError(err, "creating kernel");

  d_out = clCreateBuffer(cl.context, CL_MEM_WRITE_ONLY, 4, NULL, &err);
  checkError(err, "creating d_out");

  err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_out);
  checkError(err, "setting kernel argument");

  size_t global[1] = {1};
  err = clEnqueueNDRangeKernel(cl.queue, kernel, 1, NULL, global, NULL, 0, NULL,
                               NULL);
  checkError(err, "enqueuing kernel");

  err = clFinish(cl.queue);
  checkError(err, "running kernel");

  int* h_out = clEnqueueMapBuffer(cl.queue, d_out, CL_TRUE, CL_MAP_READ, 0, 4,
                                  0, NULL, NULL, &err);
  checkError(err, "mapping buffer for reading");

  printf("out = %d\n", *h_out);

  err = clEnqueueUnmapMemObject(cl.queue, d_out, h_out, 0, NULL, NULL);
  checkError(err, "unmapping buffer");

  clReleaseMemObject(d_out);
  clReleaseKernel(kernel);
  releaseContext(cl);
}

int main(int argc, char* argv[])
{
  run(SOURCE_INCLUDE, "-I " ROOT_DIR "/inc/nospace");
  run(SOURCE_INCLUDE, "-I \"" ROOT_DIR "/inc/with space\"");
  run(SOURCE_MACRO, "-D MSG=hello");
  run(SOURCE_MACRO, "-D MSG=foo\\ and\\ bar");
  return 0;
}
