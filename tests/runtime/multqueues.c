#include "common.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{

  cl_platform_id platf;
  cl_device_id dev;
  cl_int err;
  cl_float buf_host1[] = { 1.2, 3.2, -4.9, 4.6, 9.0, -5.1, -9.7, 7.8 };
  cl_float buf_host2[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  cl_mem buf_dev = NULL;
  cl_command_queue cq1 = NULL, cq2 = NULL;
  cl_event evt1 = NULL, evt2 = NULL;
  cl_context ctx = NULL;
  cl_uint i;

  // Get platform
  err = clGetPlatformIDs(1, &platf, NULL);
  checkError(err, "getting platform");

  // Check its Oclgrind
  checkOclgrindPlatform(platf);

  // Get first device
  err = clGetDeviceIDs(platf, CL_DEVICE_TYPE_ALL, 1, &dev, NULL);
  checkError(err, "getting device");

  // Create context
  ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
  checkError(err, "creating context");

  // Create first command queue
  cq1 = clCreateCommandQueue(ctx, dev, 0, &err);
  checkError(err, "creating first command queue");

  // Create second command queue
  cq2 = clCreateCommandQueue(ctx, dev, 0, &err);
  checkError(err, "creating second command queue");

  // Create a device buffer
  buf_dev = clCreateBuffer(ctx, CL_MEM_READ_WRITE, 8 * sizeof(cl_float),
                           NULL, &err);
  checkError(err, "creating buffer");

  // Write something to device buffer using command queue 1, get an event
  err = clEnqueueWriteBuffer(cq1, buf_dev, CL_FALSE, 0,
                             8 * sizeof(cl_float), buf_host1, 0, NULL, &evt1);
  checkError(err, "writing to buffer");

  // Read something from buffer using command queue 2, make it depend on
  // previous event
  err = clEnqueueReadBuffer(cq2, buf_dev, CL_FALSE, 0,
                            8 * sizeof(cl_float), buf_host2, 1, &evt1, &evt2);
  checkError(err, "reading from buffer");

  // Wait on host thread for read event
  clWaitForEvents(1, &evt2);

  // Free stuff
  clReleaseEvent(evt2);
  clReleaseEvent(evt1);
  clReleaseMemObject(buf_dev);
  clReleaseCommandQueue(cq2);
  clReleaseCommandQueue(cq1);
  clReleaseContext(ctx);

  // Check results
  for (i = 0; i < 8; i++)
  {
    if (buf_host1[i] != buf_host2[i])
    {
      fprintf(stderr, "Incorrect results\n");
      exit(1);
    }
  }

  printf("OK\n");
  return 0;
}
