#include "common.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMG_SIZE 100
#define TOL 1e-8
#define MAX_ERRORS 8

const char* KERNEL_SOURCE =
  "__kernel void image_copy(__read_only image2d_array_t src,  \n"
  "                         __write_only image2d_array_t dst) \n"
  "{                                                          \n"
  "   size_t size = get_image_array_size(src);                \n"
  "   const int x = get_global_id(0);                         \n"
  "   const int y = get_global_id(1);                         \n"
  "   const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | \n"
  "                             CLK_ADDRESS_CLAMP |           \n"
  "                             CLK_FILTER_NEAREST;           \n"
  "   float4 pixel = read_imagef(src, sampler, (int4)(x, y, 0, 0)); \n"
  "   write_imagef(dst, (int4)(x, y, 0, 0), pixel);                 \n"
  "}                                                          \n";

int main()
{
  cl_int err;
  cl_kernel kernel;
  cl_image_format img_fmt;
  cl_image_desc img_desc;
  cl_mem src, dst;
  float *input, *output;
  size_t width, height;
  width = height = 10;
  size_t origin[] = {0, 0, 0};
  size_t region[] = {width, height, 1};
  size_t GWSize[] = {width, height, 1};

  input = (float*)malloc(IMG_SIZE * 3 * sizeof(float));
  output = (float*)malloc(IMG_SIZE * 3 * sizeof(float));

  // Create Input data
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < IMG_SIZE; ++j)
    {
      input[i * IMG_SIZE + j] = j + 1.0;
    }
  }

  Context cl = createContext(KERNEL_SOURCE, "");
  kernel = clCreateKernel(cl.program, "image_copy", &err);
  checkError(err, "creating kernel");

  img_fmt.image_channel_order = CL_RGB;
  img_fmt.image_channel_data_type = CL_FLOAT;

  img_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  img_desc.image_width = width;
  img_desc.image_height = height;
  img_desc.image_depth = 0;
  img_desc.image_array_size = 0;
  img_desc.image_row_pitch = 0;
  img_desc.image_slice_pitch = 0;
  img_desc.num_mip_levels = 0;
  img_desc.num_samples = 0;
  img_desc.buffer = NULL;

  src = clCreateImage(cl.context, CL_MEM_READ_ONLY, &img_fmt, &img_desc, NULL,
                      &err);
  checkError(err, "creating source image");

  dst = clCreateImage(cl.context, CL_MEM_READ_WRITE, &img_fmt, &img_desc, NULL,
                      &err);
  checkError(err, "creating destination image");

  err = clEnqueueWriteImage(cl.queue, src, CL_TRUE, origin, region, 0, 0, input,
                            0, NULL, NULL);
  checkError(err, "enqueuing write image");

  err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &src);
  err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &dst);
  checkError(err, "setting kernel args");

  err = clEnqueueNDRangeKernel(cl.queue, kernel, 2, NULL, GWSize, NULL, 0, NULL,
                               NULL);
  checkError(err, "enqueuing kernel");

  err = clFinish(cl.queue);
  checkError(err, "running kernel");

  err = clEnqueueReadImage(cl.queue, dst, CL_TRUE, origin, region, 0, 0, output,
                           0, NULL, NULL);
  checkError(err, "reading image data");

  // Check results
  unsigned errors = 0;
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < IMG_SIZE; ++j)
    {
      float ref = input[i * IMG_SIZE + j];
      float val = output[i * IMG_SIZE + j];

      if (fabs(ref - val) > TOL)
      {
        if (errors < MAX_ERRORS)
        {
          fprintf(stderr, "%4d: %.4f != %.4f\n", i, val, ref);
        }
        errors++;
      }
    }
  }

  free(input);
  free(output);
  clReleaseMemObject(src);
  clReleaseMemObject(dst);
  clReleaseKernel(kernel);
  releaseContext(cl);

  return (errors != 0);
}

// cl_mem image3;
//
// image3 = clCreateImage2D(context, CL_MEM_READ_WRITE, &img_fmt, width, height,
// 0, 0, &err);
//
//// copy Image1 to Image3
// err = clEnqueueCopyImage(command_queue, image1, image3, origin, origin,
// region, 1, event, &event[3]); err_check(err, "clEnqueueCopyImage");

// clReleaseMemObject(image3);
