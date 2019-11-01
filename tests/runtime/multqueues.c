#include "common.h"

#include <stdlib.h>
#include <stdio.h>

#define MQ_BUFSIZE 128

typedef enum { MQ_WAIT_FOR_EVENTS, MQ_FINISH } WaitType;

void write_read_test(cl_context ctx, cl_device_id dev,
                     cl_command_queue cq1, cl_command_queue cq2,
                     WaitType wait_type, char *test_name)
{
  // Variables
  cl_int err;
  cl_float *buf_host1A = (cl_float*)malloc(MQ_BUFSIZE * sizeof(cl_float));
  cl_float *buf_host1B = (cl_float*)calloc(MQ_BUFSIZE, sizeof(cl_float));
  cl_mem buf_dev1 = NULL;
  cl_int *buf_host2A = (cl_int*)malloc(MQ_BUFSIZE * sizeof(cl_int));
  cl_int *buf_host2B = (cl_int*)calloc(MQ_BUFSIZE, sizeof(cl_int));
  cl_mem buf_dev2 = NULL;
  cl_event ew[2] = { NULL, NULL }, er[2] = { NULL, NULL };
  cl_uint i;

  // Fill host buffers with random data
  for (i = 0; i < MQ_BUFSIZE; i++)
  {
    buf_host1A[i] = rand() / (cl_float)RAND_MAX;
    buf_host2A[i] = (cl_int)rand();
  }

  // Create device buffers
  buf_dev1 = clCreateBuffer(ctx, CL_MEM_READ_WRITE,
                            MQ_BUFSIZE * sizeof(cl_float), NULL, &err);
  checkError(err, "creating device buffer 1");
  buf_dev2 = clCreateBuffer(ctx, CL_MEM_READ_WRITE,
                            MQ_BUFSIZE * sizeof(cl_int), NULL, &err);
  checkError(err, "creating device buffer 2");

  // Write something to device buffer 1 using command queue 1,
  // generate event ew[0]
  err = clEnqueueWriteBuffer(cq1, buf_dev1, CL_FALSE, 0,
                             MQ_BUFSIZE * sizeof(cl_float), buf_host1A,
                             0, NULL, &ew[0]);
  checkError(err, "writing to buffer 1");

  // Write something to device buffer 2 using command queue 2,
  // generate event ew[1]
  err = clEnqueueWriteBuffer(cq2, buf_dev2, CL_FALSE, 0,
                             MQ_BUFSIZE * sizeof(cl_int), buf_host2A,
                             0, NULL, &ew[1]);
  checkError(err, "writing to buffer 2");

  // Read from device buffer 1 using command queue 2, make it depend
  // on event ew[0] and generate event er[0]
  err = clEnqueueReadBuffer(cq2, buf_dev1, CL_FALSE, 0,
                            MQ_BUFSIZE * sizeof(cl_float), buf_host1B,
                            1, &ew[0], &er[0]);
  checkError(err, "reading from buffer 1");

  // Read from device buffer 2 using command queue 1, make it depend
  // on event ew[1] and generate event er[1]
  err = clEnqueueReadBuffer(cq1, buf_dev2, CL_FALSE, 0,
                            MQ_BUFSIZE * sizeof(cl_int), buf_host2B,
                            1, &ew[1], &er[1]);
  checkError(err, "reading from buffer 1");

  // Wait on host thread for work to finish
  switch (wait_type)
  {
  case MQ_WAIT_FOR_EVENTS:
    // Wait on host thread for read events
    clWaitForEvents(2, er);
    break;
  case MQ_FINISH:
    // Wait on host thread for queues to be processed
    clFinish(cq1);
    // If they're not the same queue, must also wait in the second queue
    if (cq1 != cq2)
    {
      clFinish(cq2);
    }
    break;
  default:
    fprintf(stderr, "Unknown wait type\n");
    exit(1);
  }

  // Check results
  for (i = 0; i < MQ_BUFSIZE; i++)
  {
    if (buf_host1A[i] != buf_host1B[i] || buf_host2A[i] != buf_host2B[i])
    {
      fprintf(stderr, "Incorrect results in test %s\n", test_name);
      exit(1);
    }
  }

  // If we get here everything is OK
  printf("OK\n");

  // Release stuff
  for (i = 0; i < 2; i++)
  {
    clReleaseEvent(ew[i]);
    clReleaseEvent(er[i]);
  }
  clReleaseMemObject(buf_dev1);
  clReleaseMemObject(buf_dev2);
  free(buf_host1A);
  free(buf_host1B);
  free(buf_host2A);
  free(buf_host2B);
}

int main(int argc, char *argv[])
{
  ///////////
  // Setup //
  ///////////

  // Variables
  cl_platform_id platf;
  cl_context ctx = NULL;
  cl_device_id dev;
  cl_int err;
  cl_command_queue cq1 = NULL, cq2 = NULL, oocq = NULL;

  // Initialize PRNG
  srand(0);

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

  ////////////////////////////////
  // Test 1: Two command queues //
  ////////////////////////////////

  // Create first command queue
  cq1 = clCreateCommandQueue(ctx, dev, 0, &err);
  checkError(err, "creating first command queue");

  // Create second command queue
  cq2 = clCreateCommandQueue(ctx, dev, 0, &err);
  checkError(err, "creating second command queue");

  // Test 1.1: Perform test with two different command queues and explicitly
  // waiting for events
  write_read_test(ctx, dev, cq1, cq2, MQ_WAIT_FOR_EVENTS, "test 1.1");

  // Test 1.2: Perform test with two different command queues and wait for
  // the second queue to finish
  write_read_test(ctx, dev, cq1, cq2, MQ_FINISH, "test 1.2");

  // Release command queues
  clReleaseCommandQueue(cq2);
  clReleaseCommandQueue(cq1);

  ////////////////////////////////////////////
  // Test 2: One out-of-order command queue //
  ////////////////////////////////////////////

  // Create an out-of-order command queue
  oocq = clCreateCommandQueue(ctx, dev,
                              CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
  checkError(err, "creating out-of-order command queue");

  // Test 2.1: Perform test with out-of-order command queue and explicitly
  // waiting for events
  write_read_test(ctx, dev, oocq, oocq, MQ_WAIT_FOR_EVENTS, "test 2.1");

  // Test 2.2: Perform test with out-of-order command queue and wait for
  // queue to finish
  write_read_test(ctx, dev, oocq, oocq, MQ_FINISH, "test 2.2");

  // Release command queue
  clReleaseCommandQueue(oocq);

  //////////////
  // Clean-up //
  //////////////

  // Release context
  clReleaseContext(ctx);

  return 0;
}
