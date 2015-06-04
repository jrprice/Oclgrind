#include <CL/cl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOL 1e-8
#define MAX_ERRORS 8

const char *KERNEL_SOURCE =
"kernel void vecadd(global float *a, \n"
"                   global float *b, \n"
"                   global float *c) \n"
"{                                   \n"
"  int i = get_global_id(0);         \n"
"  c[i] = a[i] + b[i];               \n"
"}                                   \n"
;

void checkError(cl_int err, const char *operation);

int main(int argc, char *argv[])
{
  cl_int err;
  cl_platform_id platform;
  cl_device_id device;
  cl_context context;
  cl_command_queue queue;
  cl_program program;
  cl_kernel kernel;
  cl_mem d_a, d_b, d_c;
  float *h_a, *h_b, *h_c;

  size_t N = 1024;
  if (argc > 1)
  {
    N = atoi(argv[1]);
  }

  err = clGetPlatformIDs(1, &platform, NULL);
  checkError(err, "getting platform");

  // Check platform is Oclgrind
  char name[256];
  err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 256, name, NULL);
  checkError(err, "getting platform name");
  if (strcmp(name, "Oclgrind"))
  {
    fprintf(stderr, "Unable to find Oclgrind platform\n");
    exit(1);
  }

  err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);
  checkError(err, "getting device");

  context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
  checkError(err, "creating context");

  queue = clCreateCommandQueue(context, device, 0, &err);
  checkError(err, "creating command queue");

  program = clCreateProgramWithSource(context, 1, &KERNEL_SOURCE, NULL, &err);
  checkError(err, "creating program");

  err = clBuildProgram(program, 1, &device, "", NULL, NULL);
  if (err == CL_BUILD_PROGRAM_FAILURE)
  {
    size_t sz;
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                          sizeof(size_t), NULL, &sz);
    char *buildLog = malloc(++sz);
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                          sz, buildLog, NULL);
    fprintf(stderr, "%s\n", buildLog);
  }
  checkError(err, "building program");

  kernel = clCreateKernel(program, "vecadd", &err);
  checkError(err, "creating kernel");

  size_t dataSize = N*sizeof(cl_float);

  d_a = clCreateBuffer(context, CL_MEM_READ_ONLY, dataSize, NULL, &err);
  checkError(err, "creating d_a buffer");
  d_b = clCreateBuffer(context, CL_MEM_READ_ONLY, dataSize, NULL, &err);
  checkError(err, "creating d_b buffer");
  d_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, dataSize, NULL, &err);
  checkError(err, "creating d_c buffer");

  // Initialise data
  srand(0);
  h_a = clEnqueueMapBuffer(queue, d_a, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION,
                           0, dataSize, 0, NULL, NULL, &err);
  checkError(err, "mapping d_a buffer");
  h_b = clEnqueueMapBuffer(queue, d_b, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION,
                           0, dataSize, 0, NULL, NULL, &err);
  checkError(err, "mapping d_b buffer");
  h_c = malloc(dataSize);
  for (unsigned i = 0; i < N; i++)
  {
    h_a[i] = rand()/(float)RAND_MAX;
    h_b[i] = rand()/(float)RAND_MAX;
    h_c[i] = 0;
  }

  err = clEnqueueUnmapMemObject(queue, d_a, h_a, 0, NULL, NULL);
  checkError(err, "unmapping d_a");
  err = clEnqueueUnmapMemObject(queue, d_b, h_b, 0, NULL, NULL);
  checkError(err, "unmapping d_b");

  err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_a);
  err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_b);
  err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_c);
  checkError(err, "setting kernel args");

  err = clEnqueueNDRangeKernel(queue, kernel,
                               1, NULL, &N, NULL, 0, NULL, NULL);
  checkError(err, "enqueuing kernel");

  h_c = clEnqueueMapBuffer(queue, d_c, CL_FALSE, CL_MAP_READ, 0, dataSize,
                           0, NULL, NULL, &err);
  checkError(err, "mapping d_c buffer");

  err = clFinish(queue);
  checkError(err, "running kernel");

  // Check results
  unsigned errors = 0;
  for (unsigned i = 0; i < N; i++)
  {
    float ref = h_a[i] + h_b[i];
    if (fabs(ref - h_c[i]) > TOL)
    {
      if (errors < MAX_ERRORS)
      {
        fprintf(stderr, "%4d: %.4f != %.4f\n", i, h_c[i], ref);
      }
      errors++;
    }
  }
  if (errors)
    printf("%d errors detected\n", errors);

  clEnqueueUnmapMemObject(queue, d_c, h_c, 0, NULL, NULL);
  checkError(err, "unmapping d_c");

  clReleaseMemObject(d_a);
  clReleaseMemObject(d_b);
  clReleaseMemObject(d_c);
  clReleaseKernel(kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);

  return (errors != 0);
}

void checkError(cl_int err, const char *operation)
{
  if (err != CL_SUCCESS)
  {
    fprintf(stderr, "Error during operation '%s': %d\n", operation, err);
    exit(1);
  }
}
