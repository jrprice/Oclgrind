#include "common.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOL 1e-8
#define MAX_ERRORS 8
#define N 16

const char* KERNEL_SOURCE =
  "kernel void test_sampler(read_only  image2d_t input,         \n"
  "                         write_only image2d_t output,        \n"
  "                                    sampler_t sampler)       \n"
  "{                                                            \n"
  "  int x = get_global_id(0);                                  \n"
  "  int y = get_global_id(1);                                  \n"
  "  float4 pixel = read_imagef(input, sampler, (int2)(x,y));   \n"
  "  float4 left  = read_imagef(input, sampler, (int2)(x-1,y)); \n"
  "  write_imagef(output, (int2)(x,y), pixel+left);             \n"
  "}                                                            \n";

unsigned checkResults(uint8_t* input, uint8_t* output);

int main(int argc, char* argv[])
{
  cl_int err;
  cl_kernel kernel;
  cl_mem d_input, d_output;
  cl_sampler sampler;

  Context cl = createContext(KERNEL_SOURCE, "");

  kernel = clCreateKernel(cl.program, "test_sampler", &err);
  checkError(err, "creating kernel");

  cl_image_format format;
  format.image_channel_order = CL_RGBA;
  format.image_channel_data_type = CL_UNORM_INT8;

  cl_image_desc desc = {0};
  desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  desc.image_width = N;
  desc.image_height = N;

  // Create images
  d_input =
    clCreateImage(cl.context, CL_MEM_READ_ONLY, &format, &desc, NULL, &err);
  checkError(err, "creating d_input image");
  d_output =
    clCreateImage(cl.context, CL_MEM_WRITE_ONLY, &format, &desc, NULL, &err);
  checkError(err, "creating d_output image");

  size_t dataSize = N * N * 4;

  // Initialise data
  uint8_t* h_input = malloc(dataSize);
  uint8_t* h_output = malloc(dataSize);
  srand(0);
  for (unsigned i = 0; i < dataSize; i++)
  {
    h_input[i] = rand() % 256 / 2;
    h_output[i] = 0;
  }

  size_t origin[] = {0, 0, 0};
  size_t region[] = {N, N, 1};
  err = clEnqueueWriteImage(cl.queue, d_input, CL_TRUE, origin, region, 0, 0,
                            h_input, 0, NULL, NULL);
  checkError(err, "writing image data");

  // Create sampler
  sampler = clCreateSampler(cl.context, CL_FALSE, CL_ADDRESS_CLAMP_TO_EDGE,
                            CL_FILTER_NEAREST, &err);
  checkError(err, "creating sampler");

  err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_input);
  err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_output);
  err |= clSetKernelArg(kernel, 2, sizeof(cl_sampler), &sampler);
  checkError(err, "setting kernel args");

  size_t global[2] = {N, N};
  err = clEnqueueNDRangeKernel(cl.queue, kernel, 2, NULL, global, NULL, 0, NULL,
                               NULL);
  checkError(err, "enqueuing kernel");

  err = clFinish(cl.queue);
  checkError(err, "running kernel");

  err = clEnqueueReadImage(cl.queue, d_output, CL_TRUE, origin, region, 0, 0,
                           h_output, 0, NULL, NULL);
  checkError(err, "writing image data");

  unsigned errors = checkResults(h_input, h_output);

  clReleaseMemObject(d_input);
  clReleaseMemObject(d_output);
  clReleaseKernel(kernel);
  releaseContext(cl);

  return (errors != 0);
}

unsigned checkResults(uint8_t* input, uint8_t* output)
{
  // Check results
  unsigned errors = 0;
  for (int y = 0; y < N; y++)
  {
    for (int x = 0; x < N; x++)
    {
      int xleft = x ? x - 1 : 0;
      for (int c = 0; c < 4; c++)
      {
        int i = (x + y * N) * 4 + c;
        int ref = input[i] + input[(xleft + y * N) * 4 + c];
        if (output[i] != ref)
        {
          if (errors < MAX_ERRORS)
          {
            fprintf(stderr, "%2d,%2d,%2d: %d != %d\n", x, y, c, output[i], ref);
          }
          errors++;
        }
      }
    }
  }

  printf("%d errors detected\n", errors);

  return errors;
}
