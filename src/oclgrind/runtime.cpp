#include <cmath>
#include <cstring>
#include <iostream>

#include "icd.h"

#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <CL/cl_gl_ext.h>

#include <spirsim/Device.h>
#include <spirsim/Kernel.h>
#include <spirsim/Memory.h>
#include <spirsim/Program.h>

using namespace std;

CLIicdDispatchTable *m_dispatchTable = NULL;
struct _cl_platform_id *m_platform = NULL;
static struct _cl_device_id *m_device = NULL;

#define ERRCODE(err) if(errcode_ret){*errcode_ret = err;}
#define MAX_MEM_SIZE (uint32_t)-1
#define MAX_WI_SIZE (unsigned short)-1

#define DEVICE_NAME "SPIR Simulator"
#define DEVICE_VENDOR "James Price, University of Bristol"
#define DEVICE_VENDOR_ID 0x0042
#define DEVICE_VERSION "OpenCL 1.2"
#define DRIVER_VERSION "0.1"

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformIDs(cl_uint           num_entries ,
                 cl_platform_id *  platforms ,
                 cl_uint *         num_platforms) CL_API_SUFFIX__VERSION_1_0
{
  return clIcdGetPlatformIDsKHR(num_entries, platforms, num_platforms);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformInfo(cl_platform_id    platform,
                  cl_platform_info  param_name,
                  size_t            param_value_size,
                  void *            param_value,
                  size_t *          param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  cl_int return_value = CL_SUCCESS;
  const char *returnString = NULL;
  size_t returnStringLength = 0;

  // validate the arguments
  if (param_value_size == 0 && param_value != NULL)
  {
    return CL_INVALID_VALUE;
  }
  // select the string to return
  switch(param_name)
  {
  case CL_PLATFORM_PROFILE:
    returnString = platform->profile;
    break;
  case CL_PLATFORM_VERSION:
    returnString = platform->version;
    break;
  case CL_PLATFORM_NAME:
    returnString = platform->name;
    break;
  case CL_PLATFORM_VENDOR:
    returnString = platform->vendor;
    break;
  case CL_PLATFORM_EXTENSIONS:
    returnString = platform->extensions;
    break;
  case CL_PLATFORM_ICD_SUFFIX_KHR:
    returnString = platform->suffix;
    break;
  default:
    return CL_INVALID_VALUE;
  }

  // make sure the buffer passed in is big enough for the result
  returnStringLength = strlen(returnString)+1;
  if (param_value_size && param_value_size < returnStringLength)
  {
    return CL_INVALID_VALUE;
  }

  // pass the data back to the user
  if (param_value)
  {
    memcpy(param_value, returnString, returnStringLength);
  }
  if (param_value_size_ret)
  {
    *param_value_size_ret = returnStringLength;
  }

  return return_value;
}


/* Device APIs */
CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceIDs(cl_platform_id   platform,
               cl_device_type   device_type,
               cl_uint          num_entries,
               cl_device_id *   devices,
               cl_uint *        num_devices) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (devices && num_entries < 1)
  {
    return CL_INVALID_VALUE;
  }

  cl_int return_value = CL_SUCCESS;
  if (device_type != CL_DEVICE_TYPE_CPU &&
      device_type != CL_DEVICE_TYPE_DEFAULT &&
      device_type != CL_DEVICE_TYPE_ALL)
  {
    return_value = CL_DEVICE_NOT_FOUND;
  }
  else  if (devices)
  {
    // Create device if haven't already
    if (!m_device)
    {
      m_device = (cl_device_id)malloc(sizeof(cl_device_id));
      m_device->dispatch = m_dispatchTable;
    }
    *devices = m_device;
  }

  if (num_devices)
  {
    *num_devices = return_value==CL_SUCCESS ? 1 : 0;
  }

  return return_value;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceInfo(cl_device_id    device,
                cl_device_info  param_name,
                size_t          param_value_size,
                void *          param_value,
                size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check device is valid
  if (device != m_device)
  {
    return CL_INVALID_DEVICE;
  }

  switch (param_name)
  {
  case CL_DEVICE_TYPE:
    result_size = sizeof(cl_device_type);
    result_data = new cl_device_type(CL_DEVICE_TYPE_CPU);
    break;
  case CL_DEVICE_VENDOR_ID:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(DEVICE_VENDOR_ID);
    break;
  case CL_DEVICE_MAX_COMPUTE_UNITS:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(3);
    break;
  case CL_DEVICE_MAX_WORK_GROUP_SIZE:
    result_size = sizeof(size_t);
    result_data = new size_t(MAX_WI_SIZE);
    break;
  case CL_DEVICE_MAX_WORK_ITEM_SIZES:
    result_size = 3*sizeof(size_t);
    result_data = new size_t[3];
    ((size_t*)result_data)[0] = MAX_WI_SIZE;
    ((size_t*)result_data)[1] = MAX_WI_SIZE;
    ((size_t*)result_data)[2] = MAX_WI_SIZE;
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(0);
    break;
  case CL_DEVICE_MAX_CLOCK_FREQUENCY:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_ADDRESS_BITS:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(sizeof(size_t)>>3);
    break;
  case CL_DEVICE_MAX_READ_IMAGE_ARGS:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(0);
    break;
  case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(0);
    break;
  case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
    result_size = sizeof(size_t);
    result_data = new cl_ulong(MAX_MEM_SIZE);
    break;
  case CL_DEVICE_IMAGE2D_MAX_WIDTH:
    result_size = sizeof(size_t);
    result_data = new size_t(0);
    break;
  case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
    result_size = sizeof(size_t);
    result_data = new size_t(0);
    break;
  case CL_DEVICE_IMAGE3D_MAX_WIDTH:
    result_size = sizeof(size_t);
    result_data = new size_t(0);
    break;
  case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
    result_size = sizeof(size_t);
    result_data = new size_t(0);
    break;
  case CL_DEVICE_IMAGE3D_MAX_DEPTH:
    result_size = sizeof(size_t);
    result_data = new size_t(0);
    break;
  case CL_DEVICE_IMAGE_SUPPORT:
    result_size = sizeof(cl_bool);
    result_data = new cl_bool(CL_FALSE);
    break;
  case CL_DEVICE_MAX_PARAMETER_SIZE:
    result_size = sizeof(size_t);
    result_data = new size_t(1024);
    break;
  case CL_DEVICE_MAX_SAMPLERS:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(0);
    break;
  case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(sizeof(cl_long16)<<3);
    break;
  case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_SINGLE_FP_CONFIG:
    result_size = sizeof(cl_device_fp_config);
    result_data = new cl_device_fp_config(
      CL_FP_ROUND_TO_NEAREST | CL_FP_INF_NAN);
    break;
  case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
    result_size = sizeof(cl_device_mem_cache_type);
    result_data = new cl_device_mem_cache_type(CL_NONE);
    break;
  case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(0);
    break;
  case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
    result_size = sizeof(cl_ulong);
    result_data = new cl_ulong(0);
    break;
  case CL_DEVICE_GLOBAL_MEM_SIZE:
    result_size = sizeof(cl_ulong);
    result_data = new cl_ulong(MAX_MEM_SIZE);
    break;
  case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
    result_size = sizeof(cl_ulong);
    result_data = new cl_ulong(MAX_MEM_SIZE);
    break;
  case CL_DEVICE_MAX_CONSTANT_ARGS:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1024);
    break;
  case CL_DEVICE_LOCAL_MEM_TYPE:
    result_size = sizeof(cl_device_local_mem_type);
    result_data = new cl_device_local_mem_type(CL_LOCAL);
    break;
  case CL_DEVICE_LOCAL_MEM_SIZE:
    result_size = sizeof(cl_ulong);
    result_data = new cl_ulong();
    break;
    return CL_INVALID_VALUE;
  case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
    result_size = sizeof(cl_bool);
    result_data = new cl_bool(CL_FALSE);
    break;
  case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
    result_size = sizeof(size_t);
    result_data = new size_t(1);
    break;
  case CL_DEVICE_ENDIAN_LITTLE:
    result_size = sizeof(cl_bool);
    result_data = new cl_bool(CL_TRUE);
    break;
  case CL_DEVICE_AVAILABLE:
    result_size = sizeof(cl_bool);
    result_data = new cl_bool(CL_TRUE);
    break;
  case CL_DEVICE_COMPILER_AVAILABLE:
    result_size = sizeof(cl_bool);
    result_data = new cl_bool(CL_TRUE);
    break;
  case CL_DEVICE_EXECUTION_CAPABILITIES:
    result_size = sizeof(cl_device_exec_capabilities);
    result_data = new cl_device_exec_capabilities(CL_EXEC_KERNEL);
    break;
  case CL_DEVICE_QUEUE_PROPERTIES:
    result_size = sizeof(cl_command_queue_properties);
    result_data = new cl_command_queue_properties(CL_QUEUE_PROFILING_ENABLE);
    break;
  case CL_DEVICE_NAME:
    result_data = strdup(DEVICE_NAME);
    result_size = (strlen((char*)result_data)+1)*sizeof(char);
    break;
  case CL_DEVICE_VENDOR:
    result_data = strdup(DEVICE_VENDOR);
    result_size = (strlen((char*)result_data)+1)*sizeof(char);
    break;
  case CL_DRIVER_VERSION:
    result_data = strdup(DRIVER_VERSION);
    result_size = (strlen((char*)result_data)+1)*sizeof(char);
    break;
  case CL_DEVICE_PROFILE:
    result_data = strdup("FULL_PROFILE");
    result_size = (strlen((char*)result_data)+1)*sizeof(char);
    break;
  case CL_DEVICE_VERSION:
    result_data = strdup(DEVICE_VERSION);
    result_size = (strlen((char*)result_data)+1)*sizeof(char);
    break;
  case CL_DEVICE_EXTENSIONS:
    result_data = strdup("");
    result_size = (strlen((char*)result_data)+1)*sizeof(char);
    break;
  case CL_DEVICE_PLATFORM:
    result_size = sizeof(cl_platform_id);
    result_data = new cl_platform_id(m_platform);
    break;
  case CL_DEVICE_DOUBLE_FP_CONFIG:
    result_size = sizeof(cl_device_fp_config);
    result_data = new cl_device_fp_config(0);
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(0);
    break;
  case CL_DEVICE_HOST_UNIFIED_MEMORY:
    result_size = sizeof(cl_bool);
    result_data = new cl_bool(CL_FALSE);
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(0);
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(0);
    break;
  case CL_DEVICE_OPENCL_C_VERSION:
    result_data = strdup(DEVICE_VERSION);
    result_size = (strlen((char*)result_data)+1)*sizeof(char);
    break;
  case CL_DEVICE_LINKER_AVAILABLE:
    result_size = sizeof(cl_bool);
    result_data = new cl_bool(CL_TRUE);
    break;
  case CL_DEVICE_BUILT_IN_KERNELS:
    result_data = strdup("");
    result_size = (strlen((char*)result_data)+1)*sizeof(char);
    break;
  case CL_DEVICE_IMAGE_MAX_BUFFER_SIZE:
    result_size = sizeof(size_t);
    result_data = new size_t(0);
    break;
  case CL_DEVICE_IMAGE_MAX_ARRAY_SIZE:
    result_size = sizeof(size_t);
    result_data = new size_t(0);
    break;
  case CL_DEVICE_PARENT_DEVICE:
    result_size = sizeof(cl_device_id);
    result_data = new cl_device_id(NULL);
    break;
  case CL_DEVICE_PARTITION_MAX_SUB_DEVICES:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(0);
    break;
  case CL_DEVICE_PARTITION_PROPERTIES:
    result_size = sizeof(cl_device_partition_property);
    result_data = new cl_device_partition_property(0);
    break;
  case CL_DEVICE_PARTITION_AFFINITY_DOMAIN:
    result_size = sizeof(cl_device_affinity_domain);
    result_data = new cl_device_affinity_domain(0);
    break;
  case CL_DEVICE_PARTITION_TYPE:
    result_size = sizeof(cl_device_partition_property);
    result_data = new cl_device_partition_property(0);
    break;
  case CL_DEVICE_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_DEVICE_PREFERRED_INTEROP_USER_SYNC:
    result_size = sizeof(cl_bool);
    result_data = new cl_bool(CL_TRUE);
    break;
  case CL_DEVICE_PRINTF_BUFFER_SIZE:
    result_size = sizeof(size_t);
    result_data = new size_t(1024*1024);
    break;
  default:
    return CL_INVALID_VALUE;
  }

  cl_int return_value = CL_SUCCESS;
  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      return_value = CL_INVALID_VALUE;
    }
    else
    {
      memcpy(param_value, result_data, result_size);
    }
  }

  if (param_value_size_ret)
  {
    *param_value_size_ret = result_size;
  }

  free(result_data);

  return return_value;
}

CL_API_ENTRY cl_int CL_API_CALL
clCreateSubDevices(cl_device_id in_device,
                   const cl_device_partition_property *properties,
                   cl_uint num_entries,
                   cl_device_id *out_devices,
                   cl_uint *num_devices) CL_API_SUFFIX__VERSION_1_2
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainDevice(cl_device_id device) CL_API_SUFFIX__VERSION_1_2
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseDevice(cl_device_id device) CL_API_SUFFIX__VERSION_1_2
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}


/* Context APIs  */
CL_API_ENTRY cl_context CL_API_CALL
clCreateContext(const cl_context_properties * properties,
                cl_uint                       num_devices ,
                const cl_device_id *          devices,
                void (CL_CALLBACK * pfn_notify)(const char *, const void *, size_t, void *),
                void *                        user_data,
                cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (num_devices != 1 || !devices)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }
  if (devices[0] != m_device)
  {
    ERRCODE(CL_INVALID_DEVICE);
    return NULL;
  }
  if (!pfn_notify && user_data)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }

  // Create context object
  cl_context context = (cl_context)malloc(sizeof(struct _cl_context));
  context->dispatch = m_dispatchTable;
  context->device = new spirsim::Device();
  context->notify = pfn_notify;
  context->data = user_data;
  context->properties = NULL;
  context->szProperties = 0;
  context->refCount = 1;

  if (properties)
  {
    int num = 1;
    while (properties[num])
    {
      num++;
    }
    size_t sz = (num+1)*sizeof(cl_context_properties);
    context->szProperties = sz;
    context->properties = (cl_context_properties*)malloc(sz);
    memcpy(context->properties, properties, sz);
  }

  ERRCODE(CL_SUCCESS);
  return context;
}

CL_API_ENTRY cl_context CL_API_CALL
clCreateContextFromType(const cl_context_properties * properties,
                        cl_device_type                device_type,
                        void (CL_CALLBACK *     pfn_notify)(const char *, const void *, size_t, void *),
                        void *                        user_data,
                        cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!pfn_notify && user_data)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }
  if (device_type != CL_DEVICE_TYPE_CPU &&
      device_type != CL_DEVICE_TYPE_DEFAULT &&
      device_type != CL_DEVICE_TYPE_ALL)
  {
    ERRCODE(CL_DEVICE_NOT_FOUND);
    return NULL;
  }

  // Create context object
  cl_context context = (cl_context)malloc(sizeof(struct _cl_context));
  context->dispatch = m_dispatchTable;
  context->device = new spirsim::Device();
  context->notify = pfn_notify;
  context->data = user_data;
  context->properties = NULL;
  context->szProperties = 0;
  context->refCount = 1;

  if (properties)
  {
    int num = 0;
    while (properties[num])
    {
      num++;
    }
    size_t sz = (num+1)*sizeof(cl_context_properties);
    context->szProperties = sz;
    context->properties = (cl_context_properties*)malloc(sz);
    memcpy(context->properties, properties, sz);
  }

  ERRCODE(CL_SUCCESS);
  return context;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
  if (!context)
  {
    return CL_INVALID_CONTEXT;
  }

  context->refCount++;

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
  if (!context)
  {
    return CL_INVALID_CONTEXT;
  }

  if (--context->refCount == 0)
  {
    delete context->device;
    free(context);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetContextInfo(cl_context         context,
                 cl_context_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check context is valid
  if (!context)
  {
    return CL_INVALID_CONTEXT;
  }

  switch (param_name)
  {
  case CL_CONTEXT_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(context->refCount);
    break;
  case CL_CONTEXT_NUM_DEVICES:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_CONTEXT_DEVICES:
    result_size = sizeof(cl_device_id);
    result_data = new cl_device_id(m_device);
    break;
  case CL_CONTEXT_PROPERTIES:
    result_size = context->szProperties;
    result_data = malloc(result_size);
    memcpy(result_data, context->properties, result_size);
    break;
  default:
    return CL_INVALID_VALUE;
  }

  cl_int return_value = CL_SUCCESS;
  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      return_value = CL_INVALID_VALUE;
    }
    else
    {
      memcpy(param_value, result_data, result_size);
    }
  }

  if (param_value_size_ret)
  {
    *param_value_size_ret = result_size;
  }

  free(result_data);

  return return_value;
}


/* Command Queue APIs */
CL_API_ENTRY cl_command_queue CL_API_CALL
clCreateCommandQueue(cl_context                     context,
                     cl_device_id                   device,
                     cl_command_queue_properties    properties,
                     cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!context)
  {
    ERRCODE(CL_INVALID_CONTEXT);
    return NULL;
  }
  if (device != m_device)
  {
    ERRCODE(CL_INVALID_DEVICE);
    return NULL;
  }
  if (properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)
  {
    ERRCODE(CL_INVALID_QUEUE_PROPERTIES);
    return NULL;
  }

  // Create command-queue object
  cl_command_queue queue;
  queue = (cl_command_queue)malloc(sizeof(struct _cl_command_queue));
  queue->dispatch = m_dispatchTable;
  queue->properties = properties;
  queue->context = context;
  queue->refCount = 1;

  ERRCODE(CL_SUCCESS);
  return queue;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetCommandQueueProperty(cl_command_queue               command_queue ,
                          cl_command_queue_properties    properties ,
                          cl_bool                        enable ,
                          cl_command_queue_properties *  old_properties) //CL_EXT_SUFFIX__VERSION_1_0_DEPRECATED
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  command_queue->refCount++;

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  if (--command_queue->refCount == 0)
  {
    free(command_queue);
  }

  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetCommandQueueInfo(cl_command_queue       command_queue ,
                      cl_command_queue_info  param_name ,
                      size_t                 param_value_size ,
                      void *                 param_value ,
                      size_t *               param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check queue is valid
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  switch (param_name)
  {
  case CL_QUEUE_CONTEXT:
    result_size = sizeof(cl_context);
    result_data = new cl_context(command_queue->context);
    break;
  case CL_QUEUE_DEVICE:
    result_size = sizeof(cl_device_id);
    result_data = new cl_device_id(m_device);
    break;
  case CL_QUEUE_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(command_queue->refCount);
    break;
  case CL_QUEUE_PROPERTIES:
    result_size = sizeof(cl_command_queue_properties);
    result_data = new cl_command_queue_properties(command_queue->properties);
    break;
  default:
    return CL_INVALID_VALUE;
  }

  cl_int return_value = CL_SUCCESS;
  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      return_value = CL_INVALID_VALUE;
    }
    else
    {
      memcpy(param_value, result_data, result_size);
    }
  }

  if (param_value_size_ret)
  {
    *param_value_size_ret = result_size;
  }

  free(result_data);

  return return_value;
}


/* Memory Object APIs */
CL_API_ENTRY cl_mem CL_API_CALL
clCreateBuffer(cl_context    context ,
               cl_mem_flags  flags ,
               size_t        size ,
               void *        host_ptr ,
               cl_int *      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!context)
  {
    ERRCODE(CL_INVALID_CONTEXT);
    return NULL;
  }
  if (size == 0)
  {
    ERRCODE(CL_INVALID_BUFFER_SIZE);
    return NULL;
  }
  if ((host_ptr == NULL) ==
      ((flags & CL_MEM_COPY_HOST_PTR) ||
        flags & CL_MEM_USE_HOST_PTR))
  {
    ERRCODE(CL_INVALID_HOST_PTR);
    return NULL;
  }
  if (flags & CL_MEM_USE_HOST_PTR)
  {
    cerr << "OCLGRIND: CL_MEM_USE_HOST_PTR not supported." << endl;
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }

  // Create memory object
  cl_mem mem = (cl_mem)malloc(sizeof(struct _cl_mem));
  mem->dispatch = m_dispatchTable;
  mem->context = context;
  mem->address = context->device->getGlobalMemory()->allocateBuffer(size);
  mem->size = size;
  mem->flags = flags;
  mem->callbacks = new std::stack<void (CL_CALLBACK *)(cl_mem, void *)>();
  mem->data = new std::stack<void*>();
  mem->refCount = 1;
  // TODO: Possible allocation failure
  //if (!mem->address)
  //{
  //  ERRCODE(CL_MEM_OBJECT_ALLOCATION_FAILURE);
  //  free(mem);
  //  return NULL;
  //}

  if (flags & CL_MEM_COPY_HOST_PTR)
  {
    context->device->getGlobalMemory()->store(mem->address, size,
                                              (const unsigned char*)host_ptr);
  }

  ERRCODE(CL_SUCCESS);
  return mem;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateSubBuffer(cl_mem                    buffer ,
                  cl_mem_flags              flags ,
                  cl_buffer_create_type     buffer_create_type ,
                  const void *              buffer_create_info ,
                  cl_int *                  errcode_ret) CL_API_SUFFIX__VERSION_1_1
{
  //cl_mem obj = (cl_mem) malloc(sizeof(struct _cl_mem));
  //obj->dispatch = dispatchTable;
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage(cl_context              context,
              cl_mem_flags            flags,
              const cl_image_format * image_format,
              const cl_image_desc *   image_desc,
              void *                  host_ptr,
              cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_2
{
  //cl_mem obj = (cl_mem) malloc(sizeof(struct _cl_mem));
  //obj->dispatch = dispatchTable;
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}


CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage2D(cl_context              context ,
                cl_mem_flags            flags ,
                const cl_image_format * image_format ,
                size_t                  image_width ,
                size_t                  image_height ,
                size_t                  image_row_pitch ,
                void *                  host_ptr ,
                cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  //cl_mem obj = (cl_mem) malloc(sizeof(struct _cl_mem));
  //obj->dispatch = dispatchTable;
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage3D(cl_context              context,
                cl_mem_flags            flags,
                const cl_image_format * image_format,
                size_t                  image_width,
                size_t                  image_height ,
                size_t                  image_depth ,
                size_t                  image_row_pitch ,
                size_t                  image_slice_pitch ,
                void *                  host_ptr ,
                cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  //cl_mem obj = (cl_mem) malloc(sizeof(struct _cl_mem));
  //obj->dispatch = dispatchTable;
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
  if (!memobj)
  {
    return CL_INVALID_MEM_OBJECT;
  }

  memobj->refCount++;
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
  if (!memobj)
  {
    return CL_INVALID_MEM_OBJECT;
  }

  if (--memobj->refCount == 0)
  {
    while (!memobj->callbacks->empty())
    {
      memobj->callbacks->top()(memobj, memobj->data->top());
      memobj->callbacks->pop();
      memobj->data->pop();
    }
    free(memobj->callbacks);
    free(memobj->data);
    free(memobj);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetSupportedImageFormats(cl_context           context,
                           cl_mem_flags         flags,
                           cl_mem_object_type   image_type ,
                           cl_uint              num_entries ,
                           cl_image_format *    image_formats ,
                           cl_uint *            num_image_formats) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetMemObjectInfo(cl_mem            memobj ,
                   cl_mem_info       param_name ,
                   size_t            param_value_size ,
                   void *            param_value ,
                   size_t *          param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check mem object is valid
  if (!memobj)
  {
    return CL_INVALID_MEM_OBJECT;
  }

  switch (param_name)
  {
  case CL_MEM_TYPE:
    result_size = sizeof(cl_mem_object_type);
    result_data = new cl_mem_object_type(CL_MEM_OBJECT_BUFFER);
    break;
  case CL_MEM_FLAGS:
    result_size = sizeof(cl_mem_flags);
    result_data = new cl_mem_flags(memobj->flags);
    break;
  case CL_MEM_SIZE:
    result_size = sizeof(size_t);
    result_data = new size_t(memobj->size);
    break;
  case CL_MEM_HOST_PTR:
    result_size = sizeof(void*);
    result_data = new void*(NULL);
  case CL_MEM_MAP_COUNT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(0);
    break;
  case CL_MEM_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(memobj->refCount);
    break;
  case CL_MEM_CONTEXT:
    result_size = sizeof(cl_context);
    result_data = new cl_context(memobj->context);
    break;
  case CL_MEM_ASSOCIATED_MEMOBJECT:
    result_size = sizeof(cl_mem);
    result_data = new cl_mem(NULL);
    break;
  case CL_MEM_OFFSET:
    result_size = sizeof(size_t);
    result_data = new size_t(0);
    break;
  default:
    return CL_INVALID_VALUE;
  }

  cl_int return_value = CL_SUCCESS;
  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      return_value = CL_INVALID_VALUE;
    }
    else
    {
      memcpy(param_value, result_data, result_size);
    }
  }

  if (param_value_size_ret)
  {
    *param_value_size_ret = result_size;
  }

  free(result_data);

  return return_value;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetImageInfo(cl_mem            image ,
               cl_image_info     param_name ,
               size_t            param_value_size ,
               void *            param_value ,
               size_t *          param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetMemObjectDestructorCallback(cl_mem  memobj ,
                                 void (CL_CALLBACK * pfn_notify)(cl_mem  memobj , void* user_data),
                                 void * user_data)             CL_API_SUFFIX__VERSION_1_1
{
  // Check parameters
  if (!memobj)
  {
    return CL_INVALID_MEM_OBJECT;
  }
  if (!pfn_notify)
  {
    return CL_INVALID_VALUE;
  }

  memobj->callbacks->push(pfn_notify);
  memobj->data->push(user_data);

  return CL_SUCCESS;
}

/* Sampler APIs  */
CL_API_ENTRY cl_sampler CL_API_CALL
clCreateSampler(cl_context           context ,
                cl_bool              normalized_coords ,
                cl_addressing_mode   addressing_mode ,
                cl_filter_mode       filter_mode ,
                cl_int *             errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  //cl_sampler obj = (cl_sampler) malloc(sizeof(struct _cl_sampler));
  //obj->dispatch = dispatchTable;
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainSampler(cl_sampler  sampler) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseSampler(cl_sampler  sampler) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetSamplerInfo(cl_sampler          sampler ,
                 cl_sampler_info     param_name ,
                 size_t              param_value_size ,
                 void *              param_value ,
                 size_t *            param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

/* Program Object APIs  */
CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithSource(cl_context         context ,
                          cl_uint            count ,
                          const char **      strings ,
                          const size_t *     lengths ,
                          cl_int *           errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!context)
  {
    ERRCODE(CL_INVALID_CONTEXT);
    return NULL;
  }
  if (count == 0 || !strings || !strings[0])
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }

  // Concatenate sources into a single string
  std::string source;
  for (int i = 0; i < count; i++)
  {
    size_t length = (lengths && lengths[i]) ? lengths[i] : strlen(strings[i]);
    source.append(strings[i], length);
  }

  // Create program object
  cl_program prog = (cl_program)malloc(sizeof(struct _cl_program));
  prog->dispatch = m_dispatchTable;
  prog->program = new spirsim::Program(source);
  prog->context = context;
  prog->refCount = 1;
  if (!prog->program)
  {
    ERRCODE(CL_OUT_OF_HOST_MEMORY);
    free(prog);
    return NULL;
  }

  ERRCODE(CL_SUCCESS);
  return prog;
}

CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithBinary(cl_context                      context ,
                          cl_uint                         num_devices ,
                          const cl_device_id *            device_list ,
                          const size_t *                  lengths ,
                          const unsigned char **          binaries ,
                          cl_int *                        binary_status ,
                          cl_int *                        errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!context)
  {
    ERRCODE(CL_INVALID_CONTEXT);
    return NULL;
  }
  if (num_devices != 1 || !device_list || !lengths || !binaries)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }
  if (device_list[0] != m_device)
  {
    ERRCODE(CL_INVALID_DEVICE);
    return NULL;
  }

  // Create program object
  cl_program prog = (cl_program)malloc(sizeof(struct _cl_program));
  prog->dispatch = m_dispatchTable;
  prog->program = spirsim::Program::createFromBitcode(binaries[0], lengths[0]);
  prog->context = context;
  prog->refCount = 1;
  if (!prog->program)
  {
    ERRCODE(CL_INVALID_BINARY);
    if (binary_status)
    {
      binary_status[0] = CL_INVALID_BINARY;
    }
    free(prog);
    return NULL;
  }
  if (binary_status)
  {
    binary_status[0] = CL_SUCCESS;
  }

  ERRCODE(CL_SUCCESS);
  return prog;
}

CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithBuiltInKernels(cl_context             context ,
                                  cl_uint                num_devices ,
                                  const cl_device_id *   device_list ,
                                  const char *           kernel_names ,
                                  cl_int *               errcode_ret) CL_API_SUFFIX__VERSION_1_2
{
  //cl_program obj = (cl_program) malloc(sizeof(struct _cl_program));
  //obj->dispatch = dispatchTable;
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainProgram(cl_program  program) CL_API_SUFFIX__VERSION_1_0
{
  if (!program)
  {
    return CL_INVALID_PROGRAM;
  }

  program->refCount++;
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseProgram(cl_program  program) CL_API_SUFFIX__VERSION_1_0
{
  if (!program)
  {
    return CL_INVALID_PROGRAM;
  }

  if (--program->refCount == 0)
  {
    delete program->program;
    free(program);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clBuildProgram(cl_program            program ,
               cl_uint               num_devices ,
               const cl_device_id *  device_list ,
               const char *          options ,
               void (CL_CALLBACK *   pfn_notify)(cl_program  program , void *  user_data),
               void *                user_data) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!program || !program->program)
  {
    return CL_INVALID_PROGRAM;
  }
  if ((num_devices > 0 && !device_list) ||
      (num_devices == 0 && device_list))
  {
    return CL_INVALID_VALUE;
  }
  if (!pfn_notify && user_data)
  {
    return CL_INVALID_VALUE;
  }
  if (device_list && !device_list[0])
  {
    return CL_INVALID_DEVICE;
  }

  // Build program
  if (!program->program->build(options))
  {
    return CL_BUILD_PROGRAM_FAILURE;
  }
  if (pfn_notify)
  {
    pfn_notify(program, user_data);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clUnloadCompiler(void) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clCompileProgram(cl_program            program ,
                 cl_uint               num_devices ,
                 const cl_device_id *  device_list ,
                 const char *          options ,
                 cl_uint               num_input_headers ,
                 const cl_program *    input_headers,
                 const char **         header_include_names ,
                 void (CL_CALLBACK *   pfn_notify)(cl_program  program , void *  user_data),
                 void *                user_data) CL_API_SUFFIX__VERSION_1_2
{
  //pfn_notify(program, NULL);
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_program CL_API_CALL
clLinkProgram(cl_context            context ,
              cl_uint               num_devices ,
              const cl_device_id *  device_list ,
              const char *          options ,
              cl_uint               num_input_programs ,
              const cl_program *    input_programs ,
              void (CL_CALLBACK *   pfn_notify)(cl_program  program , void *  user_data),
              void *                user_data ,
              cl_int *              errcode_ret) CL_API_SUFFIX__VERSION_1_2
{
  //cl_program obj = (cl_program) malloc(sizeof(cl_program));
  //obj->dispatch = dispatchTable;
  ////pfn_notify(obj, NULL);
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clUnloadPlatformCompiler(cl_platform_id  platform) CL_API_SUFFIX__VERSION_1_2
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetProgramInfo(cl_program          program ,
                 cl_program_info     param_name ,
                 size_t              param_value_size ,
                 void *              param_value ,
                 size_t *            param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check program is valid
  if (!program)
  {
    return CL_INVALID_PROGRAM;
  }
  if ((param_name == CL_PROGRAM_NUM_KERNELS ||
       param_name == CL_PROGRAM_KERNEL_NAMES) &&
      program->program->getBuildStatus() != CL_BUILD_SUCCESS)
  {
    return CL_INVALID_PROGRAM_EXECUTABLE;
  }

  switch (param_name)
  {
  case CL_PROGRAM_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(program->refCount);
    break;
  case CL_PROGRAM_CONTEXT:
    result_size = sizeof(cl_context);
    result_data = new cl_context(program->context);
    break;
  case CL_PROGRAM_NUM_DEVICES:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(1);
    break;
  case CL_PROGRAM_DEVICES:
    result_size = sizeof(cl_device_id);
    result_data = new cl_device_id(m_device);
    break;
  case CL_PROGRAM_SOURCE:
    result_data = strdup(program->program->getSource().c_str());
    result_size = (strlen((char*)result_data)+1)*sizeof(char);
    break;
  case CL_PROGRAM_BINARY_SIZES:
    result_size = sizeof(size_t);
    result_data = new size_t(program->program->getBinarySize());
    break;
  case CL_PROGRAM_BINARIES:
    result_size = sizeof(unsigned char*);
    result_data = program->program->getBinary();
    break;
  case CL_PROGRAM_NUM_KERNELS:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(program->program->getNumKernels());
    break;
  case CL_PROGRAM_KERNEL_NAMES:
  {
    list<string> names = program->program->getKernelNames();
    string ret;
    for (list<string>::iterator itr = names.begin(); itr != names.end(); itr++)
    {
      ret += *itr;
      ret += ";";
    }
    if (!ret.empty())
    {
      ret.erase(ret.length()-1);
    }
    result_data = strdup(ret.c_str());
    result_size = (strlen((char*)result_data)+1)*sizeof(char);
    break;
  }
  default:
    return CL_INVALID_VALUE;
  }

  cl_int return_value = CL_SUCCESS;
  if (param_value)
  {
    if (param_name == CL_PROGRAM_BINARIES)
    {
      memcpy(((unsigned char**)param_value)[0],
             result_data, program->program->getBinarySize());
    }
    else
    {
      // Check destination is large enough
      if (param_value_size < result_size)
      {
        return_value = CL_INVALID_VALUE;
      }
      else
      {
        memcpy(param_value, result_data, result_size);
      }
    }
  }

  if (param_value_size_ret)
  {
    *param_value_size_ret = result_size;
  }

  free(result_data);

  return return_value;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetProgramBuildInfo(cl_program             program ,
                      cl_device_id           device ,
                      cl_program_build_info  param_name ,
                      size_t                 param_value_size ,
                      void *                 param_value ,
                      size_t *               param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check program is valid
  if (!program)
  {
    return CL_INVALID_PROGRAM;
  }

  switch (param_name)
  {
  case CL_PROGRAM_BUILD_STATUS:
    result_size = sizeof(cl_build_status);
    result_data = new cl_build_status(program->program->getBuildStatus());
    break;
  case CL_PROGRAM_BUILD_OPTIONS:
    result_data = strdup(program->program->getBuildOptions().c_str());
    result_size = (strlen((char*)result_data)+1) * sizeof(char);
    break;
  case CL_PROGRAM_BUILD_LOG:
    result_data = strdup(program->program->getBuildLog().c_str());
    result_size = (strlen((char*)result_data)+1) * sizeof(char);
    break;
  case CL_PROGRAM_BINARY_TYPE:
    result_size = sizeof(cl_program_binary_type);
    result_data = new cl_program_binary_type(
      CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT);
    break;
  default:
    return CL_INVALID_VALUE;
  }

  cl_int return_value = CL_SUCCESS;
  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      return_value = CL_INVALID_VALUE;
    }
    else
    {
      memcpy(param_value, result_data, result_size);
    }
  }

  if (param_value_size_ret)
  {
    *param_value_size_ret = result_size;
  }

  free(result_data);

  return return_value;
}


/* Kernel Object APIs */
CL_API_ENTRY cl_kernel CL_API_CALL
clCreateKernel(cl_program       program ,
               const char *     kernel_name ,
               cl_int *         errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (program->dispatch != m_dispatchTable)
  {
    ERRCODE(CL_INVALID_PROGRAM);
    return NULL;
  }
  if (!kernel_name)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }

  // Create kernel object
  cl_kernel kernel = (cl_kernel)malloc(sizeof(struct _cl_kernel));
  kernel->dispatch = m_dispatchTable;
  kernel->kernel = program->program->createKernel(kernel_name);
  kernel->program = program;
  kernel->refCount = 1;
  if (!kernel->kernel)
  {
    ERRCODE(CL_INVALID_KERNEL_NAME);
    free(kernel);
    return NULL;
  }

  ERRCODE(CL_SUCCESS);
  return kernel;
}

CL_API_ENTRY cl_int CL_API_CALL
clCreateKernelsInProgram(cl_program      program ,
                         cl_uint         num_kernels ,
                         cl_kernel *     kernels ,
                         cl_uint *       num_kernels_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!program)
  {
    return CL_INVALID_PROGRAM;
  }
  if (program->program->getBuildStatus() != CL_BUILD_SUCCESS)
  {
    return CL_INVALID_PROGRAM_EXECUTABLE;
  }

  unsigned int num = program->program->getNumKernels();
  if (kernels && num_kernels < num)
  {
    return CL_INVALID_VALUE;
  }

  if (kernels)
  {
    int i = 0;
    list<string> names = program->program->getKernelNames();
    for (list<string>::iterator itr = names.begin(); itr != names.end(); itr++)
    {
      cl_kernel kernel = (cl_kernel)malloc(sizeof(struct _cl_kernel));
      kernel->dispatch = m_dispatchTable;
      kernel->kernel = program->program->createKernel(*itr);
      kernel->program = program;
      kernel->refCount = 1;
      kernels[i++] = kernel;
    }
  }

  if (num_kernels_ret)
  {
    *num_kernels_ret = num;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainKernel(cl_kernel     kernel) CL_API_SUFFIX__VERSION_1_0
{
  if (!kernel)
  {
    return CL_INVALID_KERNEL;
  }

  kernel->refCount++;
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseKernel(cl_kernel    kernel) CL_API_SUFFIX__VERSION_1_0
{
  if (!kernel)
  {
    return CL_INVALID_KERNEL;
  }

  if (--kernel->refCount == 0)
  {
    delete kernel->kernel;
    free(kernel);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetKernelArg(cl_kernel     kernel ,
               cl_uint       arg_index ,
               size_t        arg_size ,
               const void *  arg_value) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (arg_index >= kernel->kernel->getNumArguments())
  {
    return CL_INVALID_ARG_INDEX;
  }

  unsigned int type = kernel->kernel->getArgumentType(arg_index);

  if (kernel->kernel->getArgumentSize(arg_index) != arg_size
      && type != CL_KERNEL_ARG_ADDRESS_LOCAL)
  {
    return CL_INVALID_ARG_SIZE;
  }

  // Prepare argument value
  spirsim::TypedValue value;
  value.size = arg_size;
  switch (type)
  {
  case CL_KERNEL_ARG_ADDRESS_PRIVATE:
    value.data = (unsigned char*)arg_value;
    break;
  case CL_KERNEL_ARG_ADDRESS_LOCAL:
    value.data = NULL;
    break;
  case CL_KERNEL_ARG_ADDRESS_GLOBAL:
  case CL_KERNEL_ARG_ADDRESS_CONSTANT:
    value.data = (unsigned char*)&(*(cl_mem*)arg_value)->address;
    break;
  default:
    return CL_INVALID_ARG_VALUE;
  }

  // Set argument
  kernel->kernel->setArgument(arg_index, value);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelInfo(cl_kernel        kernel ,
                cl_kernel_info   param_name ,
                size_t           param_value_size ,
                void *           param_value ,
                size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check kernel is valid
  if (!kernel)
  {
    return CL_INVALID_KERNEL;
  }

  switch (param_name)
  {
  case CL_KERNEL_FUNCTION_NAME:
    result_data = strdup(kernel->kernel->getName().c_str());
    result_size = (strlen((char*)result_data)+1)*sizeof(char);
    break;
  case CL_KERNEL_NUM_ARGS:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(kernel->kernel->getNumArguments());
    break;
  case CL_KERNEL_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(kernel->refCount);
    break;
  case CL_KERNEL_CONTEXT:
    result_size = sizeof(cl_context);
    result_data = new cl_context(kernel->program->context);
    break;
  case CL_KERNEL_PROGRAM:
    result_size = sizeof(cl_program);
    result_data = new cl_program(kernel->program);
    break;
  case CL_KERNEL_ATTRIBUTES:
    result_data = strdup(""); // TODO: Attributes
    result_size = (strlen((char*)result_data)+1)*sizeof(char);
    break;
  default:
    return CL_INVALID_VALUE;
  }

  cl_int return_value = CL_SUCCESS;
  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      return_value = CL_INVALID_VALUE;
    }
    else
    {
      memcpy(param_value, result_data, result_size);
    }
  }

  if (param_value_size_ret)
  {
    *param_value_size_ret = result_size;
  }

  free(result_data);

  return return_value;

}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelArgInfo(cl_kernel        kernel ,
                   cl_uint          arg_indx ,
                   cl_kernel_arg_info   param_name ,
                   size_t           param_value_size ,
                   void *           param_value ,
                   size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_2
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelWorkGroupInfo(cl_kernel                   kernel ,
                         cl_device_id                device ,
                         cl_kernel_work_group_info   param_name ,
                         size_t                      param_value_size ,
                         void *                      param_value ,
                         size_t *                    param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check parameters is valid
  if (!kernel)
  {
    return CL_INVALID_KERNEL;
  }
  if (!device || device != m_device)
  {
    return CL_INVALID_DEVICE;
  }

  switch (param_name)
  {
  case CL_KERNEL_GLOBAL_WORK_SIZE:
    return CL_INVALID_VALUE;
  case CL_KERNEL_WORK_GROUP_SIZE:
    result_size = sizeof(size_t);
    result_data = new size_t(MAX_WI_SIZE);
    break;
  case CL_KERNEL_COMPILE_WORK_GROUP_SIZE:
    result_size = sizeof(size_t[3]);
    result_data = new size_t[3];
    memcpy(result_data, kernel->kernel->getRequiredWorkGroupSize(),
           result_size);
    break;
  case CL_KERNEL_LOCAL_MEM_SIZE:
    result_size = sizeof(cl_ulong);
    result_data = new cl_ulong(kernel->kernel->getLocalMemorySize());
    break;
  case CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE:
    result_size = sizeof(size_t);
    result_data = new size_t(1);
    break;
  case CL_KERNEL_PRIVATE_MEM_SIZE:
    result_size = sizeof(cl_ulong);
    result_data = new cl_ulong(0); // TODO: Real value
    break;
  default:
    return CL_INVALID_VALUE;
  }

  cl_int return_value = CL_SUCCESS;
  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      return_value = CL_INVALID_VALUE;
    }
    else
    {
      memcpy(param_value, result_data, result_size);
    }
  }

  if (param_value_size_ret)
  {
    *param_value_size_ret = result_size;
  }

  free(result_data);

  return return_value;
}

/* Event Object APIs  */
CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents(cl_uint              num_events ,
                const cl_event *     event_list) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!num_events || !event_list)
  {
    return CL_INVALID_VALUE;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetEventInfo(cl_event          event ,
               cl_event_info     param_name ,
               size_t            param_value_size ,
               void *            param_value ,
               size_t *          param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check event is valid
  if (!event)
  {
    return CL_INVALID_EVENT;
  }

  switch (param_name)
  {
  case CL_EVENT_COMMAND_QUEUE:
    result_size = sizeof(cl_command_queue);
    result_data = new cl_command_queue(event->queue);
    break;
  case CL_EVENT_CONTEXT:
    result_size = sizeof(cl_context);
    result_data = new cl_context(event->queue->context);
    break;
  case CL_EVENT_COMMAND_TYPE:
    result_size = sizeof(cl_command_type);
    result_data = new cl_command_type(event->type);
    break;
  case CL_EVENT_COMMAND_EXECUTION_STATUS:
    result_size = sizeof(cl_int);
    result_data = new cl_int(CL_COMPLETE);
    break;
  case CL_EVENT_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = new cl_uint(event->refCount);
    break;
  default:
    return CL_INVALID_VALUE;
  }

  cl_int return_value = CL_SUCCESS;
  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      return_value = CL_INVALID_VALUE;
    }
    else
    {
      memcpy(param_value, result_data, result_size);
    }
  }

  if (param_value_size_ret)
  {
    *param_value_size_ret = result_size;
  }

  free(result_data);

  return return_value;
}

CL_API_ENTRY cl_event CL_API_CALL
clCreateUserEvent(cl_context     context ,
                  cl_int *       errcode_ret) CL_API_SUFFIX__VERSION_1_1
{
  //cl_event obj = (cl_event) malloc(sizeof(struct _cl_event));
  //obj->dispatch = dispatchTable;
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainEvent(cl_event  event) CL_API_SUFFIX__VERSION_1_0
{
  if (!event)
  {
    return CL_INVALID_EVENT;
  }

  event->refCount++;

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseEvent(cl_event  event) CL_API_SUFFIX__VERSION_1_0
{
  if (!event)
  {
    return CL_INVALID_EVENT;
  }

  if (--event->refCount == 0)
  {
    free(event);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetUserEventStatus(cl_event    event ,
                     cl_int      execution_status) CL_API_SUFFIX__VERSION_1_1
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetEventCallback(cl_event     event ,
                   cl_int       command_exec_callback_type ,
                   void (CL_CALLBACK *  pfn_notify)(cl_event, cl_int, void *),
                   void *       user_data) CL_API_SUFFIX__VERSION_1_1
{
  //pfn_notify(event, command_exec_callback_type, NULL);
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

/* Profiling APIs  */
CL_API_ENTRY cl_int CL_API_CALL
clGetEventProfilingInfo(cl_event             event ,
                        cl_profiling_info    param_name ,
                        size_t               param_value_size ,
                        void *               param_value ,
                        size_t *             param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}


/* Flush and Finish APIs */
CL_API_ENTRY cl_int CL_API_CALL
clFlush(cl_command_queue  command_queue) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clFinish(cl_command_queue  command_queue) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  return CL_SUCCESS;
}


/* Enqueued Commands APIs */
CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBuffer(cl_command_queue     command_queue ,
                    cl_mem               buffer ,
                    cl_bool              blocking_read ,
                    size_t               offset ,
                    size_t               cb ,
                    void *               ptr ,
                    cl_uint              num_events_in_wait_list ,
                    const cl_event *     event_wait_list ,
                    cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  // Perform read
  spirsim::Memory *memory = command_queue->context->device->getGlobalMemory();
  bool ret = memory->load(buffer->address, cb, (unsigned char*)ptr);
  if (!ret)
  {
    return CL_INVALID_VALUE;
  }

  // Create event
  if (event)
  {
    cl_event evt = (cl_event)malloc(sizeof(struct _cl_event));
    evt->dispatch = m_dispatchTable;
    evt->queue = command_queue;
    evt->type = CL_COMMAND_READ_BUFFER;
    evt->refCount = 1;
    *event = evt;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBufferRect(cl_command_queue     command_queue ,
                        cl_mem               buffer ,
                        cl_bool              blocking_read ,
                        const size_t *       buffer_origin ,
                        const size_t *       host_origin ,
                        const size_t *       region ,
                        size_t               buffer_row_pitch ,
                        size_t               buffer_slice_pitch ,
                        size_t               host_row_pitch ,
                        size_t               host_slice_pitch ,
                        void *               ptr ,
                        cl_uint              num_events_in_wait_list ,
                        const cl_event *     event_wait_list ,
                        cl_event *           event) CL_API_SUFFIX__VERSION_1_1
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBuffer(cl_command_queue    command_queue ,
                     cl_mem              buffer ,
                     cl_bool             blocking_write ,
                     size_t              offset ,
                     size_t              cb ,
                     const void *        ptr ,
                     cl_uint             num_events_in_wait_list ,
                     const cl_event *    event_wait_list ,
                     cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  // Perform write
  spirsim::Memory *memory = command_queue->context->device->getGlobalMemory();
  bool ret = memory->store(buffer->address, cb, (const unsigned char*)ptr);
  if (!ret)
  {
    return CL_INVALID_VALUE;
  }

  // Create event
  if (event)
  {
    cl_event evt = (cl_event)malloc(sizeof(struct _cl_event));
    evt->dispatch = m_dispatchTable;
    evt->queue = command_queue;
    evt->type = CL_COMMAND_WRITE_BUFFER;
    evt->refCount = 1;
    *event = evt;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBufferRect(cl_command_queue     command_queue ,
                         cl_mem               buffer ,
                         cl_bool              blocking_write ,
                         const size_t *       buffer_origin ,
                         const size_t *       host_origin ,
                         const size_t *       region ,
                         size_t               buffer_row_pitch ,
                         size_t               buffer_slice_pitch ,
                         size_t               host_row_pitch ,
                         size_t               host_slice_pitch ,
                         const void *         ptr ,
                         cl_uint              num_events_in_wait_list ,
                         const cl_event *     event_wait_list ,
                         cl_event *           event) CL_API_SUFFIX__VERSION_1_1
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBuffer(cl_command_queue     command_queue ,
                    cl_mem               src_buffer ,
                    cl_mem               dst_buffer ,
                    size_t               src_offset ,
                    size_t               dst_offset ,
                    size_t               cb ,
                    cl_uint              num_events_in_wait_list ,
                    const cl_event *     event_wait_list ,
                    cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferRect(cl_command_queue     command_queue ,
                        cl_mem               src_buffer ,
                        cl_mem               dst_buffer ,
                        const size_t *       src_origin ,
                        const size_t *       dst_origin ,
                        const size_t *       region ,
                        size_t               src_row_pitch ,
                        size_t               src_slice_pitch ,
                        size_t               dst_row_pitch ,
                        size_t               dst_slice_pitch ,
                        cl_uint              num_events_in_wait_list ,
                        const cl_event *     event_wait_list ,
                        cl_event *           event) CL_API_SUFFIX__VERSION_1_1
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueFillBuffer(cl_command_queue    command_queue ,
                    cl_mem              buffer ,
                    const void *        pattern ,
                    size_t              pattern_size ,
                    size_t              offset ,
                    size_t              cb ,
                    cl_uint             num_events_in_wait_list ,
                    const cl_event *    event_wait_list ,
                    cl_event *          event) CL_API_SUFFIX__VERSION_1_2
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueFillImage(cl_command_queue    command_queue ,
                   cl_mem              image ,
                   const void *        fill_color ,
                   const size_t *      origin ,
                   const size_t *      region ,
                   cl_uint             num_events_in_wait_list ,
                   const cl_event *    event_wait_list ,
                   cl_event *          event) CL_API_SUFFIX__VERSION_1_2
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadImage(cl_command_queue      command_queue ,
                   cl_mem                image ,
                   cl_bool               blocking_read ,
                   const size_t *        origin ,
                   const size_t *        region ,
                   size_t                row_pitch ,
                   size_t                slice_pitch ,
                   void *                ptr ,
                   cl_uint               num_events_in_wait_list ,
                   const cl_event *      event_wait_list ,
                   cl_event *            event) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteImage(cl_command_queue     command_queue ,
                    cl_mem               image ,
                    cl_bool              blocking_write ,
                    const size_t *       origin ,
                    const size_t *       region ,
                    size_t               input_row_pitch ,
                    size_t               input_slice_pitch ,
                    const void *         ptr ,
                    cl_uint              num_events_in_wait_list ,
                    const cl_event *     event_wait_list ,
                    cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImage(cl_command_queue      command_queue ,
                   cl_mem                src_image ,
                   cl_mem                dst_image ,
                   const size_t *        src_origin ,
                   const size_t *        dst_origin ,
                   const size_t *        region ,
                   cl_uint               num_events_in_wait_list ,
                   const cl_event *      event_wait_list ,
                   cl_event *            event) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImageToBuffer(cl_command_queue  command_queue ,
                           cl_mem            src_image ,
                           cl_mem            dst_buffer ,
                           const size_t *    src_origin ,
                           const size_t *    region ,
                           size_t            dst_offset ,
                           cl_uint           num_events_in_wait_list ,
                           const cl_event *  event_wait_list ,
                           cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferToImage(cl_command_queue  command_queue ,
                           cl_mem            src_buffer ,
                           cl_mem            dst_image ,
                           size_t            src_offset ,
                           const size_t *    dst_origin ,
                           const size_t *    region ,
                           cl_uint           num_events_in_wait_list ,
                           const cl_event *  event_wait_list ,
                           cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY void * CL_API_CALL
clEnqueueMapBuffer(cl_command_queue  command_queue ,
                   cl_mem            buffer ,
                   cl_bool           blocking_map ,
                   cl_map_flags      map_flags ,
                   size_t            offset ,
                   size_t            cb ,
                   cl_uint           num_events_in_wait_list ,
                   const cl_event *  event_wait_list ,
                   cl_event *        event ,
                   cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY void * CL_API_CALL
clEnqueueMapImage(cl_command_queue   command_queue ,
                  cl_mem             image ,
                  cl_bool            blocking_map ,
                  cl_map_flags       map_flags ,
                  const size_t *     origin ,
                  const size_t *     region ,
                  size_t *           image_row_pitch ,
                  size_t *           image_slice_pitch ,
                  cl_uint            num_events_in_wait_list ,
                  const cl_event *   event_wait_list ,
                  cl_event *         event ,
                  cl_int *           errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueUnmapMemObject(cl_command_queue  command_queue ,
                        cl_mem            memobj ,
                        void *            mapped_ptr ,
                        cl_uint           num_events_in_wait_list ,
                        const cl_event *   event_wait_list ,
                        cl_event *         event) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMigrateMemObjects(cl_command_queue        command_queue ,
                           cl_uint                 num_mem_objects ,
                           const cl_mem *          mem_objects ,
                           cl_mem_migration_flags  flags ,
                           cl_uint                 num_events_in_wait_list ,
                           const cl_event *        event_wait_list ,
                           cl_event *              event) CL_API_SUFFIX__VERSION_1_2
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNDRangeKernel(cl_command_queue  command_queue ,
                       cl_kernel         kernel ,
                       cl_uint           work_dim ,
                       const size_t *    global_work_offset ,
                       const size_t *    global_work_size ,
                       const size_t *    local_work_size ,
                       cl_uint           num_events_in_wait_list ,
                       const cl_event *  event_wait_list ,
                       cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (work_dim < 1 || work_dim > 3)
  {
    return CL_INVALID_WORK_DIMENSION;
  }
  if (!global_work_size)
  {
    return CL_INVALID_GLOBAL_WORK_SIZE;
  }

  // Prepare 3D range
  size_t global[3] = {1,1,1};
  size_t local[3] = {1,1,1};
  for (int i = 0; i < work_dim; i++)
  {
    global[i] = global_work_size[i];
    if (local_work_size)
    {
      local[i] = local_work_size[i];
      if (global[i] % local[i])
      {
        return CL_INVALID_WORK_GROUP_SIZE;
      }
    }
  }

  // Run kernel
  command_queue->context->device->run(*kernel->kernel, global, local);

  // Create event
  if (event)
  {
    cl_event evt = (cl_event)malloc(sizeof(struct _cl_event));
    evt->dispatch = m_dispatchTable;
    evt->queue = command_queue;
    evt->type = CL_COMMAND_NDRANGE_KERNEL;
    evt->refCount = 1;
    *event = evt;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueTask(cl_command_queue   command_queue ,
              cl_kernel          kernel ,
              cl_uint            num_events_in_wait_list ,
              const cl_event *   event_wait_list ,
              cl_event *         event) CL_API_SUFFIX__VERSION_1_0
{
  size_t work = 1;
  return clEnqueueNDRangeKernel(command_queue, kernel, 1,
                                NULL, &work, &work,
                                num_events_in_wait_list,
                                event_wait_list,
                                event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNativeKernel(cl_command_queue   command_queue ,
                      void (CL_CALLBACK *user_func)(void *),
                      void *             args ,
                      size_t             cb_args ,
                      cl_uint            num_mem_objects ,
                      const cl_mem *     mem_list ,
                      const void **      args_mem_loc ,
                      cl_uint            num_events_in_wait_list ,
                      const cl_event *   event_wait_list ,
                      cl_event *         event) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY void * CL_API_CALL
clGetExtensionFunctionAddressForPlatform(cl_platform_id  platform ,
                                         const char *    func_name) CL_API_SUFFIX__VERSION_1_2
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMarkerWithWaitList(cl_command_queue  command_queue ,
                            cl_uint            num_events_in_wait_list ,
                            const cl_event *   event_wait_list ,
                            cl_event *         event) CL_API_SUFFIX__VERSION_1_2

{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBarrierWithWaitList(cl_command_queue  command_queue ,
                             cl_uint            num_events_in_wait_list ,
                             const cl_event *   event_wait_list ,
                             cl_event *         event) CL_API_SUFFIX__VERSION_1_2
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

extern CL_API_ENTRY cl_int CL_API_CALL
clSetPrintfCallback(cl_context           context ,
                    void (CL_CALLBACK *  pfn_notify)(cl_context  program ,
                                                     cl_uint printf_data_len ,
                                                     char *  printf_data_ptr ,
                                                     void *  user_data),
                    void *               user_data) CL_API_SUFFIX__VERSION_1_2
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMarker(cl_command_queue     command_queue ,
                cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWaitForEvents(cl_command_queue  command_queue ,
                       cl_uint           num_events ,
                       const cl_event *  event_list) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBarrier(cl_command_queue  command_queue) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}


#define SIZE_T_MAX (size_t) 0xFFFFFFFFFFFFFFFFULL

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLBuffer(cl_context      context ,
                     cl_mem_flags    flags ,
                     cl_GLuint       bufret_mem ,
                     int *           errcode_ret ) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLTexture(cl_context       context ,
                      cl_mem_flags     flags ,
                      cl_GLenum        target ,
                      cl_GLint         miplevel ,
                      cl_GLuint        texture ,
                      cl_int *         errcode_ret ) CL_API_SUFFIX__VERSION_1_2
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLTexture2D(cl_context       context,
                        cl_mem_flags     flags,
                        cl_GLenum        target,
                        cl_GLint         miplevel,
                        cl_GLuint        texture,
                        cl_int *         errcode_ret ) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLTexture3D(cl_context       context,
                        cl_mem_flags     flags,
                        cl_GLenum        target,
                        cl_GLint         miplevel,
                        cl_GLuint        texture,
                        cl_int *         errcode_ret ) CL_API_SUFFIX__VERSION_1_0

{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLRenderbuffer(cl_context    context,
                           cl_mem_flags  flags,
                           cl_GLuint     renderbuffer,
                           cl_int *      errcode_ret ) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetGLObjectInfo(cl_mem                 memobj,
                  cl_gl_object_type *    gl_object_type,
                  cl_GLuint *            gl_object_name ) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetGLTextureInfo(cl_mem                memobj,
                   cl_gl_texture_info    param_name,
                   size_t                param_value_size,
                   void *                param_value,
                   size_t *              param_value_size_ret ) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueAcquireGLObjects(cl_command_queue       command_queue,
                          cl_uint                num_objects,
                          const cl_mem *         mem_objects,
                          cl_uint                num_events_in_wait_list,
                          const cl_event *       event_wait_list,
                          cl_event *             event ) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReleaseGLObjects(cl_command_queue       command_queue,
                          cl_uint                num_objects,
                          const cl_mem *         mem_objects,
                          cl_uint                num_events_in_wait_list,
                          const cl_event *       event_wait_list,
                          cl_event *             event ) CL_API_SUFFIX__VERSION_1_0

{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetGLContextInfoKHR(const cl_context_properties *  properties,
                      cl_gl_context_info             param_name,
                      size_t                         param_value_size,
                      void *                         param_value,
                      size_t *                       param_value_size_ret ) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_event CL_API_CALL
clCreateEventFromGLsyncKHR(cl_context            context ,
                           cl_GLsync             cl_GLsync ,
                           cl_int *              errcode_ret ) //CL_EXT_SUFFIX__VERSION_1_1

{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  ERRCODE(CL_INVALID_PLATFORM);
  return NULL;
}
