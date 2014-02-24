// runtime.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sys/time.h>

#include "async_queue.h"
#include "icd.h"

#include <spirsim/Device.h>
#include <spirsim/Kernel.h>
#include <spirsim/Memory.h>
#include <spirsim/Program.h>
#include <spirsim/Queue.h>

using namespace std;

#define ERRCODE(err) if(errcode_ret){*errcode_ret = err;}
#define MAX_GLOBAL_MEM_SIZE 16 * 1048576 // 16 MB
#define MAX_LOCAL_MEM_SIZE 1048576 // 1 MB
#define MAX_WI_SIZE 65536

#define DEVICE_NAME "SPIR Simulator"
#define DEVICE_VENDOR "James Price, University of Bristol"
#define DEVICE_VENDOR_ID 0x0042
#define DEVICE_VERSION "OpenCL 1.2"
#define DRIVER_VERSION "0.1"
#define DEVICE_PROFILE "FULL_PROFILE"
#define DEVICE_EXTENSIONS "cl_khr_spir cl_khr_3d_image_writes \
  cl_khr_global_int32_base_atomics cl_khr_global_int32_extended_atomics \
  cl_khr_local_int32_base_atomics cl_khr_local_int32_extended_atomics \
  cl_khr_byte_addressable_store cl_khr_fp64"
#define DEVICE_SPIR_VERSIONS "1.2"

static struct _cl_platform_id *m_platform = NULL;
static struct _cl_device_id *m_device = NULL;

CL_API_ENTRY cl_int CL_API_CALL
clIcdGetPlatformIDsKHR
(
  cl_uint num_entries,
  cl_platform_id *platforms,
  cl_uint *num_platforms
)
{
  if (!m_platform)
  {
    m_platform = (cl_platform_id)malloc(sizeof(struct _cl_platform_id));
    m_platform->version = "OpenCL 1.2 Oclgrind";
    m_platform->vendor = "James Price, University of Bristol";
    m_platform->profile = "FULL_PROFILE";
    m_platform->name = "Oclgrind";
    m_platform->extensions = "cl_khr_icd";
    m_platform->suffix = "oclg";
    m_platform->dispatch = m_dispatchTable;
  }

  if (num_entries > 0)
  {
    platforms[0] = m_platform;
  }

  if (num_platforms)
  {
    *num_platforms = 1;
  }

  return CL_SUCCESS;
}

////////////////////////////////////
// OpenCL Runtime API Definitions //
////////////////////////////////////

#ifndef CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#endif

#ifndef CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#endif

CL_API_ENTRY void* CL_API_CALL
clGetExtensionFunctionAddress
(
  const char *  funcname
) CL_API_SUFFIX__VERSION_1_2
{
  if (strcmp(funcname, "clIcdGetPlatformIDsKHR") == 0)
  {
    return (void*)clIcdGetPlatformIDsKHR;
  }
  else
  {
    return NULL;
  }
}

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformIDs
(
  cl_uint           num_entries,
  cl_platform_id *  platforms,
  cl_uint *         num_platforms
) CL_API_SUFFIX__VERSION_1_0
{
  return clIcdGetPlatformIDsKHR(num_entries, platforms, num_platforms);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformInfo
(
  cl_platform_id    platform,
  cl_platform_info  param_name,
  size_t            param_value_size,
  void *            param_value,
  size_t *          param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
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

CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceIDs
(
  cl_platform_id  platform,
  cl_device_type  device_type,
  cl_uint         num_entries,
  cl_device_id *  devices,
  cl_uint *       num_devices
) CL_API_SUFFIX__VERSION_1_0
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
  else if (devices)
  {
    // Create device if haven't already
    if (!m_device)
    {
      m_device = new _cl_device_id;
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
clGetDeviceInfo
(
  cl_device_id    device,
  cl_device_info  param_name,
  size_t          param_value_size,
  void *          param_value,
  size_t *        param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
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
    result_data = malloc(result_size);
    *(cl_device_type*)result_data = CL_DEVICE_TYPE_CPU;
    break;
  case CL_DEVICE_VENDOR_ID:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = DEVICE_VENDOR_ID;
    break;
  case CL_DEVICE_MAX_COMPUTE_UNITS:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 3;
    break;
  case CL_DEVICE_MAX_WORK_GROUP_SIZE:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = MAX_WI_SIZE;
    break;
  case CL_DEVICE_MAX_WORK_ITEM_SIZES:
    result_size = 3*sizeof(size_t);
    result_data = malloc(result_size);
    ((size_t*)result_data)[0] = MAX_WI_SIZE;
    ((size_t*)result_data)[1] = MAX_WI_SIZE;
    ((size_t*)result_data)[2] = MAX_WI_SIZE;
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_MAX_CLOCK_FREQUENCY:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_ADDRESS_BITS:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = sizeof(size_t)<<3;
    break;
  case CL_DEVICE_MAX_READ_IMAGE_ARGS:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 128;
    break;
  case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 8;
    break;
  case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(cl_ulong*)result_data = MAX_GLOBAL_MEM_SIZE;
    break;
  case CL_DEVICE_IMAGE2D_MAX_WIDTH:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = 8192;
    break;
  case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = 8192;
    break;
  case CL_DEVICE_IMAGE3D_MAX_WIDTH:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = 2048;
    break;
  case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = 2048;
    break;
  case CL_DEVICE_IMAGE3D_MAX_DEPTH:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = 2048;
    break;
  case CL_DEVICE_IMAGE_SUPPORT:
    result_size = sizeof(cl_bool);
    result_data = malloc(result_size);
    *(cl_bool*)result_data = CL_TRUE;
    break;
  case CL_DEVICE_MAX_PARAMETER_SIZE:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = 1024;
    break;
  case CL_DEVICE_MAX_SAMPLERS:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 16;
    break;
  case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = sizeof(cl_long16)<<3;
    break;
  case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_SINGLE_FP_CONFIG:
    result_size = sizeof(cl_device_fp_config);
    result_data = malloc(result_size);
    *(cl_device_fp_config*)result_data =
     CL_FP_ROUND_TO_NEAREST | CL_FP_INF_NAN | CL_FP_DENORM;
    break;
  case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
    result_size = sizeof(cl_device_mem_cache_type);
    result_data = malloc(result_size);
    *(cl_device_mem_cache_type*)result_data = CL_NONE;
    break;
  case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 0;
    break;
  case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
    result_size = sizeof(cl_ulong);
    result_data = malloc(result_size);
    *(cl_ulong*)result_data = 0;
    break;
  case CL_DEVICE_GLOBAL_MEM_SIZE:
    result_size = sizeof(cl_ulong);
    result_data = malloc(result_size);
    *(cl_ulong*)result_data = MAX_GLOBAL_MEM_SIZE;
    break;
  case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
    result_size = sizeof(cl_ulong);
    result_data = malloc(result_size);
    *(cl_ulong*)result_data = MAX_GLOBAL_MEM_SIZE;
    break;
  case CL_DEVICE_MAX_CONSTANT_ARGS:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1024;
    break;
  case CL_DEVICE_LOCAL_MEM_TYPE:
    result_size = sizeof(cl_device_local_mem_type);
    result_data = malloc(result_size);
    *(cl_device_local_mem_type*)result_data = CL_LOCAL;
    break;
  case CL_DEVICE_LOCAL_MEM_SIZE:
    result_size = sizeof(cl_ulong);
    result_data = malloc(result_size);
    *(cl_ulong*)result_data = MAX_LOCAL_MEM_SIZE;
    break;
  case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
    result_size = sizeof(cl_bool);
    result_data = malloc(result_size);
    *(cl_bool*)result_data = CL_FALSE;
    break;
  case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = 1000;
    break;
  case CL_DEVICE_ENDIAN_LITTLE:
    result_size = sizeof(cl_bool);
    result_data = malloc(result_size);
    *(cl_bool*)result_data = CL_TRUE;
    break;
  case CL_DEVICE_AVAILABLE:
    result_size = sizeof(cl_bool);
    result_data = malloc(result_size);
    *(cl_bool*)result_data = CL_TRUE;
    break;
  case CL_DEVICE_COMPILER_AVAILABLE:
    result_size = sizeof(cl_bool);
    result_data = malloc(result_size);
    *(cl_bool*)result_data = CL_TRUE;
    break;
  case CL_DEVICE_EXECUTION_CAPABILITIES:
    result_size = sizeof(cl_device_exec_capabilities);
    result_data = malloc(result_size);
    *(cl_device_exec_capabilities*)result_data =
      CL_EXEC_KERNEL | CL_EXEC_NATIVE_KERNEL;
    break;
  case CL_DEVICE_QUEUE_PROPERTIES:
    result_size = sizeof(cl_command_queue_properties);
    result_data = malloc(result_size);
    *(cl_command_queue_properties*)result_data = CL_QUEUE_PROFILING_ENABLE;
    break;
  case CL_DEVICE_NAME:
    result_size = strlen(DEVICE_NAME) + 1;
    result_data = malloc(result_size);
    strcpy((char*)result_data, DEVICE_NAME);
    break;
  case CL_DEVICE_VENDOR:
    result_size = strlen(DEVICE_VENDOR) + 1;
    result_data = malloc(result_size);
    strcpy((char*)result_data, DEVICE_VENDOR);
    break;
  case CL_DRIVER_VERSION:
    result_size = sizeof(DRIVER_VERSION);
    result_data = malloc(result_size);
    strcpy((char*)result_data, DRIVER_VERSION);
    break;
  case CL_DEVICE_PROFILE:
    result_size = sizeof(DEVICE_PROFILE);
    result_data = malloc(result_size);
    strcpy((char*)result_data, DEVICE_PROFILE);
    break;
  case CL_DEVICE_VERSION:
    result_size = sizeof(DEVICE_VERSION);
    result_data = malloc(result_size);
    strcpy((char*)result_data, DEVICE_VERSION);
    break;
  case CL_DEVICE_EXTENSIONS:
    result_size = strlen(DEVICE_EXTENSIONS) + 1;
    result_data = malloc(result_size);
    strcpy((char*)result_data, DEVICE_EXTENSIONS);
    break;
  case CL_DEVICE_PLATFORM:
    result_size = sizeof(cl_platform_id);
    result_data = malloc(result_size);
    *(cl_platform_id*)result_data = m_platform;
    break;
  case CL_DEVICE_DOUBLE_FP_CONFIG:
    result_size = sizeof(cl_device_fp_config);
    result_data = malloc(result_size);
    *(cl_device_fp_config*)result_data =
      CL_FP_FMA | CL_FP_ROUND_TO_NEAREST |
      CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_INF |
      CL_FP_INF_NAN | CL_FP_DENORM;
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 0;
    break;
  case CL_DEVICE_HOST_UNIFIED_MEMORY:
    result_size = sizeof(cl_bool);
    result_data = malloc(result_size);
    *(cl_bool*)result_data = CL_FALSE;
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 0;
    break;
  case CL_DEVICE_OPENCL_C_VERSION:
    result_size = sizeof(DEVICE_VERSION);
    result_data = malloc(result_size);
    strcpy((char*)result_data, DEVICE_VERSION);
    break;
  case CL_DEVICE_LINKER_AVAILABLE:
    result_size = sizeof(cl_bool);
    result_data = malloc(result_size);
    *(cl_bool*)result_data = CL_TRUE;
    break;
  case CL_DEVICE_BUILT_IN_KERNELS:
    result_size = sizeof("");
    result_data = malloc(result_size);
    strcpy((char*)result_data, "");
    break;
  case CL_DEVICE_IMAGE_MAX_BUFFER_SIZE:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = 65536;
    break;
  case CL_DEVICE_IMAGE_MAX_ARRAY_SIZE:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = 2048;
    break;
  case CL_DEVICE_PARENT_DEVICE:
    result_size = sizeof(cl_device_id);
    result_data = malloc(result_size);
    *(cl_device_id*)result_data = NULL;
    break;
  case CL_DEVICE_PARTITION_MAX_SUB_DEVICES:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 0;
    break;
  case CL_DEVICE_PARTITION_PROPERTIES:
    result_size = sizeof(cl_device_partition_property);
    result_data = malloc(result_size);
    *(cl_device_partition_property*)result_data = 0;
    break;
  case CL_DEVICE_PARTITION_AFFINITY_DOMAIN:
    result_size = sizeof(cl_device_affinity_domain);
    result_data = malloc(result_size);
    *(cl_device_affinity_domain*)result_data = 0;
    break;
  case CL_DEVICE_PARTITION_TYPE:
    result_size = sizeof(cl_device_partition_property);
    result_data = malloc(result_size);
    *(cl_device_partition_property*)result_data = 0;
    break;
  case CL_DEVICE_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_DEVICE_PREFERRED_INTEROP_USER_SYNC:
    result_size = sizeof(cl_bool);
    result_data = malloc(result_size);
    *(cl_bool*)result_data = CL_TRUE;
    break;
  case CL_DEVICE_PRINTF_BUFFER_SIZE:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = 1024;
    break;
  case CL_DEVICE_SPIR_VERSIONS:
    result_size = sizeof(DEVICE_SPIR_VERSIONS);
    result_data = malloc(result_size);
    strcpy((char*)result_data, DEVICE_SPIR_VERSIONS);
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
clCreateSubDevices
(
  cl_device_id                          in_device,
  const cl_device_partition_property *  properties,
  cl_uint                               num_entries,
  cl_device_id *                        out_devices,
  cl_uint *                             num_devices
) CL_API_SUFFIX__VERSION_1_2
{
  return CL_INVALID_VALUE;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainDevice
(
  cl_device_id  device
) CL_API_SUFFIX__VERSION_1_2
{
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseDevice
(
  cl_device_id  device
) CL_API_SUFFIX__VERSION_1_2
{
  return CL_SUCCESS;
}

CL_API_ENTRY cl_context CL_API_CALL
clCreateContext
(
  const cl_context_properties *  properties,
  cl_uint                        num_devices,
  const cl_device_id *           devices,
  void (CL_CALLBACK *            pfn_notify)(const char *,
                                             const void *,
                                             size_t,
                                             void *),
  void *                         user_data,
  cl_int *                       errcode_ret
) CL_API_SUFFIX__VERSION_1_0
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
  cl_context context = new _cl_context;
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
clCreateContextFromType
(
  const cl_context_properties *  properties,
  cl_device_type                 device_type,
  void (CL_CALLBACK *            pfn_notify)(const char *,
                                             const void *,
                                             size_t,
                                             void *),
  void *                         user_data,
  cl_int *                       errcode_ret
) CL_API_SUFFIX__VERSION_1_0
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
  cl_context context = new _cl_context;
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
clRetainContext
(
  cl_context  context
) CL_API_SUFFIX__VERSION_1_0
{
  if (!context)
  {
    return CL_INVALID_CONTEXT;
  }

  context->refCount++;

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseContext
(
  cl_context  context
) CL_API_SUFFIX__VERSION_1_0
{
  if (!context)
  {
    return CL_INVALID_CONTEXT;
  }

  if (--context->refCount == 0)
  {
    delete context->device;
    delete context;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetContextInfo
(
  cl_context       context,
  cl_context_info  param_name,
  size_t           param_value_size,
  void *           param_value,
  size_t *         param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
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
    result_data = malloc(result_size);
    *(cl_uint*)result_data = context->refCount;
    break;
  case CL_CONTEXT_NUM_DEVICES:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_CONTEXT_DEVICES:
    result_size = sizeof(cl_device_id);
    result_data = malloc(result_size);
    *(cl_device_id*)result_data = m_device;
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

CL_API_ENTRY cl_command_queue CL_API_CALL
clCreateCommandQueue
(
  cl_context                   context,
  cl_device_id                 device,
  cl_command_queue_properties  properties,
  cl_int *                     errcode_ret
) CL_API_SUFFIX__VERSION_1_0
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
  queue = new _cl_command_queue;
  queue->queue = new spirsim::Queue(*context->device);
  queue->dispatch = m_dispatchTable;
  queue->properties = properties;
  queue->context = context;
  queue->refCount = 1;

  clRetainContext(context);

  ERRCODE(CL_SUCCESS);
  return queue;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetCommandQueueProperty
(
  cl_command_queue               command_queue,
  cl_command_queue_properties    properties,
  cl_bool                        enable,
  cl_command_queue_properties *  old_properties
)
{
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainCommandQueue
(
  cl_command_queue  command_queue
) CL_API_SUFFIX__VERSION_1_0
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
clReleaseCommandQueue
(
  cl_command_queue  command_queue
) CL_API_SUFFIX__VERSION_1_0
{
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  if (--command_queue->refCount == 0)
  {
    // TODO: Retain/release queue from async thread
    // TODO: Spec states that this function performs an implicit flush,
    // so maybe we are OK to delete queue here?
    clFinish(command_queue);
    delete command_queue->queue;
    clReleaseContext(command_queue->context);
    delete command_queue;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetCommandQueueInfo
(
  cl_command_queue       command_queue,
  cl_command_queue_info  param_name,
  size_t                 param_value_size,
  void *                 param_value,
  size_t *               param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
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
    result_data = malloc(result_size);
    *(cl_context*)result_data = command_queue->context;
    break;
  case CL_QUEUE_DEVICE:
    result_size = sizeof(cl_device_id);
    result_data = malloc(result_size);
    *(cl_device_id*)result_data = m_device;
    break;
  case CL_QUEUE_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = command_queue->refCount;
    break;
  case CL_QUEUE_PROPERTIES:
    result_size = sizeof(cl_command_queue_properties);
    result_data = malloc(result_size);
    *(cl_command_queue_properties*)result_data = command_queue->properties;
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

CL_API_ENTRY cl_mem CL_API_CALL
clCreateBuffer
(
  cl_context    context,
  cl_mem_flags  flags,
  size_t        size,
  void *        host_ptr,
  cl_int *      errcode_ret
) CL_API_SUFFIX__VERSION_1_0
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
  if ((flags & CL_MEM_USE_HOST_PTR) &&
      (flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_ALLOC_HOST_PTR)))
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }

  // Create memory object
  cl_mem mem = new _cl_mem;
  mem->dispatch = m_dispatchTable;
  mem->context = context;
  mem->parent = NULL;
  mem->size = size;
  mem->offset = 0;
  mem->flags = flags;
  mem->isImage = false;
  mem->refCount = 1;
  if (flags & CL_MEM_USE_HOST_PTR)
  {
    mem->address = context->device->getGlobalMemory()->createHostBuffer(
      size, host_ptr);
    mem->hostPtr = host_ptr;
  }
  else
  {
    mem->address = context->device->getGlobalMemory()->allocateBuffer(size);
    mem->hostPtr = NULL;
  }
  if (!mem->address)
  {
    ERRCODE(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    delete mem;
    return NULL;
  }
  clRetainContext(context);

  if (flags & CL_MEM_COPY_HOST_PTR)
  {
    context->device->getGlobalMemory()->store((const unsigned char*)host_ptr,
                                              mem->address, size);
  }

  ERRCODE(CL_SUCCESS);
  return mem;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateSubBuffer
(
  cl_mem                 buffer,
  cl_mem_flags           flags,
  cl_buffer_create_type  buffer_create_type,
  const void *           buffer_create_info,
  cl_int *               errcode_ret
) CL_API_SUFFIX__VERSION_1_1
{
  // Check parameters
  if (!buffer || buffer->parent)
  {
    ERRCODE(CL_INVALID_MEM_OBJECT);
    return NULL;
  }
  if (buffer_create_type != CL_BUFFER_CREATE_TYPE_REGION
     || !buffer_create_info)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }

  _cl_buffer_region region = *(_cl_buffer_region*)buffer_create_info;
  if (region.origin + region.size > buffer->size
      || region.size == 0)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }

  // Inherit flags from parent where appropriate
  cl_mem_flags memFlags = 0;
  cl_mem_flags rwFlags = CL_MEM_READ_ONLY | CL_MEM_READ_WRITE |
                         CL_MEM_WRITE_ONLY;
  cl_mem_flags hostAccess = CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_READ_ONLY |
                            CL_MEM_HOST_WRITE_ONLY;
  cl_mem_flags hostPtr = CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR |
                         CL_MEM_COPY_HOST_PTR;
  if ((flags & rwFlags) == 0)
  {
    memFlags |= buffer->flags & rwFlags;
  }
  else
  {
    memFlags |= flags & rwFlags;
  }
  if ((flags & hostAccess) == 0)
  {
    memFlags |= buffer->flags & hostAccess;
  }
  else
  {
    memFlags |= flags & hostAccess;
  }
  memFlags |= buffer->flags & hostPtr;

  // Create memory object
  cl_mem mem = new _cl_mem;
  mem->dispatch = m_dispatchTable;
  mem->context = buffer->context;
  mem->parent = buffer;
  mem->size = region.size;
  mem->offset = region.origin;
  mem->isImage = false;
  mem->flags = memFlags;
  mem->hostPtr = (unsigned char*)buffer->hostPtr + region.origin;
  mem->refCount = 1;
  mem->address = buffer->address + region.origin;
  clRetainMemObject(buffer);

  ERRCODE(CL_SUCCESS);
  return mem;
}

// Utility function for getting number of dimensions in image
size_t getNumDimensions(cl_mem_object_type type)
{
  switch (type)
  {
  case CL_MEM_OBJECT_IMAGE1D:
  case CL_MEM_OBJECT_IMAGE1D_ARRAY:
  case CL_MEM_OBJECT_IMAGE1D_BUFFER:
    return 1;
  case CL_MEM_OBJECT_IMAGE2D:
  case CL_MEM_OBJECT_IMAGE2D_ARRAY:
    return 2;
  case CL_MEM_OBJECT_IMAGE3D:
    return 3;
  default:
    return 0;
  }
}

// Utility function for getting number of channels in an image
size_t getNumChannels(const cl_image_format *format)
{
  switch (format->image_channel_order)
  {
  case CL_R:
  case CL_Rx:
  case CL_A:
  case CL_INTENSITY:
  case CL_LUMINANCE:
    return 1;
  case CL_RG:
  case CL_RGx:
  case CL_RA:
    return 2;
  case CL_RGB:
  case CL_RGBx:
    return 3;
  case CL_RGBA:
  case CL_ARGB:
  case CL_BGRA:
    return 4;
  default:
    return 0;
  }
}

// Utility function for computing an image format's pixel size (in bytes)
size_t getPixelSize(const cl_image_format *format)
{
  // Get number of channels
  size_t numChannels = getNumChannels(format);

  // Get size of each pixel (in bytes)
  switch (format->image_channel_data_type)
  {
  case CL_SNORM_INT8:
  case CL_UNORM_INT8:
  case CL_SIGNED_INT8:
  case CL_UNSIGNED_INT8:
    return numChannels;
  case CL_SNORM_INT16:
  case CL_UNORM_INT16:
  case CL_SIGNED_INT16:
  case CL_UNSIGNED_INT16:
  case CL_HALF_FLOAT:
    return 2*numChannels;
  case CL_SIGNED_INT32:
  case CL_UNSIGNED_INT32:
  case CL_FLOAT:
    return 4*numChannels;
  case CL_UNORM_SHORT_565:
  case CL_UNORM_SHORT_555:
    return 2;
  case CL_UNORM_INT_101010:
    return 4;
  default:
    return 0;
  }
}

bool isImageArray(cl_mem_object_type type)
{
  if (type == CL_MEM_OBJECT_IMAGE1D_ARRAY ||
      type == CL_MEM_OBJECT_IMAGE2D_ARRAY)
  {
    return true;
  }
  return false;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage
(
  cl_context               context,
  cl_mem_flags             flags,
  const cl_image_format *  image_format,
  const cl_image_desc *    image_desc,
  void *                   host_ptr,
  cl_int *                 errcode_ret
) CL_API_SUFFIX__VERSION_1_2
{
  // Check parameters
  if (!context)
  {
    ERRCODE(CL_INVALID_CONTEXT);
    return NULL;
  }
  if (!image_format)
  {
    ERRCODE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
    return NULL;
  }
  if (!image_desc)
  {
    ERRCODE(CL_INVALID_IMAGE_DESCRIPTOR);
    return NULL;
  }

  // Get size of each pixel (in bytes)
  size_t pixelSize = getPixelSize(image_format);
  if (!pixelSize)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }

  // Get image dimensions
  size_t dims = getNumDimensions(image_desc->image_type);
  size_t width = image_desc->image_width;
  size_t height = 1, depth = 1;
  size_t arraySize = 1;
  if (dims > 1)
  {
    height = image_desc->image_height;
  }
  if (dims > 2)
  {
    depth = image_desc->image_depth;
  }
  if (isImageArray(image_desc->image_type))
  {
    arraySize = image_desc->image_array_size;
  }

  // Calculate total size of iamge
  size_t size = width * height * depth * arraySize * pixelSize;

  cl_mem mem;

  if (image_desc->image_type == CL_MEM_OBJECT_IMAGE1D_BUFFER)
  {
    // Use existing buffer
    if (!image_desc->buffer)
    {
      ERRCODE(CL_INVALID_VALUE);
      return NULL;
    }
    mem = image_desc->buffer;
    clRetainMemObject(image_desc->buffer);
  }
  else
  {
    // Create buffer
    // TODO: Use pitches
    mem = clCreateBuffer(context, flags, size, host_ptr, errcode_ret);
    if (!mem)
    {
      return NULL;
    }
  }

  // Create image object wrapper
  cl_image *image = new cl_image;
  *(cl_mem)image = *mem;
  image->isImage = true;
  image->format = *image_format;
  image->desc = *image_desc;
  image->desc.image_width = width;
  image->desc.image_height = height;
  image->desc.image_depth = depth;
  image->desc.image_array_size = arraySize;
  image->refCount = 1;
  if (image_desc->image_type != CL_MEM_OBJECT_IMAGE1D_BUFFER)
  {
    delete mem;
  }

  ERRCODE(CL_SUCCESS);
  return image;
}


CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage2D
(
  cl_context               context,
  cl_mem_flags             flags,
  const cl_image_format *  image_format,
  size_t                   image_width,
  size_t                   image_height,
  size_t                   image_row_pitch,
  void *                   host_ptr,
  cl_int *                 errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  cl_image_desc desc =
  {
    CL_MEM_OBJECT_IMAGE2D,
    image_width,
    image_height,
    1,
    1,
    image_row_pitch,
    0,
    0,
    0,
    NULL
  };
  return clCreateImage(context, flags,
                       image_format, &desc,
                       host_ptr, errcode_ret);
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage3D
(
  cl_context               context,
  cl_mem_flags             flags,
  const cl_image_format *  image_format,
  size_t                   image_width,
  size_t                   image_height,
  size_t                   image_depth,
  size_t                   image_row_pitch,
  size_t                   image_slice_pitch,
  void *                   host_ptr,
  cl_int *                 errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  cl_image_desc desc =
  {
    CL_MEM_OBJECT_IMAGE3D,
    image_width,
    image_height,
    image_depth,
    1,
    image_row_pitch,
    image_slice_pitch,
    0,
    0,
    NULL
  };
  return clCreateImage(context, flags,
                       image_format, &desc,
                       host_ptr, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainMemObject
(
  cl_mem  memobj
) CL_API_SUFFIX__VERSION_1_0
{
  if (!memobj)
  {
    return CL_INVALID_MEM_OBJECT;
  }

  memobj->refCount++;
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseMemObject
(
  cl_mem  memobj
) CL_API_SUFFIX__VERSION_1_0
{
  if (!memobj)
  {
    return CL_INVALID_MEM_OBJECT;
  }

  if (--memobj->refCount == 0)
  {
    if (memobj->isImage &&
        ((cl_image*)memobj)->desc.image_type == CL_MEM_OBJECT_IMAGE1D_BUFFER)
    {
      clReleaseMemObject(((cl_image*)memobj)->desc.buffer);
    }
    else
    {
      if (memobj->parent)
      {
        clReleaseMemObject(memobj->parent);
      }
      else
      {
        memobj->context->device->getGlobalMemory()->deallocateBuffer(
          memobj->address);
        clReleaseContext(memobj->context);
      }

      while (!memobj->callbacks.empty())
      {
        pair<void (CL_CALLBACK *)(cl_mem, void *), void*> callback =
          memobj->callbacks.top();
        callback.first(memobj, callback.second);
        memobj->callbacks.pop();
      }
    }

    delete memobj;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetSupportedImageFormats
(
  cl_context          context,
  cl_mem_flags        flags,
  cl_mem_object_type  image_type,
  cl_uint             num_entries,
  cl_image_format *   image_formats,
  cl_uint *           num_image_formats
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!context)
  {
    return CL_INVALID_CONTEXT;
  }
  if (num_entries == 0 && image_formats)
  {
    return CL_INVALID_VALUE;
  }

  const cl_channel_order orders[] =
  {
    CL_R, CL_Rx, CL_A,
    CL_INTENSITY,
    CL_LUMINANCE,
    CL_RG, CL_RGx, CL_RA,
    CL_RGB, CL_RGBx,
    CL_RGBA,
    CL_ARGB, CL_BGRA,
  };
  const cl_channel_type types[] =
  {
    CL_SNORM_INT8,
    CL_SNORM_INT16,
    CL_UNORM_INT8,
    CL_UNORM_INT16,
    CL_SIGNED_INT8,
    CL_SIGNED_INT16,
    CL_SIGNED_INT32,
    CL_UNSIGNED_INT8,
    CL_UNSIGNED_INT16,
    CL_UNSIGNED_INT32,
//    CL_HALF_FLOAT, // TODO: Add support for this
    CL_FLOAT,
  };

  // Calculate number of formats
  size_t numOrders = sizeof(orders)/sizeof(cl_channel_order);
  size_t numTypes = sizeof(types)/sizeof(cl_channel_type);
  size_t numFormats = numOrders * numTypes;
  if (num_image_formats)
  {
    *num_image_formats = numFormats;
  }

  // Generate list of all order/type combinations
  // TODO: Add support for packed image types
  if (image_formats)
  {
    for (int o = 0; o < numOrders; o++)
    {
      for (int t = 0; t < sizeof(types)/sizeof(cl_channel_type); t++)
      {
        int i = t + o*numTypes;
        if (i >= num_entries)
        {
          return CL_SUCCESS;
        }

        cl_image_format format = {orders[o], types[t]};
        image_formats[i] = format;
      }
    }
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetMemObjectInfo
(
  cl_mem       memobj,
  cl_mem_info  param_name,
  size_t       param_value_size,
  void *       param_value,
  size_t *     param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
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
    result_data = malloc(result_size);
    *(cl_mem_object_type*)result_data = CL_MEM_OBJECT_BUFFER;
    break;
  case CL_MEM_FLAGS:
    result_size = sizeof(cl_mem_flags);
    result_data = malloc(result_size);
    *(cl_mem_flags*)result_data = memobj->flags;
    break;
  case CL_MEM_SIZE:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = memobj->size;
    break;
  case CL_MEM_HOST_PTR:
    result_size = sizeof(void*);
    result_data = malloc(result_size);
    memcpy(result_data, &(memobj->hostPtr), result_size);
    break;
  case CL_MEM_MAP_COUNT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 0;
    break;
  case CL_MEM_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = memobj->refCount;
    break;
  case CL_MEM_CONTEXT:
    result_size = sizeof(cl_context);
    result_data = malloc(result_size);
    *(cl_context*)result_data = memobj->context;
    break;
  case CL_MEM_ASSOCIATED_MEMOBJECT:
    result_size = sizeof(cl_mem);
    result_data = malloc(result_size);
    *(cl_mem*)result_data = memobj->parent;
    break;
  case CL_MEM_OFFSET:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = memobj->offset;
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
clGetImageInfo
(
  cl_mem         image,
  cl_image_info  param_name,
  size_t         param_value_size,
  void *         param_value,
  size_t *       param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check mem object is valid
  if (!image)
  {
    return CL_INVALID_MEM_OBJECT;
  }
  cl_image *img = (cl_image*)image;

  switch (param_name)
  {
  case CL_IMAGE_FORMAT:
    result_size = sizeof(cl_image_format);
    result_data = malloc(result_size);
    *(cl_image_format*)result_data = img->format;
    break;
  case CL_IMAGE_ELEMENT_SIZE:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = getPixelSize(&img->format);
    break;
  case CL_IMAGE_ROW_PITCH:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = img->desc.image_row_pitch;
    break;
  case CL_IMAGE_SLICE_PITCH:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = img->desc.image_slice_pitch;
    break;
  case CL_IMAGE_WIDTH:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = img->desc.image_width;
    break;
  case CL_IMAGE_HEIGHT:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data =
      getNumDimensions(img->desc.image_type) > 1 ? img->desc.image_height : 0;
    break;
  case CL_IMAGE_DEPTH:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data =
      getNumDimensions(img->desc.image_type) > 2 ? img->desc.image_depth : 0;
    break;
  case CL_IMAGE_ARRAY_SIZE:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data =
      isImageArray(img->desc.image_type) ? img->desc.image_array_size : 0;
    break;
  case CL_IMAGE_BUFFER:
    result_size = sizeof(cl_mem);
    result_data = malloc(result_size);
    *(cl_mem*)result_data = img->desc.buffer;
    break;
  case CL_IMAGE_NUM_MIP_LEVELS:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 0;
    break;
  case CL_IMAGE_NUM_SAMPLES:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 0;
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
clSetMemObjectDestructorCallback
(
  cl_mem               memobj,
  void (CL_CALLBACK *  pfn_notify)(cl_mem, void*),
  void *               user_data
) CL_API_SUFFIX__VERSION_1_1
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

  memobj->callbacks.push(make_pair(pfn_notify, user_data));

  return CL_SUCCESS;
}

CL_API_ENTRY cl_sampler CL_API_CALL
clCreateSampler
(
  cl_context          context,
  cl_bool             normalized_coords,
  cl_addressing_mode  addressing_mode,
  cl_filter_mode      filter_mode,
  cl_int *            errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!context)
  {
    ERRCODE(CL_INVALID_CONTEXT);
    return NULL;
  }

  // Create sampler bitfield
  uint32_t bitfield = 0;

  if (normalized_coords)
  {
    bitfield |= 0x0001;
  }

  switch (addressing_mode)
  {
    case CL_ADDRESS_NONE:
      break;
    case CL_ADDRESS_CLAMP_TO_EDGE:
      bitfield |= 0x0002;
      break;
    case CL_ADDRESS_CLAMP:
      bitfield |= 0x0004;
      break;
    case CL_ADDRESS_REPEAT:
      bitfield |= 0x0006;
      break;
    case CL_ADDRESS_MIRRORED_REPEAT:
      bitfield |= 0x0008;
      break;
    default:
      ERRCODE(CL_INVALID_VALUE);
      return NULL;
  }

  switch (filter_mode)
  {
    case CL_FILTER_NEAREST:
      bitfield |= 0x0010;
      break;
    case CL_FILTER_LINEAR:
      bitfield |= 0x0020;
      break;
    default:
      ERRCODE(CL_INVALID_VALUE);
      return NULL;
  }

  // Create sampler
  cl_sampler sampler = new _cl_sampler;
  sampler->dispatch = m_dispatchTable;
  sampler->context = context;
  sampler->normCoords = normalized_coords;
  sampler->addressMode = addressing_mode;
  sampler->filterMode = filter_mode;
  sampler->sampler = bitfield;

  ERRCODE(CL_SUCCESS);
  return sampler;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainSampler
(
  cl_sampler  sampler
) CL_API_SUFFIX__VERSION_1_0
{
  if (!sampler)
  {
    return CL_INVALID_SAMPLER;
  }

  sampler->refCount++;

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseSampler
(
  cl_sampler  sampler
) CL_API_SUFFIX__VERSION_1_0
{
  if (!sampler)
  {
    return CL_INVALID_SAMPLER;
  }

  if (--sampler->refCount == 0)
  {
    delete sampler;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetSamplerInfo
(
  cl_sampler       sampler,
  cl_sampler_info  param_name,
  size_t           param_value_size,
  void *           param_value,
  size_t *         param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check sampler is valid
  if (!sampler)
  {
    return CL_INVALID_SAMPLER;
  }

  switch (param_name)
  {
  case CL_SAMPLER_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = sampler->refCount;
    break;
  case CL_SAMPLER_CONTEXT:
    result_size = sizeof(cl_context);
    result_data = malloc(result_size);
    *(cl_context*)result_data = sampler->context;
    break;
  case CL_SAMPLER_NORMALIZED_COORDS:
    result_size = sizeof(cl_bool);
    result_data = malloc(result_size);
    *(cl_bool*)result_data = sampler->normCoords;
    break;
  case CL_SAMPLER_ADDRESSING_MODE:
    result_size = sizeof(cl_addressing_mode);
    result_data = malloc(result_size);
    *(cl_addressing_mode*)result_data = sampler->addressMode;
    break;
  case CL_SAMPLER_FILTER_MODE:
    result_size = sizeof(cl_filter_mode);
    result_data = malloc(result_size);
    *(cl_filter_mode*)result_data = sampler->filterMode;
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

CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithSource
(
  cl_context      context,
  cl_uint         count,
  const char **   strings,
  const size_t *  lengths,
  cl_int *        errcode_ret
) CL_API_SUFFIX__VERSION_1_0
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
  cl_program prog = new _cl_program;
  prog->dispatch = m_dispatchTable;
  prog->program = new spirsim::Program(source);
  prog->context = context;
  prog->refCount = 1;
  if (!prog->program)
  {
    ERRCODE(CL_OUT_OF_HOST_MEMORY);
    delete prog;
    return NULL;
  }

  ERRCODE(CL_SUCCESS);
  return prog;
}

CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithBinary
(
  cl_context              context,
  cl_uint                 num_devices,
  const cl_device_id *    device_list,
  const size_t *          lengths,
  const unsigned char **  binaries,
  cl_int *                binary_status,
  cl_int *                errcode_ret
) CL_API_SUFFIX__VERSION_1_0
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
  cl_program prog = new _cl_program;
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
    delete prog;
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
clCreateProgramWithBuiltInKernels
(
  cl_context            context,
  cl_uint               num_devices,
  const cl_device_id *  device_list,
  const char *          kernel_names,
  cl_int *              errcode_ret
) CL_API_SUFFIX__VERSION_1_2
{
  ERRCODE(CL_INVALID_VALUE);
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainProgram
(
  cl_program  program
) CL_API_SUFFIX__VERSION_1_0
{
  if (!program)
  {
    return CL_INVALID_PROGRAM;
  }

  program->refCount++;
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseProgram
(
  cl_program  program
) CL_API_SUFFIX__VERSION_1_0
{
  if (!program)
  {
    return CL_INVALID_PROGRAM;
  }

  if (--program->refCount == 0)
  {
    delete program->program;
    delete program;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clBuildProgram
(
  cl_program            program,
  cl_uint               num_devices,
  const cl_device_id *  device_list,
  const char *          options,
  void (CL_CALLBACK *   pfn_notify)(cl_program, void*),
  void *                user_data
) CL_API_SUFFIX__VERSION_1_0
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

  // Fire callback
  if (pfn_notify)
  {
    pfn_notify(program, user_data);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clUnloadCompiler
(
  void
) CL_API_SUFFIX__VERSION_1_0
{
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clCompileProgram
(
  cl_program            program,
  cl_uint               num_devices,
  const cl_device_id *  device_list,
  const char *          options,
  cl_uint               num_input_headers,
  const cl_program *    input_headers,
  const char **         header_include_names,
  void (CL_CALLBACK *   pfn_notify)(cl_program, void*),
  void *                user_data
) CL_API_SUFFIX__VERSION_1_2
{
  // Check parameters
  if (!program)
  {
    return CL_INVALID_PROGRAM;
  }
  if ((num_devices > 0 && !device_list) ||
      (num_devices == 0 && device_list))
  {
    return CL_INVALID_VALUE;
  }
  if ((num_input_headers && (!input_headers || !header_include_names)) ||
      (!num_input_headers && (input_headers || header_include_names)))
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

  // Prepare headers
  list<spirsim::Program::Header> headers;
  for (int i = 0; i < num_input_headers; i++)
  {
    headers.push_back(make_pair(header_include_names[i],
                                input_headers[i]->program));
  }

  // Build program
  if (!program->program->build(options, headers))
  {
    return CL_BUILD_PROGRAM_FAILURE;
  }

  // Fire callback
  if (pfn_notify)
  {
    pfn_notify(program, user_data);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_program CL_API_CALL
clLinkProgram
(
  cl_context            context,
  cl_uint               num_devices,
  const cl_device_id *  device_list,
  const char *          options,
  cl_uint               num_input_programs,
  const cl_program *    input_programs,
  void (CL_CALLBACK *   pfn_notify)(cl_program, void*),
  void *                user_data,
  cl_int *              errcode_ret
) CL_API_SUFFIX__VERSION_1_2
{
  // Check parameters
  if (!context)
  {
    ERRCODE(CL_INVALID_CONTEXT);
    return NULL;
  }
  if ((num_devices > 0 && !device_list) ||
      (num_devices == 0 && device_list))
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }
  if (!num_input_programs || !input_programs)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }
  if (!pfn_notify && user_data)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }
  if (device_list && !device_list[0])
  {
    ERRCODE(CL_INVALID_DEVICE);
    return NULL;
  }

  // Prepare programs
  list<const spirsim::Program*> programs;
  for (int i = 0; i < num_input_programs; i++)
  {
    programs.push_back(input_programs[i]->program);
  }

  // Create program object
  cl_program prog = new _cl_program;
  prog->dispatch = m_dispatchTable;
  prog->program = spirsim::Program::createFromPrograms(programs);
  prog->context = context;
  prog->refCount = 1;
  if (!prog->program)
  {
    ERRCODE(CL_INVALID_BINARY);
    delete prog;
    return NULL;
  }

  // Fire callback
  if (pfn_notify)
  {
    pfn_notify(prog, user_data);
  }

  ERRCODE(CL_SUCCESS);
  return prog;
}

CL_API_ENTRY cl_int CL_API_CALL
clUnloadPlatformCompiler
(
  cl_platform_id  platform
) CL_API_SUFFIX__VERSION_1_2
{
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetProgramInfo
(
  cl_program       program,
  cl_program_info  param_name,
  size_t           param_value_size,
  void *           param_value,
  size_t *         param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
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
    result_data = malloc(result_size);
    *(cl_uint*)result_data = program->refCount;
    break;
  case CL_PROGRAM_CONTEXT:
    result_size = sizeof(cl_context);
    result_data = malloc(result_size);
    *(cl_context*)result_data = program->context;
    break;
  case CL_PROGRAM_NUM_DEVICES:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = 1;
    break;
  case CL_PROGRAM_DEVICES:
    result_size = sizeof(cl_device_id);
    result_data = malloc(result_size);
    *(cl_device_id*)result_data = m_device;
    break;
  case CL_PROGRAM_SOURCE:
    result_size = strlen(program->program->getSource().c_str()) + 1;
    result_data = malloc(result_size);
    strcpy((char*)result_data, program->program->getSource().c_str());
    break;
  case CL_PROGRAM_BINARY_SIZES:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = program->program->getBinarySize();
    break;
  case CL_PROGRAM_BINARIES:
    result_size = sizeof(unsigned char*);
    result_data = program->program->getBinary();
    break;
  case CL_PROGRAM_NUM_KERNELS:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = program->program->getNumKernels();
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
    result_size = strlen(ret.c_str()) + 1;
    result_data = malloc(result_size);
    strcpy((char*)result_data, ret.c_str());
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
clGetProgramBuildInfo
(
  cl_program             program,
  cl_device_id           device,
  cl_program_build_info  param_name,
  size_t                 param_value_size,
  void *                 param_value,
  size_t *               param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
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
    result_data = malloc(result_size);
    *(cl_build_status*)result_data = program->program->getBuildStatus();
    break;
  case CL_PROGRAM_BUILD_OPTIONS:
    result_size = strlen(program->program->getBuildOptions().c_str()) + 1;
    result_data = malloc(result_size);
    strcpy((char*)result_data, program->program->getBuildOptions().c_str());
    break;
  case CL_PROGRAM_BUILD_LOG:
    result_size = strlen(program->program->getBuildLog().c_str()) + 1;
    result_data = malloc(result_size);
    strcpy((char*)result_data, program->program->getBuildLog().c_str());
    break;
  case CL_PROGRAM_BINARY_TYPE:
    result_size = sizeof(cl_program_binary_type);
    result_data = malloc(result_size);
    *(cl_program_binary_type*)result_data =
      CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT;
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

CL_API_ENTRY cl_kernel CL_API_CALL
clCreateKernel
(
  cl_program    program,
  const char *  kernel_name,
  cl_int *      errcode_ret
) CL_API_SUFFIX__VERSION_1_0
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
  cl_kernel kernel = new _cl_kernel;
  kernel->dispatch = m_dispatchTable;
  kernel->kernel = program->program->createKernel(kernel_name);
  kernel->program = program;
  kernel->refCount = 1;
  if (!kernel->kernel)
  {
    ERRCODE(CL_INVALID_KERNEL_NAME);
    delete kernel;
    return NULL;
  }

  clRetainProgram(program);

  ERRCODE(CL_SUCCESS);
  return kernel;
}

CL_API_ENTRY cl_int CL_API_CALL
clCreateKernelsInProgram
(
  cl_program   program,
  cl_uint      num_kernels,
  cl_kernel *  kernels,
  cl_uint *    num_kernels_ret
) CL_API_SUFFIX__VERSION_1_0
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
      cl_kernel kernel = new _cl_kernel;
      kernel->dispatch = m_dispatchTable;
      kernel->kernel = program->program->createKernel(*itr);
      kernel->program = program;
      kernel->refCount = 1;
      kernels[i++] = kernel;

      clRetainProgram(program);
    }
  }

  if (num_kernels_ret)
  {
    *num_kernels_ret = num;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainKernel
(
  cl_kernel  kernel
) CL_API_SUFFIX__VERSION_1_0
{
  if (!kernel)
  {
    return CL_INVALID_KERNEL;
  }

  kernel->refCount++;
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseKernel
(
  cl_kernel  kernel
) CL_API_SUFFIX__VERSION_1_0
{
  if (!kernel)
  {
    return CL_INVALID_KERNEL;
  }

  if (--kernel->refCount == 0)
  {
    clReleaseProgram(kernel->program);
    delete kernel->kernel;
    delete kernel;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetKernelArg
(
  cl_kernel     kernel,
  cl_uint       arg_index,
  size_t        arg_size,
  const void *  arg_value
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (arg_index >= kernel->kernel->getNumArguments())
  {
    return CL_INVALID_ARG_INDEX;
  }

  unsigned int addr = kernel->kernel->getArgumentAddressQualifier(arg_index);
  bool isSampler = strcmp(kernel->kernel->getArgumentTypeName(arg_index),
                          "sampler_t") == 0;

  if (kernel->kernel->getArgumentSize(arg_index) != arg_size
      && !isSampler
      && addr != CL_KERNEL_ARG_ADDRESS_LOCAL)
  {
    return CL_INVALID_ARG_SIZE;
  }

  // Prepare argument value
  spirsim::TypedValue value;
  value.data = new unsigned char[arg_size];
  value.size = arg_size;
  value.num = 1;
  switch (addr)
  {
  case CL_KERNEL_ARG_ADDRESS_PRIVATE:
    if (isSampler)
    {
      memcpy(value.data, &(*(cl_sampler*)arg_value)->sampler, 4);
    }
    else
    {
      memcpy(value.data, arg_value, arg_size);
    }
    break;
  case CL_KERNEL_ARG_ADDRESS_LOCAL:
    *(size_t*)value.data = 0;
    break;
  case CL_KERNEL_ARG_ADDRESS_GLOBAL:
  case CL_KERNEL_ARG_ADDRESS_CONSTANT:
    if (arg_value && *(cl_mem*)arg_value)
    {
      cl_mem mem = *(cl_mem*)arg_value;

      if (mem->isImage)
      {
        // Create Image struct
        spirsim::Image *image = new spirsim::Image;
        image->address = mem->address;
        image->format = ((cl_image*)mem)->format;
        image->desc = ((cl_image*)mem)->desc;
        *(spirsim::Image**)value.data = image;
      }
      else
      {
        memcpy(value.data, &mem->address, arg_size);
      }

      kernel->memArgs[arg_index] = mem;
    }
    else
    {
      *(size_t*)value.data = 0;
      kernel->memArgs.erase(arg_index);
    }
    break;
  default:
    return CL_INVALID_ARG_VALUE;
  }

  // Set argument
  kernel->kernel->setArgument(arg_index, value);
  delete[] value.data;

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelInfo
(
  cl_kernel       kernel,
  cl_kernel_info  param_name,
  size_t          param_value_size,
  void *          param_value,
  size_t *        param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
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
    result_size = strlen(kernel->kernel->getName().c_str()) + 1;
    result_data = malloc(result_size);
    strcpy((char*)result_data, kernel->kernel->getName().c_str());
    break;
  case CL_KERNEL_NUM_ARGS:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = kernel->kernel->getNumArguments();
    break;
  case CL_KERNEL_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = kernel->refCount;
    break;
  case CL_KERNEL_CONTEXT:
    result_size = sizeof(cl_context);
    result_data = malloc(result_size);
    *(cl_context*)result_data = kernel->program->context;
    break;
  case CL_KERNEL_PROGRAM:
    result_size = sizeof(cl_program);
    result_data = malloc(result_size);
    *(cl_program*)result_data = kernel->program;
    break;
  case CL_KERNEL_ATTRIBUTES:
    result_size = strlen(kernel->kernel->getAttributes().c_str()) + 1;
    result_data = malloc(result_size);
    strcpy((char*)result_data, kernel->kernel->getAttributes().c_str());
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
clGetKernelArgInfo
(
  cl_kernel           kernel,
  cl_uint             arg_indx,
  cl_kernel_arg_info  param_name,
  size_t              param_value_size,
  void *              param_value,
  size_t *            param_value_size_ret
) CL_API_SUFFIX__VERSION_1_2
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check parameters are valid
  if (!kernel)
  {
    return CL_INVALID_KERNEL;
  }
  if (arg_indx >= kernel->kernel->getNumArguments())
  {
    return CL_INVALID_ARG_INDEX;
  }

  switch (param_name)
  {
  case CL_KERNEL_ARG_ADDRESS_QUALIFIER:
    result_size = sizeof(cl_kernel_arg_address_qualifier);
    result_data = malloc(result_size);
    *(cl_kernel_arg_address_qualifier*)result_data =
      kernel->kernel->getArgumentAddressQualifier(arg_indx);
    break;
  case CL_KERNEL_ARG_ACCESS_QUALIFIER:
    result_size = sizeof(cl_kernel_arg_access_qualifier);
    result_data = malloc(result_size);
    *(cl_kernel_arg_access_qualifier*)result_data =
      kernel->kernel->getArgumentAccessQualifier(arg_indx);
    break;
  case CL_KERNEL_ARG_TYPE_NAME:
    result_data = kernel->kernel->getArgumentTypeName(arg_indx);
    if (!result_data)
    {
      return CL_INVALID_VALUE;
    }
    result_size = strlen((char*)result_data) + 1;
    break;
  case CL_KERNEL_ARG_TYPE_QUALIFIER:
    result_size = sizeof(cl_kernel_arg_type_qualifier);
    result_data = malloc(result_size);
    *(cl_kernel_arg_type_qualifier*)result_data =
      kernel->kernel->getArgumentTypeQualifier(arg_indx);
    break;
  case CL_KERNEL_ARG_NAME:
    result_data = kernel->kernel->getArgumentName(arg_indx);
    if (!result_data)
    {
      return CL_INVALID_VALUE;
    }
    result_size = strlen((char*)result_data) + 1;
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
clGetKernelWorkGroupInfo
(
  cl_kernel                  kernel,
  cl_device_id               device,
  cl_kernel_work_group_info  param_name,
  size_t                     param_value_size,
  void *                     param_value,
  size_t *                   param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check parameters are valid
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
    result_data = malloc(result_size);
    *(size_t*)result_data = MAX_WI_SIZE;
    break;
  case CL_KERNEL_COMPILE_WORK_GROUP_SIZE:
    result_size = sizeof(size_t[3]);
    result_data = malloc(result_size);
    kernel->kernel->getRequiredWorkGroupSize((size_t*)result_data);
    break;
  case CL_KERNEL_LOCAL_MEM_SIZE:
    result_size = sizeof(cl_ulong);
    result_data = malloc(result_size);
    *(cl_ulong*)result_data = kernel->kernel->getLocalMemorySize();
    break;
  case CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE:
    result_size = sizeof(size_t);
    result_data = malloc(result_size);
    *(size_t*)result_data = 1;
    break;
  case CL_KERNEL_PRIVATE_MEM_SIZE:
    result_size = sizeof(cl_ulong);
    result_data = malloc(result_size);
    *(cl_ulong*)result_data = 0; // TODO: Real value
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

// Utility to check if an event has completed (or terminated)
inline bool isComplete(cl_event event)
{
  return (event->event->state == CL_COMPLETE || event->event->state < 0);
}

CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents
(
  cl_uint           num_events,
  const cl_event *  event_list
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!num_events || !event_list)
  {
    return CL_INVALID_VALUE;
  }

  cl_int ret = CL_SUCCESS;

  // Loop until all events complete
  bool complete = false;
  while (!complete)
  {
    complete = true;
    for (int i = 0; i < num_events; i++)
    {
      // Skip event if already complete
      if (isComplete(event_list[i]))
      {
        continue;
      }

      // If it's not a user event, update the queue
      if (event_list[i]->queue)
      {
        spirsim::Queue::Command *cmd = event_list[i]->queue->queue->update();
        if (cmd)
        {
          asyncQueueRelease(cmd);
          delete cmd;
        }

        // If it's still not complete, update flag
        if (!isComplete(event_list[i]))
        {
          complete = false;
        }
      }
      else
      {
        complete = false;
      }
    }
  }

  // Check if any command terminated unsuccessfully
  for (int i = 0; i < num_events; i++)
  {
    if (event_list[i]->event->state < 0)
    {
      ret = CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
    }
  }

  return ret;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetEventInfo
(
  cl_event       event,
  cl_event_info  param_name,
  size_t         param_value_size,
  void *         param_value,
  size_t *       param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
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
    result_data = malloc(result_size);
    *(cl_command_queue*)result_data = event->queue;
    break;
  case CL_EVENT_CONTEXT:
    result_size = sizeof(cl_context);
    result_data = malloc(result_size);
    *(cl_context*)result_data = event->context;
    break;
  case CL_EVENT_COMMAND_TYPE:
    result_size = sizeof(cl_command_type);
    result_data = malloc(result_size);
    *(cl_command_type*)result_data = event->type;
    break;
  case CL_EVENT_COMMAND_EXECUTION_STATUS:
    result_size = sizeof(cl_int);
    result_data = malloc(result_size);
    *(cl_int*)result_data = event->event->state;
    break;
  case CL_EVENT_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data = malloc(result_size);
    *(cl_uint*)result_data = event->refCount;
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
clCreateUserEvent
(
  cl_context  context,
  cl_int *    errcode_ret
) CL_API_SUFFIX__VERSION_1_1
{
  // Check parameters
  if (!context)
  {
    ERRCODE(CL_INVALID_CONTEXT);
    return NULL;
  }

  /// Create event object
  cl_event event = new _cl_event;
  event->dispatch = m_dispatchTable;
  event->context = context;
  event->queue = 0;
  event->type = CL_COMMAND_USER;
  event->event = new spirsim::Event();
  event->event->state = CL_SUBMITTED;
  event->refCount = 1;

  ERRCODE(CL_SUCCESS);
  return event;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainEvent
(
  cl_event  event
) CL_API_SUFFIX__VERSION_1_0
{
  if (!event)
  {
    return CL_INVALID_EVENT;
  }

  event->refCount++;

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseEvent
(
  cl_event  event
) CL_API_SUFFIX__VERSION_1_0
{
  if (!event)
  {
    return CL_INVALID_EVENT;
  }

  if (--event->refCount == 0)
  {
    if (event->event)
    {
      delete event->event;
    }
    delete event;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetUserEventStatus
(
  cl_event  event,
  cl_int    execution_status
) CL_API_SUFFIX__VERSION_1_1
{
  // Check parameters
  if (!event || event->queue)
  {
    return CL_INVALID_EVENT;
  }
  if (execution_status != CL_COMPLETE && execution_status >= 0)
  {
    return CL_INVALID_VALUE;
  }
  if (event->event->state == CL_COMPLETE || event->event->state < 0)
  {
    return CL_INVALID_OPERATION;
  }

  event->event->state = execution_status;

  // Perform callbacks
  list< pair<void (CL_CALLBACK *)(cl_event, cl_int, void *), void*> >::iterator itr;
  for (itr = event->callbacks.begin(); itr != event->callbacks.end(); itr++)
  {
    itr->first(event, execution_status, itr->second);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetEventCallback
(
  cl_event             event,
  cl_int               command_exec_callback_type,
  void (CL_CALLBACK *  pfn_notify)(cl_event, cl_int, void*),
  void *               user_data
) CL_API_SUFFIX__VERSION_1_1
{
  // Check parameters
  if (!event)
  {
    return CL_INVALID_EVENT;
  }
  if (!pfn_notify ||
      (command_exec_callback_type != CL_COMPLETE &&
       command_exec_callback_type != CL_SUBMITTED &&
       command_exec_callback_type != CL_RUNNING))
  {
    return CL_INVALID_VALUE;
  }

  event->callbacks.push_back(make_pair(pfn_notify, user_data));

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetEventProfilingInfo
(
  cl_event           event,
  cl_profiling_info  param_name,
  size_t             param_value_size,
  void *             param_value,
  size_t *           param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
{
  size_t result_size = 0;
  void *result_data = NULL;

  // Check event is valid
  if (!event)
  {
    return CL_INVALID_EVENT;
  }
  if (!event->queue)
  {
    return CL_PROFILING_INFO_NOT_AVAILABLE;
  }

  switch (param_name)
  {
  case CL_PROFILING_COMMAND_QUEUED:
    result_size = sizeof(cl_ulong);
    result_data = malloc(result_size);
    *(cl_ulong*)result_data = event->event->queueTime;
    break;
  case CL_PROFILING_COMMAND_SUBMIT:
    result_size = sizeof(cl_ulong);
    result_data = malloc(result_size);
    *(cl_ulong*)result_data = event->event->startTime;
    break;
  case CL_PROFILING_COMMAND_START:
    result_size = sizeof(cl_ulong);
    result_data = malloc(result_size);
    *(cl_ulong*)result_data = event->event->startTime;
    break;
  case CL_PROFILING_COMMAND_END:
    result_size = sizeof(cl_ulong);
    result_data = malloc(result_size);
    *(cl_ulong*)result_data = event->event->endTime;
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
clFlush
(
  cl_command_queue  command_queue
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  // TODO: Implement properly?
  clFinish(command_queue);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clFinish
(
  cl_command_queue  command_queue
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  while (!command_queue->queue->isEmpty())
  {
    // TODO: Move this update to async thread?
    spirsim::Queue::Command *cmd = command_queue->queue->update();
    if (cmd)
    {
      asyncQueueRelease(cmd);
      delete cmd;
    }
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBuffer
(
  cl_command_queue  command_queue,
  cl_mem            buffer,
  cl_bool           blocking_read,
  size_t            offset,
  size_t            cb,
  void *            ptr,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!buffer)
  {
    return CL_INVALID_MEM_OBJECT;
  }
  if (!ptr)
  {
    return CL_INVALID_VALUE;
  }
  if (offset + cb > buffer->size)
  {
    return CL_INVALID_VALUE;
  }

  // Enqueue command
  spirsim::Queue::BufferCommand *cmd =
    new spirsim::Queue::BufferCommand(spirsim::Queue::READ);
  cmd->ptr = (unsigned char*)ptr;
  cmd->address = buffer->address + offset;
  cmd->size = cb;
  asyncQueueRetain(cmd, buffer);
  asyncEnqueue(command_queue, CL_COMMAND_READ_BUFFER, cmd,
               num_events_in_wait_list, event_wait_list, event);

  if (blocking_read)
  {
    return clFinish(command_queue);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBufferRect
(
  cl_command_queue  command_queue,
  cl_mem            buffer,
  cl_bool           blocking_read,
  const size_t *    buffer_origin,
  const size_t *    host_origin,
  const size_t *    region,
  size_t            buffer_row_pitch,
  size_t            buffer_slice_pitch,
  size_t            host_row_pitch,
  size_t            host_slice_pitch,
  void *            ptr,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_1
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!buffer)
  {
    return CL_INVALID_MEM_OBJECT;
  }
  if (!ptr)
  {
    return CL_INVALID_VALUE;
  }

  // Compute pitches if neccessary
  if (buffer_row_pitch == 0)
  {
    buffer_row_pitch = region[0];
  }
  if (buffer_slice_pitch == 0)
  {
    buffer_slice_pitch = region[1] * buffer_row_pitch;
  }
  if (host_row_pitch == 0)
  {
    host_row_pitch = region[0];
  }
  if (host_slice_pitch == 0)
  {
    host_slice_pitch = region[1] * host_row_pitch;
  }

  // Compute origin offsets
  size_t buffer_offset =
    buffer_origin[2] * buffer_slice_pitch +
    buffer_origin[1] * buffer_row_pitch +
    buffer_origin[0];
  size_t host_offset =
    host_origin[2] * host_slice_pitch +
    host_origin[1] * host_row_pitch +
    host_origin[0];

  // Ensure buffer region valid
  size_t end =
    buffer_offset + region[0] +
    (region[1]-1) * buffer_row_pitch +
    (region[2]-1) * buffer_slice_pitch;
  if (end > buffer->size)
  {
    return CL_INVALID_VALUE;
  }

  // Enqueue command
  spirsim::Queue::BufferRectCommand *cmd =
    new spirsim::Queue::BufferRectCommand(spirsim::Queue::READ_RECT);
  cmd->ptr = (unsigned char*)ptr;
  cmd->address = buffer->address;
  cmd->buffer_offset[0] = buffer_offset;
  cmd->buffer_offset[1] = buffer_row_pitch;
  cmd->buffer_offset[2] = buffer_slice_pitch;
  cmd->host_offset[0] = host_offset;
  cmd->host_offset[1] = host_row_pitch;
  cmd->host_offset[2] = host_slice_pitch;
  memcpy(cmd->region, region, 3*sizeof(size_t));
  asyncQueueRetain(cmd, buffer);
  asyncEnqueue(command_queue, CL_COMMAND_READ_BUFFER, cmd,
               num_events_in_wait_list, event_wait_list, event);

  if (blocking_read)
  {
    return clFinish(command_queue);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBuffer
(
  cl_command_queue  command_queue,
  cl_mem            buffer,
  cl_bool           blocking_write,
  size_t            offset,
  size_t            cb,
  const void *      ptr,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!buffer)
  {
    return CL_INVALID_MEM_OBJECT;
  }
  if (!ptr)
  {
    return CL_INVALID_VALUE;
  }
  if (offset + cb > buffer->size)
  {
    return CL_INVALID_VALUE;
  }

  // Enqueue command
  spirsim::Queue::BufferCommand *cmd =
    new spirsim::Queue::BufferCommand(spirsim::Queue::WRITE);
  cmd->ptr = (unsigned char*)ptr;
  cmd->address = buffer->address + offset;
  cmd->size = cb;
  asyncQueueRetain(cmd, buffer);
  asyncEnqueue(command_queue, CL_COMMAND_WRITE_BUFFER, cmd,
               num_events_in_wait_list, event_wait_list, event);

  if (blocking_write)
  {
    return clFinish(command_queue);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBufferRect
(
  cl_command_queue  command_queue,
  cl_mem            buffer,
  cl_bool           blocking_write,
  const size_t *    buffer_origin,
  const size_t *    host_origin,
  const size_t *    region,
  size_t            buffer_row_pitch,
  size_t            buffer_slice_pitch,
  size_t            host_row_pitch,
  size_t            host_slice_pitch,
  const void *      ptr,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_1
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!buffer)
  {
    return CL_INVALID_MEM_OBJECT;
  }
  if (!ptr)
  {
    return CL_INVALID_VALUE;
  }

  // Compute pitches if neccessary
  if (buffer_row_pitch == 0)
  {
    buffer_row_pitch = region[0];
  }
  if (buffer_slice_pitch == 0)
  {
    buffer_slice_pitch = region[1] * buffer_row_pitch;
  }
  if (host_row_pitch == 0)
  {
    host_row_pitch = region[0];
  }
  if (host_slice_pitch == 0)
  {
    host_slice_pitch = region[1] * host_row_pitch;
  }

  // Compute origin offsets
  size_t buffer_offset =
    buffer_origin[2] * buffer_slice_pitch +
    buffer_origin[1] * buffer_row_pitch +
    buffer_origin[0];
  size_t host_offset =
    host_origin[2] * host_slice_pitch +
    host_origin[1] * host_row_pitch +
    host_origin[0];

  // Ensure buffer region valid
  size_t end =
    buffer_offset + region[0] +
    (region[1]-1) * buffer_row_pitch +
    (region[2]-1) * buffer_slice_pitch;
  if (end > buffer->size)
  {
    return CL_INVALID_VALUE;
  }

  // Enqueue command
  spirsim::Queue::BufferRectCommand *cmd =
    new spirsim::Queue::BufferRectCommand(spirsim::Queue::WRITE_RECT);
  cmd->ptr = (unsigned char*)ptr;
  cmd->address = buffer->address;
  cmd->buffer_offset[0] = buffer_offset;
  cmd->buffer_offset[1] = buffer_row_pitch;
  cmd->buffer_offset[2] = buffer_slice_pitch;
  cmd->host_offset[0] = host_offset;
  cmd->host_offset[1] = host_row_pitch;
  cmd->host_offset[2] = host_slice_pitch;
  memcpy(cmd->region, region, 3*sizeof(size_t));
  asyncQueueRetain(cmd, buffer);
  asyncEnqueue(command_queue, CL_COMMAND_WRITE_BUFFER, cmd,
               num_events_in_wait_list, event_wait_list, event);

  if (blocking_write)
  {
    return clFinish(command_queue);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBuffer
(
  cl_command_queue  command_queue,
  cl_mem            src_buffer,
  cl_mem            dst_buffer,
  size_t            src_offset,
  size_t            dst_offset,
  size_t            cb,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!src_buffer || !dst_buffer)
  {
    return CL_INVALID_MEM_OBJECT;
  }
  if (dst_offset + cb > dst_buffer->size ||
      src_offset + cb > src_buffer->size)
  {
    return CL_INVALID_VALUE;
  }

  // Enqueue command
  spirsim::Queue::CopyCommand *cmd = new spirsim::Queue::CopyCommand();
  cmd->dst = dst_buffer->address + dst_offset;
  cmd->src = src_buffer->address + src_offset;
  cmd->size = cb;
  asyncQueueRetain(cmd, src_buffer);
  asyncQueueRetain(cmd, dst_buffer);
  asyncEnqueue(command_queue, CL_COMMAND_COPY_BUFFER, cmd,
               num_events_in_wait_list, event_wait_list, event);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferRect
(
  cl_command_queue  command_queue,
  cl_mem            src_buffer,
  cl_mem            dst_buffer,
  const size_t *    src_origin,
  const size_t *    dst_origin,
  const size_t *    region,
  size_t            src_row_pitch,
  size_t            src_slice_pitch,
  size_t            dst_row_pitch,
  size_t            dst_slice_pitch,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_1
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!src_buffer || !dst_buffer)
  {
    return CL_INVALID_MEM_OBJECT;
  }

  // Compute pitches if neccessary
  if (src_row_pitch == 0)
  {
    src_row_pitch = region[0];
  }
  if (src_slice_pitch == 0)
  {
    src_slice_pitch = region[1] * src_row_pitch;
  }
  if (dst_row_pitch == 0)
  {
    dst_row_pitch = region[0];
  }
  if (dst_slice_pitch == 0)
  {
    dst_slice_pitch = region[1] * dst_row_pitch;
  }

  // Compute origin offsets
  size_t src_offset =
    src_origin[2] * src_slice_pitch +
    src_origin[1] * src_row_pitch +
    src_origin[0];
  size_t dst_offset =
    dst_origin[2] * dst_slice_pitch +
    dst_origin[1] * dst_row_pitch +
    dst_origin[0];

  // Ensure buffer region valid
  size_t src_end =
    src_offset + region[0] +
    (region[1]-1) * src_row_pitch +
    (region[2]-1) * src_slice_pitch;
  size_t dst_end =
    dst_offset + region[0] +
    (region[1]-1) * dst_row_pitch +
    (region[2]-1) * dst_slice_pitch;
  if (src_end > src_buffer->size ||
      dst_end > dst_buffer->size)
  {
    return CL_INVALID_VALUE;
  }

  // Enqueue command
  spirsim::Queue::CopyRectCommand *cmd = new spirsim::Queue::CopyRectCommand();
  cmd->src = src_buffer->address;
  cmd->dst = dst_buffer->address;
  cmd->src_offset[0] = src_offset;
  cmd->src_offset[1] = src_row_pitch;
  cmd->src_offset[2] = src_slice_pitch;
  cmd->dst_offset[0] = dst_offset;
  cmd->dst_offset[1] = dst_row_pitch;
  cmd->dst_offset[2] = dst_slice_pitch;
  memcpy(cmd->region, region, 3*sizeof(size_t));
  asyncQueueRetain(cmd, src_buffer);
  asyncQueueRetain(cmd, dst_buffer);
  asyncEnqueue(command_queue, CL_COMMAND_COPY_BUFFER, cmd,
               num_events_in_wait_list, event_wait_list, event);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueFillBuffer
(
  cl_command_queue  command_queue,
  cl_mem            buffer,
  const void *      pattern,
  size_t            pattern_size,
  size_t            offset,
  size_t            cb,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_2
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!buffer)
  {
    return CL_INVALID_MEM_OBJECT;
  }
  if (offset + cb > buffer->size)
  {
    return CL_INVALID_VALUE;
  }
  if (!pattern || pattern_size == 0)
  {
    return CL_INVALID_VALUE;
  }
  if (offset%pattern_size || cb%pattern_size)
  {
    return CL_INVALID_VALUE;
  }

  // Enqueue command
  spirsim::Queue::FillBufferCommand *cmd =
    new spirsim::Queue::FillBufferCommand((const unsigned char*)pattern,
                                          pattern_size);
  cmd->address = buffer->address + offset;
  cmd->size = cb;
  asyncQueueRetain(cmd, buffer);
  asyncEnqueue(command_queue, CL_COMMAND_FILL_BUFFER, cmd,
               num_events_in_wait_list, event_wait_list, event);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueFillImage
(
  cl_command_queue  command_queue,
  cl_mem            image,
  const void *      fill_color,
  const size_t *    origin,
  const size_t *    region,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_2
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!image)
  {
    return CL_INVALID_MEM_OBJECT;
  }
  if (!fill_color)
  {
    return CL_INVALID_VALUE;
  }

  // Get image dimensions
  cl_image *img = (cl_image*)image;
  size_t width = img->desc.image_width;
  size_t height = img->desc.image_height;
  size_t depth = img->desc.image_depth;
  size_t pixelSize = getPixelSize(&img->format);
  size_t row_pitch = width * pixelSize;
  size_t slice_pitch = height * row_pitch;

  // Compute byte offset and size
  size_t offset = origin[0] * pixelSize
                + origin[1] * row_pitch
                + origin[2] * slice_pitch;
  size_t size = region[0] * pixelSize
              + (region[1]-1) * row_pitch
              + (region[2]-1) * slice_pitch;
  if (offset + size > image->size)
  {
    return CL_INVALID_VALUE;
  }

  // Generate color data with correct order and data type
  unsigned char *color = new unsigned char[pixelSize];
  for (int output = 0; output < getNumChannels(&img->format); output++)
  {
    // Get input channel index
    int input = output;
    switch (img->format.image_channel_order)
    {
      case CL_R:
      case CL_Rx:
      case CL_RG:
      case CL_RGx:
      case CL_RGB:
      case CL_RGBx:
      case CL_RGBA:
        break;
      case CL_BGRA:
        if (output == 0) input = 2;
        if (output == 2) input = 0;
        break;
      case CL_ARGB:
        if (output == 0) input = 3;
        if (output == 1) input = 0;
        if (output == 2) input = 1;
        if (output == 3) input = 2;
        break;
      case CL_A:
        if (output == 0) input = 3;
        break;
      case CL_RA:
        if (output == 1) input = 3;
        break;
      case CL_INTENSITY:
      case CL_LUMINANCE:
        input = 0;
        break;
      default:
        return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
    }

    // Interpret data
    switch (img->format.image_channel_data_type)
    {
    case CL_SNORM_INT8:
      ((int8_t*)color)[output] =
        rint(min(max(((float*)fill_color)[input]*127.f, -127.f), 128.f));
      break;
    case CL_UNORM_INT8:
      ((uint8_t*)color)[output] =
        rint(min(max(((float*)fill_color)[input]*255.f, 0.f), 255.f));
      break;
    case CL_SNORM_INT16:
      ((int16_t*)color)[output] =
        rint(min(max(((float*)fill_color)[input]*32767.f, -32768.f), 32767.f));
      break;
    case CL_UNORM_INT16:
      ((uint16_t*)color)[output] =
        rint(min(max(((float*)fill_color)[input]*65535.f, 0.f), 65535.f));
      break;
    case CL_FLOAT:
      ((float*)color)[output] = ((float*)fill_color)[input];
      break;
    case CL_SIGNED_INT8:
      ((int8_t*)color)[output] = ((int32_t*)fill_color)[input];
      break;
    case CL_SIGNED_INT16:
      ((int16_t*)color)[output] = ((int32_t*)fill_color)[input];
      break;
    case CL_SIGNED_INT32:
      ((int32_t*)color)[output] = ((int32_t*)fill_color)[input];
      break;
    case CL_UNSIGNED_INT8:
      ((uint8_t*)color)[output] = ((uint32_t*)fill_color)[input];
      break;
    case CL_UNSIGNED_INT16:
      ((uint16_t*)color)[output] = ((uint32_t*)fill_color)[input];
      break;
    case CL_UNSIGNED_INT32:
      ((uint32_t*)color)[output] = ((uint32_t*)fill_color)[input];
      break;
    default:
      return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
    }
  }

  // Enqueue command
  spirsim::Queue::FillImageCommand *cmd =
    new spirsim::Queue::FillImageCommand(image->address, origin, region,
                                         row_pitch, slice_pitch,
                                         pixelSize, color);
  asyncQueueRetain(cmd, image);
  asyncEnqueue(command_queue, CL_COMMAND_FILL_IMAGE, cmd,
               num_events_in_wait_list, event_wait_list, event);
  delete[] color;

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadImage
(
  cl_command_queue  command_queue,
  cl_mem            image,
  cl_bool           blocking_read,
  const size_t *    origin,
  const size_t *    region,
  size_t            row_pitch,
  size_t            slice_pitch,
  void *            ptr,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!image)
  {
    return CL_INVALID_MEM_OBJECT;
  }

  cl_image *img = (cl_image*)image;

  size_t pixelSize = getPixelSize(&img->format);
  size_t buffer_origin[3] = {origin[0]*pixelSize, origin[1], origin[2]};
  size_t pixel_region[3] = {region[0]*pixelSize, region[1], region[2]};
  size_t host_origin[3] = {0, 0, 0};

  size_t img_row_pitch = img->desc.image_width * pixelSize;
  size_t img_slice_pitch = img->desc.image_height * img_row_pitch;
  if (row_pitch == 0)
  {
    row_pitch = pixel_region[0];
  }
  if (slice_pitch == 0)
  {
    slice_pitch = pixel_region[1] * row_pitch;
  }

  // TODO: Event should be CL_COMMAND_READ_IMAGE
  return clEnqueueReadBufferRect(command_queue, image, blocking_read,
                                 buffer_origin, host_origin, pixel_region,
                                 img_row_pitch, img_slice_pitch,
                                 row_pitch, slice_pitch,
                                 ptr, num_events_in_wait_list,
                                 event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteImage
(
  cl_command_queue  command_queue,
  cl_mem            image,
  cl_bool           blocking_write,
  const size_t *    origin,
  const size_t *    region,
  size_t            input_row_pitch,
  size_t            input_slice_pitch,
  const void *      ptr,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!image)
  {
    return CL_INVALID_MEM_OBJECT;
  }

  cl_image *img = (cl_image*)image;

  size_t pixelSize = getPixelSize(&img->format);
  size_t buffer_origin[3] = {origin[0]*pixelSize, origin[1], origin[2]};
  size_t pixel_region[3] = {region[0]*pixelSize, region[1], region[2]};
  size_t host_origin[3] = {0, 0, 0};

  size_t img_row_pitch = img->desc.image_width * pixelSize;
  size_t img_slice_pitch = img->desc.image_height * img_row_pitch;
  if (input_row_pitch == 0)
  {
    input_row_pitch = pixel_region[0];
  }
  if (input_slice_pitch == 0)
  {
    input_slice_pitch = pixel_region[1] * input_row_pitch;
  }

  // TODO: Event should be CL_COMMAND_WRITE_IMAGE
  return clEnqueueWriteBufferRect(command_queue, image, blocking_write,
                                  buffer_origin, host_origin, pixel_region,
                                  img_row_pitch, img_slice_pitch,
                                  input_row_pitch, input_slice_pitch,
                                  ptr, num_events_in_wait_list,
                                  event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImage
(
  cl_command_queue  command_queue,
  cl_mem            src_image,
  cl_mem            dst_image,
  const size_t *    src_origin,
  const size_t *    dst_origin,
  const size_t *    region,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!src_image || !dst_image)
  {
    return CL_INVALID_MEM_OBJECT;
  }

  cl_image *src = (cl_image*)src_image;
  cl_image *dst = (cl_image*)dst_image;
  if (src->format.image_channel_order != dst->format.image_channel_order ||
    src->format.image_channel_data_type != dst->format.image_channel_data_type)
  {
    return CL_IMAGE_FORMAT_MISMATCH;
  }

  size_t srcPixelSize = getPixelSize(&src->format);
  size_t dstPixelSize = getPixelSize(&dst->format);

  size_t src_pixel_origin[3] = {src_origin[0]*srcPixelSize,
                                src_origin[1], src_origin[2]};
  size_t dst_pixel_origin[3] = {dst_origin[0]*dstPixelSize,
                                dst_origin[1], dst_origin[2]};
  size_t pixel_region[3] = {region[0]*srcPixelSize, region[1], region[2]};

  size_t src_row_pitch = src->desc.image_width * srcPixelSize;
  size_t src_slice_pitch = src->desc.image_height * src_row_pitch;
  size_t dst_row_pitch = dst->desc.image_width * dstPixelSize;
  size_t dst_slice_pitch = dst->desc.image_height * dst_row_pitch;

  // TODO: Event should be CL_COMMAND_COPY_IMAGE
  return clEnqueueCopyBufferRect(command_queue, src_image, dst_image,
                                 src_pixel_origin, dst_pixel_origin,
                                 pixel_region,
                                 src_row_pitch, src_slice_pitch,
                                 dst_row_pitch, dst_slice_pitch,
                                 num_events_in_wait_list,
                                 event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImageToBuffer
(
  cl_command_queue  command_queue,
  cl_mem            src_image,
  cl_mem            dst_buffer,
  const size_t *    src_origin,
  const size_t *    region,
  size_t            dst_offset,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!src_image || !dst_buffer)
  {
    return CL_INVALID_MEM_OBJECT;
  }

  cl_image *src = (cl_image*)src_image;
  size_t pixel_size = getPixelSize(&src->format);
  size_t src_pixel_origin[3] = {src_origin[0]*pixel_size,
                                src_origin[1], src_origin[2]};
  size_t src_row_pitch = src->desc.image_width * pixel_size;
  size_t src_slice_pitch = src->desc.image_height * src_row_pitch;

  size_t pixel_region[3] = {region[0]*pixel_size, region[1], region[2]};
  size_t dst_origin[3] = {dst_offset, 0, 0};

  // TODO: Event should be CL_COMMAND_COPY_IMAGE_TO_BUFFER
  return clEnqueueCopyBufferRect(command_queue, src_image, dst_buffer,
                                 src_pixel_origin, dst_origin,
                                 pixel_region,
                                 src_row_pitch, src_slice_pitch, 0, 0,
                                 num_events_in_wait_list,
                                 event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferToImage
(
  cl_command_queue  command_queue,
  cl_mem            src_buffer,
  cl_mem            dst_image,
  size_t            src_offset,
  const size_t *    dst_origin,
  const size_t *    region,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!src_buffer || !dst_image)
  {
    return CL_INVALID_MEM_OBJECT;
  }

  cl_image *dst = (cl_image*)dst_image;
  size_t pixel_size = getPixelSize(&dst->format);
  size_t dst_pixel_origin[3] = {dst_origin[0]*pixel_size,
                                dst_origin[1], dst_origin[2]};
  size_t dst_row_pitch = dst->desc.image_width * pixel_size;
  size_t dst_slice_pitch = dst->desc.image_height * dst_row_pitch;

  size_t pixel_region[3] = {region[0]*pixel_size, region[1], region[2]};
  size_t src_origin[3] = {src_offset, 0, 0};

  // TODO: Event should be CL_COMMAND_COPY_BUFFER_TO_IMAGE
  return clEnqueueCopyBufferRect(command_queue, src_buffer, dst_image,
                                 src_origin, dst_pixel_origin,
                                 pixel_region,
                                 0, 0, dst_row_pitch, dst_slice_pitch,
                                 num_events_in_wait_list,
                                 event_wait_list, event);
}

CL_API_ENTRY void* CL_API_CALL
clEnqueueMapBuffer
(
  cl_command_queue  command_queue,
  cl_mem            buffer,
  cl_bool           blocking_map,
  cl_map_flags      map_flags,
  size_t            offset,
  size_t            cb,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event,
  cl_int *          errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    ERRCODE(CL_INVALID_COMMAND_QUEUE);
    return NULL;
  }
  if (!buffer)
  {
    ERRCODE(CL_INVALID_MEM_OBJECT);
    return NULL;
  }

  // Map buffer
  void *ptr = buffer->context->device->getGlobalMemory()->mapBuffer(
    buffer->address, offset, cb);
  if (ptr == NULL)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }

  // Enqueue command
  spirsim::Queue::Command *cmd = new spirsim::Queue::Command();
  asyncQueueRetain(cmd, buffer);
  asyncEnqueue(command_queue, CL_COMMAND_MAP_BUFFER, cmd,
               num_events_in_wait_list, event_wait_list, event);

  ERRCODE(CL_SUCCESS);
  if (blocking_map)
  {
    ERRCODE(clFinish(command_queue));
  }

  return ptr;
}

CL_API_ENTRY void* CL_API_CALL
clEnqueueMapImage
(
  cl_command_queue  command_queue,
  cl_mem            image,
  cl_bool           blocking_map,
  cl_map_flags      map_flags,
  const size_t *    origin,
  const size_t *    region,
  size_t *          image_row_pitch,
  size_t *          image_slice_pitch,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event,
  cl_int *          errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    ERRCODE(CL_INVALID_COMMAND_QUEUE);
    return NULL;
  }
  if (!image)
  {
    ERRCODE(CL_INVALID_MEM_OBJECT);
    return NULL;
  }
  if (!image_row_pitch)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }

  // Get image dimensions
  cl_image *img = (cl_image*)image;
  size_t width = img->desc.image_width;
  size_t height = img->desc.image_height;
  size_t depth = img->desc.image_depth;
  size_t pixelSize = getPixelSize(&img->format);
  size_t row_pitch = width * pixelSize;
  size_t slice_pitch = height * row_pitch;

  // Compute byte offset and size
  size_t offset = origin[0] * pixelSize
                + origin[1] * row_pitch
                + origin[2] * slice_pitch;
  size_t size = region[0] * pixelSize
              + (region[1]-1) * row_pitch
              + (region[2]-1) * slice_pitch;

  // Map image
  void *ptr = image->context->device->getGlobalMemory()->mapBuffer(
        image->address, offset, size);
  if (ptr == NULL)
  {
    ERRCODE(CL_INVALID_VALUE);
    return NULL;
  }

  *image_row_pitch = row_pitch;
  if (image_slice_pitch)
  {
    *image_slice_pitch = slice_pitch;
  }

  // Enqueue command
  spirsim::Queue::Command *cmd = new spirsim::Queue::Command();
  asyncQueueRetain(cmd, image);
  asyncEnqueue(command_queue, CL_COMMAND_MAP_IMAGE, cmd,
               num_events_in_wait_list, event_wait_list, event);

  ERRCODE(CL_SUCCESS);
  if (blocking_map)
  {
    ERRCODE(clFinish(command_queue));
  }

  return ptr;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueUnmapMemObject
(
  cl_command_queue  command_queue,
  cl_mem            memobj,
  void *            mapped_ptr,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!memobj)
  {
    return CL_INVALID_MEM_OBJECT;
  }

  // Enqueue command
  spirsim::Queue::Command *cmd = new spirsim::Queue::Command();
  asyncQueueRetain(cmd, memobj);
  asyncEnqueue(command_queue, CL_COMMAND_UNMAP_MEM_OBJECT, cmd,
               num_events_in_wait_list, event_wait_list, event);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMigrateMemObjects
(
  cl_command_queue        command_queue,
  cl_uint                 num_mem_objects,
  const cl_mem *          mem_objects,
  cl_mem_migration_flags  flags,
  cl_uint                 num_events_in_wait_list,
  const cl_event *        event_wait_list,
  cl_event *              event
) CL_API_SUFFIX__VERSION_1_2
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  // Enqueue command
  spirsim::Queue::Command *cmd = new spirsim::Queue::Command();
  asyncEnqueue(command_queue, CL_COMMAND_MIGRATE_MEM_OBJECTS, cmd,
               num_events_in_wait_list, event_wait_list, event);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNDRangeKernel
(
  cl_command_queue  command_queue,
  cl_kernel         kernel,
  cl_uint           work_dim,
  const size_t *    global_work_offset,
  const size_t *    global_work_size,
  const size_t *    local_work_size,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
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

  // Check global and local sizes are valid
  for (int i = 0; i < work_dim; i++)
  {
    if (!global_work_size[i])
    {
      return CL_INVALID_GLOBAL_WORK_SIZE;
    }
    if (local_work_size && global_work_size[i] % local_work_size[i])
    {
      return CL_INVALID_WORK_GROUP_SIZE;
    }
  }

  // Ensure all arguments have been set
  if (!kernel->kernel->allArgumentsSet())
  {
    return CL_INVALID_KERNEL_ARGS;
  }

  // Enqueue command
  spirsim::Queue::KernelCommand *cmd = new spirsim::Queue::KernelCommand();
  cmd->kernel = new spirsim::Kernel(*kernel->kernel);
  cmd->work_dim = work_dim;
  memcpy(cmd->global_size, global_work_size, work_dim*sizeof(size_t));
  memset(cmd->global_offset, 0, 3*sizeof(size_t));
  memset(cmd->local_size, 0, 3*sizeof(size_t));
  if (global_work_offset)
  {
    memcpy(cmd->global_offset, global_work_offset, work_dim*sizeof(size_t));
  }
  if (local_work_size)
  {
    memcpy(cmd->local_size, local_work_size, work_dim*sizeof(size_t));
  }
  asyncQueueRetain(cmd, kernel);
  asyncEnqueue(command_queue, CL_COMMAND_NDRANGE_KERNEL, cmd,
               num_events_in_wait_list, event_wait_list, event);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueTask
(
  cl_command_queue  command_queue,
  cl_kernel         kernel,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  size_t work = 1;
  return clEnqueueNDRangeKernel(command_queue, kernel, 1,
                                NULL, &work, &work,
                                num_events_in_wait_list,
                                event_wait_list,
                                event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNativeKernel
(
  cl_command_queue  command_queue,
  void (CL_CALLBACK *user_func)(void *),
  void *            args,
  size_t            cb_args,
  cl_uint           num_mem_objects,
  const cl_mem *    mem_list,
  const void **     args_mem_loc,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }
  if (!user_func)
  {
    return CL_INVALID_VALUE;
  }
  if (!args && (cb_args > 0 || num_mem_objects > 0))
  {
    return CL_INVALID_VALUE;
  }
  if (args && cb_args == 0)
  {
    return CL_INVALID_VALUE;
  }
  if ((num_mem_objects > 0 && (!mem_list || !args_mem_loc)) ||
      (num_mem_objects == 0 && (mem_list || !args_mem_loc)))
  {
    return CL_INVALID_VALUE;
  }

  // Replace mem objects with real pointers
  spirsim::Memory *memory = command_queue->context->device->getGlobalMemory();
  for (int i = 0; i < num_mem_objects; i++)
  {
    void *addr = memory->getPointer(mem_list[i]->address);
    if (addr == NULL)
    {
      return CL_INVALID_VALUE;
    }
    memcpy((void*)args_mem_loc[i], &addr, sizeof(void*));
  }

  // Create command
  spirsim::Queue::NativeKernelCommand *cmd =
    new spirsim::Queue::NativeKernelCommand(user_func, args, cb_args);

  // Retain memory objects
  for (int i = 0; i < num_mem_objects; i++)
  {
    asyncQueueRetain(cmd, mem_list[i]);
  }

  // Enqueue commands
  asyncEnqueue(command_queue, CL_COMMAND_NATIVE_KERNEL, cmd,
               num_events_in_wait_list, event_wait_list, event);

  return CL_SUCCESS;
}

CL_API_ENTRY void* CL_API_CALL
clGetExtensionFunctionAddressForPlatform
(
  cl_platform_id  platform,
  const char *    func_name
) CL_API_SUFFIX__VERSION_1_2
{
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMarkerWithWaitList
(
  cl_command_queue  command_queue,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_2
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  // Enqueue command
  spirsim::Queue::Command *cmd = new spirsim::Queue::Command();
  asyncEnqueue(command_queue, CL_COMMAND_MARKER, cmd,
               num_events_in_wait_list, event_wait_list, event);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBarrierWithWaitList
(
  cl_command_queue  command_queue,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_2
{
  // Check parameters
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  // Enqueue command
  spirsim::Queue::Command *cmd = new spirsim::Queue::Command();
  asyncEnqueue(command_queue, CL_COMMAND_BARRIER, cmd,
               num_events_in_wait_list, event_wait_list, event);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetPrintfCallback
(
  cl_context           context,
  void (CL_CALLBACK *  pfn_notify)(cl_context, cl_uint, char*, void*),
  void *               user_data
) CL_API_SUFFIX__VERSION_1_2
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMarker
(
  cl_command_queue  command_queue,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  return clEnqueueMarkerWithWaitList(command_queue, 0, NULL, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWaitForEvents
(
  cl_command_queue  command_queue,
  cl_uint           num_events,
  const cl_event *  event_list
) CL_API_SUFFIX__VERSION_1_0
{
  if (!command_queue)
  {
    return CL_INVALID_COMMAND_QUEUE;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBarrier
(
  cl_command_queue  command_queue
) CL_API_SUFFIX__VERSION_1_0
{
  return clEnqueueBarrierWithWaitList(command_queue, 0, NULL, NULL);
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLBuffer
(
  cl_context    context,
  cl_mem_flags  flags,
  cl_GLuint     bufret_mem,
  int *         errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  ERRCODE(CL_INVALID_CONTEXT);
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLTexture
(
  cl_context    context,
  cl_mem_flags  flags,
  cl_GLenum     target,
  cl_GLint      miplevel,
  cl_GLuint     texture,
  cl_int *      errcode_ret
) CL_API_SUFFIX__VERSION_1_2
{
  ERRCODE(CL_INVALID_CONTEXT);
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLTexture2D
(
  cl_context    context,
  cl_mem_flags  flags,
  cl_GLenum     target,
  cl_GLint      miplevel,
  cl_GLuint     texture,
  cl_int *      errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  ERRCODE(CL_INVALID_CONTEXT);
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLTexture3D
(
  cl_context    context,
  cl_mem_flags  flags,
  cl_GLenum     target,
  cl_GLint      miplevel,
  cl_GLuint     texture,
  cl_int *      errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  ERRCODE(CL_INVALID_CONTEXT);
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLRenderbuffer
(
  cl_context    context,
  cl_mem_flags  flags,
  cl_GLuint     renderbuffer,
  cl_int *      errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  ERRCODE(CL_INVALID_CONTEXT);
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetGLObjectInfo
(
  cl_mem               memobj,
  cl_gl_object_type *  gl_object_type,
  cl_GLuint *          gl_object_name
) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_MEM_OBJECT;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetGLTextureInfo
(
  cl_mem              memobj,
  cl_gl_texture_info  param_name,
  size_t              param_value_size,
  void *              param_value,
  size_t *            param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_MEM_OBJECT;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueAcquireGLObjects
(
  cl_command_queue  command_queue,
  cl_uint           num_objects,
  const cl_mem *    mem_objects,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_CONTEXT;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReleaseGLObjects
(
  cl_command_queue  command_queue,
  cl_uint           num_objects,
  const cl_mem *    mem_objects,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_CONTEXT;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetGLContextInfoKHR
(
  const cl_context_properties *  properties,
  cl_gl_context_info             param_name,
  size_t                         param_value_size,
  void *                         param_value,
  size_t *                       param_value_size_ret
) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_event CL_API_CALL
clCreateEventFromGLsyncKHR
(
  cl_context  context,
  cl_GLsync   cl_GLsync,
  cl_int *    errcode_ret
) CL_EXT_SUFFIX__VERSION_1_1
{
  ERRCODE(CL_INVALID_CONTEXT);
  return NULL;
}

////////////////////
// Dispatch Table //
////////////////////

#define _NULL_ NULL
#define DISPATCH_TABLE_ENTRY(FUNCTION) (void*)(FUNCTION)
void *m_dispatchTable[] =
{
  DISPATCH_TABLE_ENTRY(clGetPlatformIDs),
  DISPATCH_TABLE_ENTRY(clGetPlatformInfo),
  DISPATCH_TABLE_ENTRY(clGetDeviceIDs),
  DISPATCH_TABLE_ENTRY(clGetDeviceInfo),
  DISPATCH_TABLE_ENTRY(clCreateContext),
  DISPATCH_TABLE_ENTRY(clCreateContextFromType),
  DISPATCH_TABLE_ENTRY(clRetainContext),
  DISPATCH_TABLE_ENTRY(clReleaseContext),
  DISPATCH_TABLE_ENTRY(clGetContextInfo),
  DISPATCH_TABLE_ENTRY(clCreateCommandQueue),
  DISPATCH_TABLE_ENTRY(clRetainCommandQueue),
  DISPATCH_TABLE_ENTRY(clReleaseCommandQueue),
  DISPATCH_TABLE_ENTRY(clGetCommandQueueInfo),
  DISPATCH_TABLE_ENTRY(clSetCommandQueueProperty),
  DISPATCH_TABLE_ENTRY(clCreateBuffer),
  DISPATCH_TABLE_ENTRY(clCreateImage2D),
  DISPATCH_TABLE_ENTRY(clCreateImage3D),
  DISPATCH_TABLE_ENTRY(clRetainMemObject),
  DISPATCH_TABLE_ENTRY(clReleaseMemObject),
  DISPATCH_TABLE_ENTRY(clGetSupportedImageFormats),
  DISPATCH_TABLE_ENTRY(clGetMemObjectInfo),
  DISPATCH_TABLE_ENTRY(clGetImageInfo),
  DISPATCH_TABLE_ENTRY(clCreateSampler),
  DISPATCH_TABLE_ENTRY(clRetainSampler),
  DISPATCH_TABLE_ENTRY(clReleaseSampler),
  DISPATCH_TABLE_ENTRY(clGetSamplerInfo),
  DISPATCH_TABLE_ENTRY(clCreateProgramWithSource),
  DISPATCH_TABLE_ENTRY(clCreateProgramWithBinary),
  DISPATCH_TABLE_ENTRY(clRetainProgram),
  DISPATCH_TABLE_ENTRY(clReleaseProgram),
  DISPATCH_TABLE_ENTRY(clBuildProgram),
  DISPATCH_TABLE_ENTRY(clUnloadCompiler),
  DISPATCH_TABLE_ENTRY(clGetProgramInfo),
  DISPATCH_TABLE_ENTRY(clGetProgramBuildInfo),
  DISPATCH_TABLE_ENTRY(clCreateKernel),
  DISPATCH_TABLE_ENTRY(clCreateKernelsInProgram),
  DISPATCH_TABLE_ENTRY(clRetainKernel),
  DISPATCH_TABLE_ENTRY(clReleaseKernel),
  DISPATCH_TABLE_ENTRY(clSetKernelArg),
  DISPATCH_TABLE_ENTRY(clGetKernelInfo),
  DISPATCH_TABLE_ENTRY(clGetKernelWorkGroupInfo),
  DISPATCH_TABLE_ENTRY(clWaitForEvents),
  DISPATCH_TABLE_ENTRY(clGetEventInfo),
  DISPATCH_TABLE_ENTRY(clRetainEvent),
  DISPATCH_TABLE_ENTRY(clReleaseEvent),
  DISPATCH_TABLE_ENTRY(clGetEventProfilingInfo),
  DISPATCH_TABLE_ENTRY(clFlush),
  DISPATCH_TABLE_ENTRY(clFinish),
  DISPATCH_TABLE_ENTRY(clEnqueueReadBuffer),
  DISPATCH_TABLE_ENTRY(clEnqueueWriteBuffer),
  DISPATCH_TABLE_ENTRY(clEnqueueCopyBuffer),
  DISPATCH_TABLE_ENTRY(clEnqueueReadImage),
  DISPATCH_TABLE_ENTRY(clEnqueueWriteImage),
  DISPATCH_TABLE_ENTRY(clEnqueueCopyImage),
  DISPATCH_TABLE_ENTRY(clEnqueueCopyImageToBuffer),
  DISPATCH_TABLE_ENTRY(clEnqueueCopyBufferToImage),
  DISPATCH_TABLE_ENTRY(clEnqueueMapBuffer),
  DISPATCH_TABLE_ENTRY(clEnqueueMapImage),
  DISPATCH_TABLE_ENTRY(clEnqueueUnmapMemObject),
  DISPATCH_TABLE_ENTRY(clEnqueueNDRangeKernel),
  DISPATCH_TABLE_ENTRY(clEnqueueTask),
  DISPATCH_TABLE_ENTRY(clEnqueueNativeKernel),
  DISPATCH_TABLE_ENTRY(clEnqueueMarker),
  DISPATCH_TABLE_ENTRY(clEnqueueWaitForEvents),
  DISPATCH_TABLE_ENTRY(clEnqueueBarrier),
  DISPATCH_TABLE_ENTRY(clGetExtensionFunctionAddress),
  DISPATCH_TABLE_ENTRY(clCreateFromGLBuffer),
  DISPATCH_TABLE_ENTRY(clCreateFromGLTexture2D),
  DISPATCH_TABLE_ENTRY(clCreateFromGLTexture3D),
  DISPATCH_TABLE_ENTRY(clCreateFromGLRenderbuffer),
  DISPATCH_TABLE_ENTRY(clGetGLObjectInfo),
  DISPATCH_TABLE_ENTRY(clGetGLTextureInfo),
  DISPATCH_TABLE_ENTRY(clEnqueueAcquireGLObjects),
  DISPATCH_TABLE_ENTRY(clEnqueueReleaseGLObjects),

  DISPATCH_TABLE_ENTRY(clGetGLContextInfoKHR),

#if defined(_WIN32)
  DISPATCH_TABLE_ENTRY(NULL); // clGetDeviceIDsFromD3D10KHR
  DISPATCH_TABLE_ENTRY(NULL); // clCreateFromD3D10BufferKHR
  DISPATCH_TABLE_ENTRY(NULL); // clCreateFromD3D10Texture2DKHR
  DISPATCH_TABLE_ENTRY(NULL); // clCreateFromD3D10Texture3DKHR
  DISPATCH_TABLE_ENTRY(NULL); // clEnqueueAcquireD3D10ObjectsKHR
  DISPATCH_TABLE_ENTRY(NULL); // clEnqueueReleaseD3D10ObjectsKHR
#else
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
#endif

  // OpenCL 1.1
  DISPATCH_TABLE_ENTRY(clSetEventCallback),
  DISPATCH_TABLE_ENTRY(clCreateSubBuffer),
  DISPATCH_TABLE_ENTRY(clSetMemObjectDestructorCallback),
  DISPATCH_TABLE_ENTRY(clCreateUserEvent),
  DISPATCH_TABLE_ENTRY(clSetUserEventStatus),
  DISPATCH_TABLE_ENTRY(clEnqueueReadBufferRect),
  DISPATCH_TABLE_ENTRY(clEnqueueWriteBufferRect),
  DISPATCH_TABLE_ENTRY(clEnqueueCopyBufferRect),

  DISPATCH_TABLE_ENTRY(NULL), // clCreateSubDevicesEXT
  DISPATCH_TABLE_ENTRY(NULL), // clRetainDeviceEXT
  DISPATCH_TABLE_ENTRY(NULL), // clReleaseDeviceEXT

  DISPATCH_TABLE_ENTRY(clCreateEventFromGLsyncKHR),

  // OpenCL 1.2
  DISPATCH_TABLE_ENTRY(clCreateSubDevices),
  DISPATCH_TABLE_ENTRY(clRetainDevice),
  DISPATCH_TABLE_ENTRY(clReleaseDevice),
  DISPATCH_TABLE_ENTRY(clCreateImage),
  DISPATCH_TABLE_ENTRY(clCreateProgramWithBuiltInKernels),
  DISPATCH_TABLE_ENTRY(clCompileProgram),
  DISPATCH_TABLE_ENTRY(clLinkProgram),
  DISPATCH_TABLE_ENTRY(clUnloadPlatformCompiler),
  DISPATCH_TABLE_ENTRY(clGetKernelArgInfo),
  DISPATCH_TABLE_ENTRY(clEnqueueFillBuffer),
  DISPATCH_TABLE_ENTRY(clEnqueueFillImage),
  DISPATCH_TABLE_ENTRY(clEnqueueMigrateMemObjects),
  DISPATCH_TABLE_ENTRY(clEnqueueMarkerWithWaitList),
  DISPATCH_TABLE_ENTRY(clEnqueueBarrierWithWaitList),
  DISPATCH_TABLE_ENTRY(clGetExtensionFunctionAddressForPlatform),
  DISPATCH_TABLE_ENTRY(clCreateFromGLTexture),

#if defined(_WIN32)
  DISPATCH_TABLE_ENTRY(NULL); // clGetDeviceIDsFromD3D11KHR
  DISPATCH_TABLE_ENTRY(NULL); // clCreateFromD3D11BufferKHR
  DISPATCH_TABLE_ENTRY(NULL); // clCreateFromD3D11Texture2DKHR
  DISPATCH_TABLE_ENTRY(NULL); // clCreateFromD3D11Texture3DKHR
  DISPATCH_TABLE_ENTRY(NULL); // clCreateFromDX9MediaSurfaceKHR
  DISPATCH_TABLE_ENTRY(NULL); // clEnqueueAcquireD3D11ObjectsKHR
  DISPATCH_TABLE_ENTRY(NULL); // clEnqueueReleaseD3D11ObjectsKHR
  DISPATCH_TABLE_ENTRY(NULL); // clGetDeviceIDsFromDX9MediaAdapterKHR
  DISPATCH_TABLE_ENTRY(NULL); // clEnqueueAcquireDX9MediaSurfacesKHR
  DISPATCH_TABLE_ENTRY(NULL); // clEnqueueReleaseDX9MediaSurfacesKHR
#else
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
#endif
};
