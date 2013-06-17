#include <cstring>
#include <iostream>

#include "icd.h"

#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <CL/cl_gl_ext.h>

using namespace std;

CLIicdDispatchTable *m_dispatchTable = NULL;
struct _cl_platform_id *m_platform = NULL;
static struct _cl_device_id *m_device = NULL;
static struct _cl_context *m_context = NULL;
static struct _cl_command_queue *m_queue = NULL;

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformIDs(cl_uint           num_entries ,
                 cl_platform_id *  platforms ,
                 cl_uint *         num_platforms) CL_API_SUFFIX__VERSION_1_0
{
  return clIcdGetPlatformIDsKHR(num_entries, platforms, num_platforms);
}

#define DEVICE_NAME "SPIR Simulator"

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
  cl_int return_value = CL_SUCCESS;

  // Check parameters
  if (devices && num_entries < 1)
  {
    return_value = CL_INVALID_VALUE;
  }
  else if (devices)
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
    *num_devices = 1;
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
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_VENDOR_ID:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MAX_COMPUTE_UNITS:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MAX_WORK_GROUP_SIZE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MAX_WORK_ITEM_SIZES:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MAX_CLOCK_FREQUENCY:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_ADDRESS_BITS:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MAX_READ_IMAGE_ARGS:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_IMAGE2D_MAX_WIDTH:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_IMAGE3D_MAX_WIDTH:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_IMAGE3D_MAX_DEPTH:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_IMAGE_SUPPORT:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MAX_PARAMETER_SIZE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MAX_SAMPLERS:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_SINGLE_FP_CONFIG:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_GLOBAL_MEM_SIZE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_MAX_CONSTANT_ARGS:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_LOCAL_MEM_TYPE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_LOCAL_MEM_SIZE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_ENDIAN_LITTLE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_AVAILABLE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_COMPILER_AVAILABLE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_EXECUTION_CAPABILITIES:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_QUEUE_PROPERTIES:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_NAME:
    result_size = (strlen(DEVICE_NAME)+1)*sizeof(char);
    result_data = malloc(result_size);
    memcpy(result_data, DEVICE_NAME, result_size);
    break;
  case CL_DEVICE_VENDOR:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DRIVER_VERSION:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PROFILE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_VERSION:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_EXTENSIONS:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PLATFORM:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_DOUBLE_FP_CONFIG:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_HOST_UNIFIED_MEMORY:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_OPENCL_C_VERSION:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_LINKER_AVAILABLE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_BUILT_IN_KERNELS:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_IMAGE_MAX_BUFFER_SIZE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_IMAGE_MAX_ARRAY_SIZE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PARENT_DEVICE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PARTITION_MAX_SUB_DEVICES:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PARTITION_PROPERTIES:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PARTITION_AFFINITY_DOMAIN:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PARTITION_TYPE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_REFERENCE_COUNT:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PREFERRED_INTEROP_USER_SYNC:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
  case CL_DEVICE_PRINTF_BUFFER_SIZE:
    cerr << endl << "OCLGRIND: Unimplemented cl_device_info parameter." << endl;
    return CL_INVALID_VALUE;
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
  if (properties)
  {
    cerr << endl << "OCLGRIND: Non-NULL properties not supported." << endl;
    *errcode_ret = CL_INVALID_PLATFORM;
    return NULL;
  }
  if (num_devices != 1 || !devices)
  {
    *errcode_ret = CL_INVALID_VALUE;
    return NULL;
  }
  if (devices[0] != m_device)
  {
    *errcode_ret = CL_INVALID_DEVICE;
    return NULL;
  }
  if (pfn_notify)
  {
    cerr << endl << "OCLGRIND: Non-NULL pfn_notify not supported." << endl;
    *errcode_ret = CL_INVALID_VALUE;
    return NULL;
  }

  // Create context object
  if (!m_context)
  {
    m_context = (cl_context)malloc(sizeof(struct _cl_context));
    m_context->dispatch = m_dispatchTable;
  }

  *errcode_ret = CL_SUCCESS;
  return m_context;
}

CL_API_ENTRY cl_context CL_API_CALL
clCreateContextFromType(const cl_context_properties * properties,
                        cl_device_type                device_type,
                        void (CL_CALLBACK *     pfn_notify)(const char *, const void *, size_t, void *),
                        void *                        user_data,
                        cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  //cl_context obj = (cl_context) malloc(sizeof(struct _cl_context));
  //obj->dispatch = dispatchTable;
  //pfn_notify(NULL, NULL, 0, NULL);
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  *errcode_ret = CL_INVALID_PLATFORM;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
  if (context != m_context)
  {
    return CL_INVALID_CONTEXT;
  }

  // TODO: Reference count and retain
  free(m_context);
  m_context = NULL;

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetContextInfo(cl_context         context,
                 cl_context_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}


/* Command Queue APIs */
CL_API_ENTRY cl_command_queue CL_API_CALL
clCreateCommandQueue(cl_context                     context,
                     cl_device_id                   device,
                     cl_command_queue_properties    properties,
                     cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (context != m_context)
  {
    *errcode_ret = CL_INVALID_CONTEXT;
    return NULL;
  }
  if (device != m_device)
  {
    *errcode_ret = CL_INVALID_DEVICE;
    return NULL;
  }
  if (properties)
  {
    cerr << endl << "OCLGRIND: Non-NULL properties not supported." << endl;
    *errcode_ret = CL_INVALID_VALUE;
    return NULL;
  }

  // Create command-queue object
  if (!m_queue)
  {
    m_queue = (cl_command_queue)malloc(sizeof(struct _cl_command_queue));
    m_queue->dispatch = m_dispatchTable;
  }

  *errcode_ret = CL_SUCCESS;
  return m_queue;
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
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
  if (command_queue != m_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  // TODO: Reference count and retain
  free(m_queue);
  m_queue = NULL;

  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetCommandQueueInfo(cl_command_queue       command_queue ,
                      cl_command_queue_info  param_name ,
                      size_t                 param_value_size ,
                      void *                 param_value ,
                      size_t *               param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}


/* Memory Object APIs */
CL_API_ENTRY cl_mem CL_API_CALL
clCreateBuffer(cl_context    context ,
               cl_mem_flags  flags ,
               size_t        size ,
               void *        host_ptr ,
               cl_int *      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  //cl_mem obj = (cl_mem) malloc(sizeof(struct _cl_mem));
  //obj->dispatch = dispatchTable;
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  *errcode_ret = CL_INVALID_PLATFORM;
  return NULL;
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
  *errcode_ret = CL_INVALID_PLATFORM;
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
  *errcode_ret = CL_INVALID_PLATFORM;
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
  *errcode_ret = CL_INVALID_PLATFORM;
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
  *errcode_ret = CL_INVALID_PLATFORM;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
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
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
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
  //pfn_notify(memobj, NULL);
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
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
  *errcode_ret = CL_INVALID_PLATFORM;
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
  //cl_program obj = (cl_program) malloc(sizeof(struct _cl_program));
  //obj->dispatch = dispatchTable;
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  *errcode_ret = CL_INVALID_PLATFORM;
  return NULL;
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
  //cl_program obj = (cl_program) malloc(sizeof(struct _cl_program));
  //obj->dispatch = dispatchTable;
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  *errcode_ret = CL_INVALID_PLATFORM;
  return NULL;
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
  *errcode_ret = CL_INVALID_PLATFORM;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainProgram(cl_program  program) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseProgram(cl_program  program) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clBuildProgram(cl_program            program ,
               cl_uint               num_devices ,
               const cl_device_id *  device_list ,
               const char *          options ,
               void (CL_CALLBACK *   pfn_notify)(cl_program  program , void *  user_data),
               void *                user_data) CL_API_SUFFIX__VERSION_1_0
{
  //pfn_notify(program, NULL);
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
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
  *errcode_ret = CL_INVALID_PLATFORM;
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
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetProgramBuildInfo(cl_program             program ,
                      cl_device_id           device ,
                      cl_program_build_info  param_name ,
                      size_t                 param_value_size ,
                      void *                 param_value ,
                      size_t *               param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}


/* Kernel Object APIs */
CL_API_ENTRY cl_kernel CL_API_CALL
clCreateKernel(cl_program       program ,
               const char *     kernel_name ,
               cl_int *         errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  //cl_kernel obj = (cl_kernel) malloc(sizeof(struct _cl_kernel));
  //obj->dispatch = dispatchTable;
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  *errcode_ret = CL_INVALID_PLATFORM;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clCreateKernelsInProgram(cl_program      program ,
                         cl_uint         num_kernels ,
                         cl_kernel *     kernels ,
                         cl_uint *       num_kernels_ret) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainKernel(cl_kernel     kernel) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseKernel(cl_kernel    kernel) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetKernelArg(cl_kernel     kernel ,
               cl_uint       arg_index ,
               size_t        arg_size ,
               const void *  arg_value) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelInfo(cl_kernel        kernel ,
                cl_kernel_info   param_name ,
                size_t           param_value_size ,
                void *           param_value ,
                size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
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
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

/* Event Object APIs  */
CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents(cl_uint              num_events ,
                const cl_event *     event_list) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetEventInfo(cl_event          event ,
               cl_event_info     param_name ,
               size_t            param_value_size ,
               void *            param_value ,
               size_t *          param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_event CL_API_CALL
clCreateUserEvent(cl_context     context ,
                  cl_int *       errcode_ret) CL_API_SUFFIX__VERSION_1_1
{
  //cl_event obj = (cl_event) malloc(sizeof(struct _cl_event));
  //obj->dispatch = dispatchTable;
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  *errcode_ret = CL_INVALID_PLATFORM;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainEvent(cl_event  event) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseEvent(cl_event  event) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
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
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clFinish(cl_command_queue  command_queue) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
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
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
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
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
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
  *errcode_ret = CL_INVALID_PLATFORM;
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
  *errcode_ret = CL_INVALID_PLATFORM;
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
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueTask(cl_command_queue   command_queue ,
              cl_kernel          kernel ,
              cl_uint            num_events_in_wait_list ,
              const cl_event *   event_wait_list ,
              cl_event *         event) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  return CL_INVALID_PLATFORM;
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
  *errcode_ret = CL_INVALID_PLATFORM;
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
  *errcode_ret = CL_INVALID_PLATFORM;
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
  *errcode_ret = CL_INVALID_PLATFORM;
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
  *errcode_ret = CL_INVALID_PLATFORM;
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLRenderbuffer(cl_context    context,
                           cl_mem_flags  flags,
                           cl_GLuint     renderbuffer,
                           cl_int *      errcode_ret ) CL_API_SUFFIX__VERSION_1_0
{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  *errcode_ret = CL_INVALID_PLATFORM;
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
                           cl_int *              errcode_ret ) CL_EXT_SUFFIX__VERSION_1_1

{
  cerr << endl << "OCLGRIND: Unimplemented OpenCL API call " << __func__ << endl;
  *errcode_ret = CL_INVALID_PLATFORM;
  return NULL;
}
