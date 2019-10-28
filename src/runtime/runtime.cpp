// runtime.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "config.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>

#include "async_queue.h"
#include "icd.h"

#include "core/Context.h"
#include "core/Kernel.h"
#include "core/half.h"
#include "core/Memory.h"
#include "core/Program.h"
#include "core/Queue.h"

using namespace std;

#define DEFAULT_GLOBAL_MEM_SIZE   (128 * 1048576)
#define DEFAULT_CONSTANT_MEM_SIZE (65536)
#define DEFAULT_LOCAL_MEM_SIZE    (32768)
#define DEFAULT_MAX_WGSIZE        (1024)

#define PLATFORM_NAME       "Oclgrind"
#define PLATFORM_VENDOR     "University of Bristol"
#define PLATFORM_VERSION    "OpenCL 1.2 (Oclgrind " PACKAGE_VERSION ")"
#define PLATFORM_PROFILE    "FULL_PROFILE"
#define PLATFORM_SUFFIX     "oclg"
#define PLATFORM_EXTENSIONS "cl_khr_icd"

#define DEVICE_NAME          "Oclgrind Simulator"
#define DEVICE_VENDOR        "University of Bristol"
#define DEVICE_VENDOR_ID     0x0042
#define DEVICE_VERSION       "OpenCL 1.2 (Oclgrind " PACKAGE_VERSION ")"
#define DEVICE_LANG_VERSION  "OpenCL C 1.2 (Oclgrind " PACKAGE_VERSION ")"
#define DRIVER_VERSION       "Oclgrind " PACKAGE_VERSION
#define DEVICE_PROFILE       "FULL_PROFILE"
#define DEVICE_SPIR_VERSIONS "1.2"
#define DEVICE_EXTENSIONS    "         \
  cl_khr_spir                          \
  cl_khr_3d_image_writes               \
  cl_khr_global_int32_base_atomics     \
  cl_khr_global_int32_extended_atomics \
  cl_khr_local_int32_base_atomics      \
  cl_khr_local_int32_extended_atomics  \
  cl_khr_int64_base_atomics            \
  cl_khr_int64_extended_atomics        \
  cl_khr_byte_addressable_store        \
  cl_khr_fp64"
#define DEVICE_TYPE (CL_DEVICE_TYPE_CPU | \
                     CL_DEVICE_TYPE_GPU | \
                     CL_DEVICE_TYPE_ACCELERATOR | \
                     CL_DEVICE_TYPE_DEFAULT)


namespace
{
#define CASE(X) case X: return #X;
  const char* CLErrorToString(cl_int err)
  {
    switch (err)
    {
      CASE(CL_SUCCESS)
      CASE(CL_DEVICE_NOT_FOUND)
      CASE(CL_DEVICE_NOT_AVAILABLE)
      CASE(CL_COMPILER_NOT_AVAILABLE)
      CASE(CL_MEM_OBJECT_ALLOCATION_FAILURE)
      CASE(CL_OUT_OF_RESOURCES)
      CASE(CL_OUT_OF_HOST_MEMORY)
      CASE(CL_PROFILING_INFO_NOT_AVAILABLE)
      CASE(CL_MEM_COPY_OVERLAP)
      CASE(CL_IMAGE_FORMAT_MISMATCH)
      CASE(CL_IMAGE_FORMAT_NOT_SUPPORTED)
      CASE(CL_BUILD_PROGRAM_FAILURE)
      CASE(CL_MAP_FAILURE)
      CASE(CL_MISALIGNED_SUB_BUFFER_OFFSET)
      CASE(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST)
      CASE(CL_COMPILE_PROGRAM_FAILURE)
      CASE(CL_LINKER_NOT_AVAILABLE)
      CASE(CL_LINK_PROGRAM_FAILURE)
      CASE(CL_DEVICE_PARTITION_FAILED)
      CASE(CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
      CASE(CL_INVALID_VALUE)
      CASE(CL_INVALID_DEVICE_TYPE)
      CASE(CL_INVALID_PLATFORM)
      CASE(CL_INVALID_DEVICE)
      CASE(CL_INVALID_CONTEXT)
      CASE(CL_INVALID_QUEUE_PROPERTIES)
      CASE(CL_INVALID_COMMAND_QUEUE)
      CASE(CL_INVALID_HOST_PTR)
      CASE(CL_INVALID_MEM_OBJECT)
      CASE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR)
      CASE(CL_INVALID_IMAGE_SIZE)
      CASE(CL_INVALID_SAMPLER)
      CASE(CL_INVALID_BINARY)
      CASE(CL_INVALID_BUILD_OPTIONS)
      CASE(CL_INVALID_PROGRAM)
      CASE(CL_INVALID_PROGRAM_EXECUTABLE)
      CASE(CL_INVALID_KERNEL_NAME)
      CASE(CL_INVALID_KERNEL_DEFINITION)
      CASE(CL_INVALID_KERNEL)
      CASE(CL_INVALID_ARG_INDEX)
      CASE(CL_INVALID_ARG_VALUE)
      CASE(CL_INVALID_ARG_SIZE)
      CASE(CL_INVALID_KERNEL_ARGS)
      CASE(CL_INVALID_WORK_DIMENSION)
      CASE(CL_INVALID_WORK_GROUP_SIZE)
      CASE(CL_INVALID_WORK_ITEM_SIZE)
      CASE(CL_INVALID_GLOBAL_OFFSET)
      CASE(CL_INVALID_EVENT_WAIT_LIST)
      CASE(CL_INVALID_EVENT)
      CASE(CL_INVALID_OPERATION)
      CASE(CL_INVALID_GL_OBJECT)
      CASE(CL_INVALID_BUFFER_SIZE)
      CASE(CL_INVALID_MIP_LEVEL)
      CASE(CL_INVALID_GLOBAL_WORK_SIZE)
      CASE(CL_INVALID_PROPERTY)
      CASE(CL_INVALID_IMAGE_DESCRIPTOR)
      CASE(CL_INVALID_COMPILER_OPTIONS)
      CASE(CL_INVALID_LINKER_OPTIONS)
      CASE(CL_INVALID_DEVICE_PARTITION_COUNT)
    }
    return "Unknown";
  }
#undef CASE

  void notifyAPIError(cl_context context, cl_int err,
                      const char* function, string info = "")
  {
    // Remove leading underscore from function name if necessary
    if (!strncmp(function, "_cl", 3))
    {
      function++;
    }

    // Build error message
    ostringstream oss;
    oss << endl
        << "Oclgrind - OpenCL runtime error detected" << endl
        << "\tFunction: " << function << endl
        << "\tError:    " << CLErrorToString(err) << endl;
    if (!info.empty())
    {
      oss << "\t" << info << endl;
    }
    string error = oss.str();

    // Output message to stderr if required
    if (oclgrind::checkEnv("OCLGRIND_CHECK_API"))
    {
      cerr << error << endl;
    }

    // Fire context callback if set
    if (context && context->notify)
    {
      context->notify(error.c_str(), context->data, 0, NULL);
    }
  }

  void releaseCommand(oclgrind::Command *command)
  {
    if (command)
    {
      asyncQueueRelease(command);

      // Release dependent commands
      while (!command->execBefore.empty())
      {
        oclgrind::Command *cmd = command->execBefore.front();
        command->execBefore.pop_front();
        releaseCommand(cmd);
      }

      delete command;
    }
  }
}

#if defined(_WIN32) && !defined(__MINGW32__)
#define __func__ __FUNCTION__
#endif

#define ReturnErrorInfo(context, err, info)          \
{                                                    \
  ostringstream oss;                                 \
  oss << info;                                       \
  notifyAPIError(context, err, __func__, oss.str()); \
  return err;                                        \
}
#define ReturnErrorArg(context, err, arg) \
  ReturnErrorInfo(context, err, "For argument '" #arg "'")
#define ReturnError(context, err) \
  ReturnErrorInfo(context, err, "")

#define SetErrorInfo(context, err, info)               \
  if (err != CL_SUCCESS)                               \
  {                                                    \
    ostringstream oss;                                 \
    oss << info;                                       \
    notifyAPIError(context, err, __func__, oss.str()); \
  }                                                    \
  if (errcode_ret)                                     \
  {                                                    \
    *errcode_ret = err;                                \
  }
#define SetErrorArg(context, err, arg) \
  SetErrorInfo(context, err, "For argument '" #arg "'")
#define SetError(context, err) \
  SetErrorInfo(context, err, "")

#define ParamValueSizeTooSmall                        \
  "param_value_size is " << param_value_size <<       \
  ", but result requires " << result_size << " bytes"


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
  if (platforms && num_entries < 1)
  {
    ReturnError(NULL, CL_INVALID_VALUE);
  }

  if (!m_platform)
  {
    m_platform = new _cl_platform_id;
    m_platform->dispatch = m_dispatchTable;

    m_device = new _cl_device_id;
    m_device->dispatch = m_dispatchTable;
    m_device->globalMemSize =
      oclgrind::getEnvInt("OCLGRIND_GLOBAL_MEM_SIZE",
                          DEFAULT_GLOBAL_MEM_SIZE, false);
    m_device->constantMemSize =
      oclgrind::getEnvInt("OCLGRIND_CONSTANT_MEM_SIZE",
                          DEFAULT_CONSTANT_MEM_SIZE, false);
    m_device->localMemSize =
      oclgrind::getEnvInt("OCLGRIND_LOCAL_MEM_SIZE",
                          DEFAULT_LOCAL_MEM_SIZE, false);
    m_device->maxWGSize =
      oclgrind::getEnvInt("OCLGRIND_MAX_WGSIZE", DEFAULT_MAX_WGSIZE, false);
  }

  if (platforms)
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
  else if (strcmp(funcname, "clGetPlatformInfo") == 0)
  {
    return (void*)clGetPlatformInfo;
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
  // Select platform info string
  const char *result = NULL;
  switch(param_name)
  {
  case CL_PLATFORM_PROFILE:
    result = PLATFORM_PROFILE;
    break;
  case CL_PLATFORM_VERSION:
    result = PLATFORM_VERSION;
    break;
  case CL_PLATFORM_NAME:
    result = PLATFORM_NAME;
    break;
  case CL_PLATFORM_VENDOR:
    result = PLATFORM_VENDOR;
    break;
  case CL_PLATFORM_EXTENSIONS:
    result = PLATFORM_EXTENSIONS;
    break;
  case CL_PLATFORM_ICD_SUFFIX_KHR:
    result = PLATFORM_SUFFIX;
    break;
  default:
    ReturnErrorArg(NULL, CL_INVALID_VALUE, param_name);
  }

  // Compute size of result
  size_t result_size = strlen(result) + 1;
  if (param_value_size_ret)
  {
    *param_value_size_ret = result_size;
  }

  // Return result
  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(NULL, CL_INVALID_VALUE, ParamValueSizeTooSmall);
    }
    else
    {
      memcpy(param_value, result, result_size);
    }
  }

  return CL_SUCCESS;
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
    ReturnError(NULL, CL_INVALID_VALUE);
  }

  if (!(device_type & DEVICE_TYPE))
  {
    ReturnError(NULL, CL_DEVICE_NOT_FOUND);
  }

  if (devices)
  {
    *devices = m_device;
  }

  if (num_devices)
  {
    *num_devices = 1;
  }

  return CL_SUCCESS;
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
  // Check device is valid
  if (device != m_device)
  {
    ReturnErrorArg(NULL, CL_INVALID_DEVICE, device);
  }

  size_t dummy;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;
  // All possible return types
  union
  {
    cl_uint cluint;
    size_t sizet;
    size_t sizet3[3];
    cl_ulong clulong;
    cl_bool clbool;
    cl_device_id cldeviceid;
    cl_device_type cldevicetype;
    cl_device_fp_config devicefpconfig;
    cl_device_mem_cache_type devicememcachetype;
    cl_device_local_mem_type devicelocalmemtype;
    cl_device_exec_capabilities cldevexeccap;
    cl_command_queue_properties clcmdqprop;
    cl_platform_id clplatid;
    cl_device_partition_property cldevpartprop;
    cl_device_affinity_domain cldevaffdom;
    cl_device_svm_capabilities svm;
  } result_data;
  // The result is actually a string that needs copying
  const char* str = 0;

  switch (param_name)
  {
  case CL_DEVICE_TYPE:
    result_size = sizeof(cl_device_type);
    result_data.cldevicetype = DEVICE_TYPE;
    break;
  case CL_DEVICE_VENDOR_ID:
    result_size = sizeof(cl_uint);
    result_data.cluint = DEVICE_VENDOR_ID;
    break;
  case CL_DEVICE_MAX_COMPUTE_UNITS:
    result_size = sizeof(cl_uint);
    result_data.cluint =
        oclgrind::getEnvInt("OCLGRIND_COMPUTE_UNITS", 1, false);
    break;
  case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
    result_size = sizeof(cl_uint);
    result_data.cluint = 3;
    break;
  case CL_DEVICE_MAX_WORK_GROUP_SIZE:
    result_size = sizeof(size_t);
    result_data.sizet = m_device->maxWGSize;
    break;
  case CL_DEVICE_MAX_WORK_ITEM_SIZES:
    result_size = 3*sizeof(size_t);
    result_data.sizet3[0] = m_device->maxWGSize;
    result_data.sizet3[1] = m_device->maxWGSize;
    result_data.sizet3[2] = m_device->maxWGSize;
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1;
    break;
  case CL_DEVICE_MAX_CLOCK_FREQUENCY:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1;
    break;
  case CL_DEVICE_ADDRESS_BITS:
    result_size = sizeof(cl_uint);
    result_data.cluint = sizeof(size_t)<<3;
    break;
  case CL_DEVICE_MAX_READ_IMAGE_ARGS:
    result_size = sizeof(cl_uint);
    result_data.cluint = 128;
    break;
  case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
    result_size = sizeof(cl_uint);
    result_data.cluint = 64;
    break;
  case CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS:
    result_size = sizeof(cl_uint);
    result_data.cluint = 64;
    break;
  case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
    result_size = sizeof(cl_ulong);
    result_data.clulong = m_device->globalMemSize;
    break;
  case CL_DEVICE_IMAGE2D_MAX_WIDTH:
  case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
    result_size = sizeof(size_t);
    result_data.sizet = 8192;
    break;
  case CL_DEVICE_IMAGE3D_MAX_WIDTH:
  case CL_DEVICE_IMAGE3D_MAX_DEPTH:
  case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
    result_size = sizeof(size_t);
    result_data.sizet = 2048;
    break;
  case CL_DEVICE_IMAGE_SUPPORT:
    result_size = sizeof(cl_bool);
    result_data.clbool = CL_TRUE;
    break;
  case CL_DEVICE_MAX_PARAMETER_SIZE:
    result_size = sizeof(size_t);
    result_data.sizet = 1024;
    break;
  case CL_DEVICE_MAX_SAMPLERS:
    result_size = sizeof(cl_uint);
    result_data.cluint = 16;
    break;
  case CL_DEVICE_IMAGE_PITCH_ALIGNMENT:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1;
    break;
  case CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT:
    result_size = sizeof(cl_uint);
    result_data.cluint = 4;
    break;
  case CL_DEVICE_MAX_PIPE_ARGS:
    result_size = sizeof(cl_uint);
    result_data.cluint = 16;
    break;
  case CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1;
    break;
  case CL_DEVICE_PIPE_MAX_PACKET_SIZE:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1024;
    break;
  case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
    result_size = sizeof(cl_uint);
    result_data.cluint = sizeof(cl_long16)<<3;
    break;
  case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1;
    break;
  case CL_DEVICE_SINGLE_FP_CONFIG:
    result_size = sizeof(cl_device_fp_config);
    result_data.devicefpconfig =
      CL_FP_ROUND_TO_NEAREST | CL_FP_INF_NAN | CL_FP_DENORM;
    break;
  case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
    result_size = sizeof(cl_device_mem_cache_type);
    result_data.devicememcachetype = CL_NONE;
    break;
  case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
    result_size = sizeof(cl_uint);
    result_data.cluint = 0;
    break;
  case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
    result_size = sizeof(cl_ulong);
    result_data.clulong = 0;
    break;
  case CL_DEVICE_GLOBAL_MEM_SIZE:
    result_size = sizeof(cl_ulong);
    result_data.clulong = device->globalMemSize;
    break;
  case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
    result_size = sizeof(cl_ulong);
    result_data.clulong = device->constantMemSize;
    break;
  case CL_DEVICE_MAX_CONSTANT_ARGS:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1024;
    break;
  case CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE:
    result_size = sizeof(size_t);
    result_data.sizet = 64 * 1024;
    break;
  case CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE:
    result_size = sizeof(size_t);
    result_data.sizet = device->globalMemSize;
    break;
  case CL_DEVICE_LOCAL_MEM_TYPE:
    result_size = sizeof(cl_device_local_mem_type);
    result_data.devicelocalmemtype = CL_LOCAL;
    break;
  case CL_DEVICE_LOCAL_MEM_SIZE:
    result_size = sizeof(cl_ulong);
    result_data.clulong = device->localMemSize;
    break;
  case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
    result_size = sizeof(cl_bool);
    result_data.clbool = CL_FALSE;
    break;
  case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
    result_size = sizeof(size_t);
    result_data.sizet = 1000;
    break;
  case CL_DEVICE_ENDIAN_LITTLE:
    result_size = sizeof(cl_bool);
#if IS_BIG_ENDIAN
    result_data.clbool = CL_FALSE;
#else
    result_data.clbool = CL_TRUE;
#endif
    break;
  case CL_DEVICE_AVAILABLE:
    result_size = sizeof(cl_bool);
    result_data.clbool = CL_TRUE;
    break;
  case CL_DEVICE_COMPILER_AVAILABLE:
    result_size = sizeof(cl_bool);
    result_data.clbool = CL_TRUE;
    break;
  case CL_DEVICE_EXECUTION_CAPABILITIES:
    result_size = sizeof(cl_device_exec_capabilities);
    result_data.cldevexeccap =  CL_EXEC_KERNEL | CL_EXEC_NATIVE_KERNEL;
    break;
  case CL_DEVICE_QUEUE_ON_HOST_PROPERTIES:
    result_size = sizeof(cl_command_queue_properties);
    result_data.clcmdqprop =
      CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE;
    break;
  case CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES:
    result_size = sizeof(cl_command_queue_properties);
    result_data.clcmdqprop =
      CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE;
    break;
  case CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE:
    result_size = sizeof(cl_uint);
    result_data.cluint = 16 * 1024;
    break;
  case CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE:
    result_size = sizeof(cl_uint);
    result_data.cluint = 256 * 1024;
    break;
  case CL_DEVICE_MAX_ON_DEVICE_QUEUES:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1;
    break;
  case CL_DEVICE_MAX_ON_DEVICE_EVENTS:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1024;
    break;
  case CL_DEVICE_NAME:
    result_size = sizeof(DEVICE_NAME);
    str = DEVICE_NAME;
    break;
  case CL_DEVICE_VENDOR:
    result_size = sizeof(DEVICE_VENDOR);
    str = DEVICE_VENDOR;
    break;
  case CL_DRIVER_VERSION:
    result_size = sizeof(DRIVER_VERSION);
    str = DRIVER_VERSION;
    break;
  case CL_DEVICE_PROFILE:
    result_size = sizeof(DEVICE_PROFILE);
    str = DEVICE_PROFILE;
    break;
  case CL_DEVICE_VERSION:
    result_size = sizeof(DEVICE_VERSION);
    str = DEVICE_VERSION;
    break;
  case CL_DEVICE_EXTENSIONS:
    result_size = sizeof(DEVICE_EXTENSIONS);
    str = DEVICE_EXTENSIONS;
    break;
  case CL_DEVICE_PLATFORM:
    result_size = sizeof(cl_platform_id);
    result_data.clplatid = m_platform;
    break;
  case CL_DEVICE_DOUBLE_FP_CONFIG:
    result_size = sizeof(cl_device_fp_config);
    result_data.devicefpconfig =
      CL_FP_FMA | CL_FP_ROUND_TO_NEAREST |
      CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_INF |
      CL_FP_INF_NAN | CL_FP_DENORM;
    break;
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
    result_size = sizeof(cl_uint);
    result_data.cluint = 0;
    break;
  case CL_DEVICE_HOST_UNIFIED_MEMORY:
    result_size = sizeof(cl_bool);
    result_data.clbool = CL_FALSE;
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1;
    break;
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
    result_size = sizeof(cl_uint);
    result_data.cluint = 0;
    break;
  case CL_DEVICE_OPENCL_C_VERSION:
    result_size = sizeof(DEVICE_LANG_VERSION);
    str = DEVICE_LANG_VERSION;
    break;
  case CL_DEVICE_LINKER_AVAILABLE:
    result_size = sizeof(cl_bool);
    result_data.clbool = CL_TRUE;
    break;
  case CL_DEVICE_BUILT_IN_KERNELS:
    result_size = 1;
    str = "";
    break;
  case CL_DEVICE_IMAGE_MAX_BUFFER_SIZE:
    result_size = sizeof(size_t);
    result_data.sizet = 65536;
    break;
  case CL_DEVICE_IMAGE_MAX_ARRAY_SIZE:
    result_size = sizeof(size_t);
    result_data.sizet = 2048;
    break;
  case CL_DEVICE_PARENT_DEVICE:
    result_size = sizeof(cl_device_id);
    result_data.cldeviceid = NULL;
    break;
  case CL_DEVICE_PARTITION_MAX_SUB_DEVICES:
    result_size = sizeof(cl_uint);
    result_data.cluint = 0;
    break;
  case CL_DEVICE_PARTITION_PROPERTIES:
  case CL_DEVICE_PARTITION_TYPE:
    result_size = sizeof(cl_device_partition_property);
    result_data.cldevpartprop = 0;
    break;
  case CL_DEVICE_PARTITION_AFFINITY_DOMAIN:
    result_size = sizeof(cl_device_affinity_domain);
    result_data.cldevaffdom = 0;
    break;
  case CL_DEVICE_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1;
    break;
  case CL_DEVICE_PREFERRED_INTEROP_USER_SYNC:
    result_size = sizeof(cl_bool);
    result_data.clbool = CL_TRUE;
    break;
  case CL_DEVICE_PRINTF_BUFFER_SIZE:
    result_size = sizeof(size_t);
    result_data.sizet = 1024;
    break;
  case CL_DEVICE_SVM_CAPABILITIES:
    result_size = sizeof(cl_device_svm_capabilities);
    result_data.svm = CL_DEVICE_SVM_COARSE_GRAIN_BUFFER;
    break;
  case CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT:
    result_size = sizeof(cl_uint);
    result_data.cluint = 0;
    break;
  case CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT:
    result_size = sizeof(cl_uint);
    result_data.cluint = 0;
    break;
  case CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT:
    result_size = sizeof(cl_uint);
    result_data.cluint = 0;
    break;
  case CL_DEVICE_SPIR_VERSIONS:
    result_size = sizeof(DEVICE_SPIR_VERSIONS);
    str = DEVICE_SPIR_VERSIONS;
    break;
  default:
    ReturnErrorArg(NULL, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(NULL, CL_INVALID_VALUE, ParamValueSizeTooSmall);
    }
    else
    {
      if (str)
        memcpy(param_value, str, result_size);
      else
        memcpy(param_value, &result_data, result_size);
    }
  }

  return CL_SUCCESS;
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
  ReturnErrorInfo(NULL, CL_INVALID_VALUE, "Not yet implemented");
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
  if (num_devices != 1)
  {
    SetErrorArg(NULL, CL_INVALID_VALUE, num_devices);
    return NULL;
  }
  if (!devices)
  {
    SetErrorArg(NULL, CL_INVALID_VALUE, devices);
    return NULL;
  }
  if (devices[0] != m_device)
  {
    SetError(NULL, CL_INVALID_DEVICE);
    return NULL;
  }
  if (!pfn_notify && user_data)
  {
    SetErrorInfo(NULL, CL_INVALID_VALUE,
                 "pfn_notify NULL but user_data non-NULL");
    return NULL;
  }

  // Create context object
  cl_context context = new _cl_context;
  context->dispatch = m_dispatchTable;
  context->context = new oclgrind::Context();
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

  SetError(NULL, CL_SUCCESS);
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
    SetErrorInfo(NULL, CL_INVALID_VALUE,
                 "pfn_notify NULL but user_data non-NULL");
    return NULL;
  }
  if (!(device_type & DEVICE_TYPE))
  {
    SetErrorArg(NULL, CL_DEVICE_NOT_FOUND, device_type);
    return NULL;
  }

  // Create context object
  cl_context context = new _cl_context;
  context->dispatch = m_dispatchTable;
  context->context = new oclgrind::Context();
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

  SetError(NULL, CL_SUCCESS);
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
    ReturnErrorArg(NULL, CL_INVALID_CONTEXT, context);
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
    ReturnErrorArg(NULL, CL_INVALID_CONTEXT, context);
  }

  if (--context->refCount == 0)
  {
    if (context->properties)
    {
      free(context->properties);
    }
    delete context->context;
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
  // Check context is valid
  if (!context)
  {
    ReturnErrorArg(NULL, CL_INVALID_CONTEXT, context);
  }

  size_t dummy = 0;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;

  union
  {
    cl_uint cluint;
    cl_device_id cldevid;
  } result_data;
  cl_context_properties* properties = NULL;

  switch (param_name)
  {
  case CL_CONTEXT_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data.cluint = context->refCount;
    break;
  case CL_CONTEXT_NUM_DEVICES:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1;
    break;
  case CL_CONTEXT_DEVICES:
    result_size = sizeof(cl_device_id);
    result_data.cldevid = m_device;
    break;
  case CL_CONTEXT_PROPERTIES:
    result_size = context->szProperties;
    properties = context->properties;
    break;
  default:
    ReturnErrorArg(context, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(context, CL_INVALID_VALUE, ParamValueSizeTooSmall);
    }
    else
    {
      if (properties)
        memcpy(param_value, properties, result_size);
      else
        memcpy(param_value, &result_data, result_size);
    }
  }

  return CL_SUCCESS;
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
    SetErrorArg(NULL, CL_INVALID_CONTEXT, context);
    return NULL;
  }
  if (device != m_device)
  {
    SetErrorArg(context, CL_INVALID_DEVICE, device);
    return NULL;
  }

  // Create command-queue object
  bool out_of_order = properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
  cl_command_queue queue;
  queue = new _cl_command_queue;
  queue->queue = new oclgrind::Queue(context->context, out_of_order);
  queue->dispatch = m_dispatchTable;
  queue->properties = properties;
  queue->context = context;
  queue->refCount = 1;

  clRetainContext(context);

  SetError(context, CL_SUCCESS);
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
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
  // Check queue is valid
  if (!command_queue)
  {
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }

  size_t dummy = 0;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;

  union
  {
    cl_uint cluint;
    cl_context context;
    cl_device_id cldevid;
    cl_command_queue_properties properties;
  } result_data;

  switch (param_name)
  {
  case CL_QUEUE_CONTEXT:
    result_size = sizeof(cl_context);
    result_data.context = command_queue->context;
    break;
  case CL_QUEUE_DEVICE:
    result_size = sizeof(cl_device_id);
    result_data.cldevid = m_device;
    break;
  case CL_QUEUE_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data.cluint = command_queue->refCount;
    break;
  case CL_QUEUE_PROPERTIES:
    result_size = sizeof(cl_command_queue_properties);
    result_data.properties = command_queue->properties;
    break;
  default:
    ReturnErrorArg(command_queue->context, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                      ParamValueSizeTooSmall);
    }
    else
    {
      memcpy(param_value, &result_data, result_size);
    }
  }

  return CL_SUCCESS;
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
    SetErrorArg(NULL, CL_INVALID_CONTEXT, context);
    return NULL;
  }
  if (size == 0)
  {
    SetErrorArg(context, CL_INVALID_BUFFER_SIZE, size);
    return NULL;
  }
  if ((host_ptr == NULL) ==
      ((flags & CL_MEM_COPY_HOST_PTR) ||
        flags & CL_MEM_USE_HOST_PTR))
  {
    SetErrorInfo(context, CL_INVALID_HOST_PTR,
                 "host_ptr NULL but CL_MEM_{COPY,USE}_HOST_PTR used");
    return NULL;
  }
  if ((flags & CL_MEM_USE_HOST_PTR) &&
      (flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_ALLOC_HOST_PTR)))
  {
    SetErrorInfo(context, CL_INVALID_VALUE,
                 "CL_MEM_USE_HOST_PTR cannot be used with "
                 "CL_MEM_{COPY,ALLOC}_HOST_PTR");
    return NULL;
  }

  // Create memory object
  oclgrind::Memory *globalMemory = context->context->getGlobalMemory();
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
    mem->address = globalMemory->createHostBuffer(size, host_ptr, flags);
    mem->hostPtr = host_ptr;
  }
  else
  {
    mem->address = globalMemory->allocateBuffer(size, flags);
    mem->hostPtr = NULL;
  }
  if (!mem->address)
  {
    SetError(context, CL_MEM_OBJECT_ALLOCATION_FAILURE);
    delete mem;
    return NULL;
  }
  clRetainContext(context);

  if (flags & CL_MEM_COPY_HOST_PTR)
  {
    context->context->getGlobalMemory()->store((const unsigned char*)host_ptr,
                                               mem->address, size);
  }

  SetError(context, CL_SUCCESS);
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
  if (!buffer)
  {
    SetErrorArg(NULL, CL_INVALID_MEM_OBJECT, buffer);
    return NULL;
  }
  if (buffer->parent)
  {
    SetErrorInfo(buffer->context, CL_INVALID_MEM_OBJECT,
                 "Parent buffer cannot be a sub-buffer");
    return NULL;
  }
  if (buffer_create_type != CL_BUFFER_CREATE_TYPE_REGION)
  {
    SetErrorArg(buffer->context, CL_INVALID_VALUE, buffer_create_type);
    return NULL;
  }
  if (!buffer_create_info)
  {
    SetErrorArg(buffer->context, CL_INVALID_VALUE, buffer_create_info);
    return NULL;
  }

  _cl_buffer_region region = *(_cl_buffer_region*)buffer_create_info;
  if (region.origin + region.size > buffer->size)
  {
    SetErrorInfo(buffer->context, CL_INVALID_VALUE,
                 "Region doesn't fit inside parent buffer");
    return NULL;
  }
  if (region.size == 0)
  {
    SetErrorInfo(buffer->context, CL_INVALID_VALUE, "Region size cannot be 0");
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

  SetError(buffer->context, CL_SUCCESS);
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
    SetErrorArg(NULL, CL_INVALID_CONTEXT, context);
    return NULL;
  }
  if (!image_format)
  {
    SetErrorArg(context, CL_INVALID_IMAGE_FORMAT_DESCRIPTOR, image_format);
    return NULL;
  }
  if (!image_desc)
  {
    SetErrorArg(context, CL_INVALID_IMAGE_DESCRIPTOR, image_desc);
    return NULL;
  }

  // Get size of each pixel (in bytes)
  size_t pixelSize = getPixelSize(image_format);
  if (!pixelSize)
  {
    SetErrorArg(context, CL_INVALID_VALUE, image_format);
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

  // Calculate total size of image
  size_t size = width * height * depth * arraySize * pixelSize;

  cl_mem mem;

  if (image_desc->image_type == CL_MEM_OBJECT_IMAGE1D_BUFFER)
  {
    // Use existing buffer
    if (!image_desc->buffer)
    {
      SetErrorInfo(context, CL_INVALID_VALUE,
                   "image_desc->buffer cannot be NULL "
                   "when using CL_MEM_OBJECT_IMAGE1D_BUFFER");
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

  SetError(context, CL_SUCCESS);
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
    {NULL}
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
    {NULL}
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
    ReturnErrorArg(NULL, CL_INVALID_MEM_OBJECT, memobj);
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
    ReturnErrorArg(NULL, CL_INVALID_MEM_OBJECT, memobj);
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
        memobj->context->context->getGlobalMemory()->deallocateBuffer(
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
    ReturnErrorArg(NULL, CL_INVALID_CONTEXT, context);
  }
  if (num_entries == 0 && image_formats)
  {
    ReturnErrorInfo(context, CL_INVALID_VALUE,
                    "num_entries should be >0 if image_formats non-NULL");
  }

  // TODO: Add support for packed image types

  // Channel orders
  const cl_channel_order ordersAll[] =
  {
    CL_R, CL_Rx, CL_A,
    CL_RG, CL_RGx, CL_RA,
    CL_RGBA,
  };
  const cl_channel_order ordersNormalized[] = {CL_INTENSITY, CL_LUMINANCE};
  const cl_channel_order ordersByte[] = {CL_ARGB, CL_BGRA};
  const cl_channel_order ordersPacked[] = {CL_RGB, CL_RGBx};
  const cl_channel_order *orders[] =
  {
    ordersAll, ordersNormalized, ordersByte //, ordersPacked
  };
  const size_t numOrders[] =
  {
    sizeof(ordersAll)        / sizeof(cl_channel_order),
    sizeof(ordersNormalized) / sizeof(cl_channel_order),
    sizeof(ordersByte)       / sizeof(cl_channel_order),
    sizeof(ordersPacked)     / sizeof(cl_channel_order),
  };

  // Channel types
  const cl_channel_type typesAll[] =
  {
    CL_SNORM_INT8, CL_SNORM_INT16,
    CL_UNORM_INT8, CL_UNORM_INT16,
    CL_SIGNED_INT8, CL_SIGNED_INT16, CL_SIGNED_INT32,
    CL_UNSIGNED_INT8, CL_UNSIGNED_INT16, CL_UNSIGNED_INT32,
    CL_FLOAT, CL_HALF_FLOAT,
  };
  const cl_channel_type typesNormalized[] =
  {
    CL_SNORM_INT8, CL_SNORM_INT16,
    CL_UNORM_INT8, CL_UNORM_INT16,
    CL_FLOAT, CL_HALF_FLOAT,
  };
  const cl_channel_type typesByte[] =
  {
    CL_SNORM_INT8, CL_UNORM_INT8,
    CL_SIGNED_INT8, CL_UNSIGNED_INT8,
  };
  const cl_channel_type typesPacked[] =
  {
    CL_UNORM_SHORT_565, CL_UNORM_SHORT_555, CL_UNORM_INT_101010
  };
  const cl_channel_type *types[] =
  {
    typesAll, typesNormalized, typesByte //, typesPacked,
  };
  const size_t numTypes[] =
  {
    sizeof(typesAll)        / sizeof(cl_channel_order),
    sizeof(typesNormalized) / sizeof(cl_channel_order),
    sizeof(typesByte)       / sizeof(cl_channel_order),
    sizeof(typesPacked)     / sizeof(cl_channel_order),
  };

  // Calculate total number of formats
  size_t numCatagories = sizeof(orders)/sizeof(cl_channel_order*);
  size_t numFormats = 0;
  for (size_t c = 0; c < numCatagories; c++)
  {
    numFormats += numOrders[c] * numTypes[c];
  }
  if (num_image_formats)
  {
    *num_image_formats = numFormats;
  }

  // Generate list of all valid order/type combinations
  if (image_formats)
  {
    unsigned i = 0;
    for (size_t c = 0; c < numCatagories; c++)
    {
      for (size_t o = 0; o < numOrders[c]; o++)
      {
        for (size_t t = 0; t < numTypes[c]; t++)
        {
          if (i >= num_entries)
          {
            return CL_SUCCESS;
          }

          cl_image_format format = {orders[c][o], types[c][t]};
          image_formats[i++] = format;
        }
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
  // Check mem object is valid
  if (!memobj)
  {
    ReturnErrorArg(NULL, CL_INVALID_MEM_OBJECT, memobj);
  }

  size_t dummy = 0;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;
  union
  {
    cl_mem_object_type clmemobjty;
    cl_mem_flags clmemflags;
    cl_context context;
    cl_mem clmem;
    size_t sizet;
    cl_uint cluint;
    void* ptr;
  } result_data;

  switch (param_name)
  {
  case CL_MEM_TYPE:
    result_size = sizeof(cl_mem_object_type);
    result_data.clmemobjty = memobj->isImage ?
      ((cl_image*)memobj)->desc.image_type : CL_MEM_OBJECT_BUFFER;
    break;
  case CL_MEM_FLAGS:
    result_size = sizeof(cl_mem_flags);
    result_data.clmemflags = memobj->flags;
    break;
  case CL_MEM_SIZE:
    result_size = sizeof(size_t);
    result_data.sizet = memobj->size;
    break;
  case CL_MEM_HOST_PTR:
    result_size = sizeof(void*);
    result_data.ptr = memobj->hostPtr;
    break;
  case CL_MEM_MAP_COUNT:
    result_size = sizeof(cl_uint);
    result_data.cluint = 0;
    break;
  case CL_MEM_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data.cluint = memobj->refCount;
    break;
  case CL_MEM_CONTEXT:
    result_size = sizeof(cl_context);
    result_data.context = memobj->context;
    break;
  case CL_MEM_ASSOCIATED_MEMOBJECT:
    result_size = sizeof(cl_mem);
    result_data.clmem = memobj->parent;
    break;
  case CL_MEM_OFFSET:
    result_size = sizeof(size_t);
    result_data.sizet = memobj->offset;
    break;
  default:
    ReturnErrorArg(memobj->context, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(memobj->context, CL_INVALID_VALUE,
                      ParamValueSizeTooSmall);
    }
    else
    {
      memcpy(param_value, &result_data, result_size);
    }
  }

  return CL_SUCCESS;
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
  // Check mem object is valid
  if (!image)
  {
    ReturnErrorArg(NULL, CL_INVALID_MEM_OBJECT, image);
  }
  cl_image *img = (cl_image*)image;

  size_t dummy = 0;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;
  union
  {
    cl_image_format climgfmt;
    size_t sizet;
    cl_mem clmem;
    cl_uint cluint;
  } result_data;

  switch (param_name)
  {
  case CL_IMAGE_FORMAT:
    result_size = sizeof(cl_image_format);
    result_data.climgfmt = img->format;
    break;
  case CL_IMAGE_ELEMENT_SIZE:
    result_size = sizeof(size_t);
    result_data.sizet = getPixelSize(&img->format);
    break;
  case CL_IMAGE_ROW_PITCH:
    result_size = sizeof(size_t);
    result_data.sizet = img->desc.image_row_pitch;
    break;
  case CL_IMAGE_SLICE_PITCH:
    result_size = sizeof(size_t);
    result_data.sizet = img->desc.image_slice_pitch;
    break;
  case CL_IMAGE_WIDTH:
    result_size = sizeof(size_t);
    result_data.sizet = img->desc.image_width;
    break;
  case CL_IMAGE_HEIGHT:
    result_size = sizeof(size_t);
    result_data.sizet =
      getNumDimensions(img->desc.image_type) > 1 ? img->desc.image_height : 0;
    break;
  case CL_IMAGE_DEPTH:
    result_size = sizeof(size_t);
    result_data.sizet =
      getNumDimensions(img->desc.image_type) > 2 ? img->desc.image_depth : 0;
    break;
  case CL_IMAGE_ARRAY_SIZE:
    result_size = sizeof(size_t);
    result_data.sizet =
      isImageArray(img->desc.image_type) ? img->desc.image_array_size : 0;
    break;
  case CL_IMAGE_BUFFER:
    result_size = sizeof(cl_mem);
    result_data.clmem = img->desc.buffer;
    break;
  case CL_IMAGE_NUM_MIP_LEVELS:
    result_size = sizeof(cl_uint);
    result_data.cluint = 0;
    break;
  case CL_IMAGE_NUM_SAMPLES:
    result_size = sizeof(cl_uint);
    result_data.cluint = 0;
    break;
  default:
    ReturnErrorArg(image->context, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(image->context, CL_INVALID_VALUE, ParamValueSizeTooSmall);
    }
    else
    {
      memcpy(param_value, &result_data, result_size);
    }
  }

  return CL_SUCCESS;
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
    ReturnErrorArg(NULL, CL_INVALID_MEM_OBJECT, memobj);
  }
  if (!pfn_notify)
  {
    ReturnErrorArg(memobj->context, CL_INVALID_VALUE, pfn_notify);
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
    SetErrorArg(NULL, CL_INVALID_CONTEXT, context);
    return NULL;
  }

  // Create sampler bitfield
  uint32_t bitfield = 0;

  if (normalized_coords)
  {
    bitfield |= CLK_NORMALIZED_COORDS_TRUE;
  }

  switch (addressing_mode)
  {
    case CL_ADDRESS_NONE:
      break;
    case CL_ADDRESS_CLAMP_TO_EDGE:
      bitfield |= CLK_ADDRESS_CLAMP_TO_EDGE;
      break;
    case CL_ADDRESS_CLAMP:
      bitfield |= CLK_ADDRESS_CLAMP;
      break;
    case CL_ADDRESS_REPEAT:
      bitfield |= CLK_ADDRESS_REPEAT;
      break;
    case CL_ADDRESS_MIRRORED_REPEAT:
      bitfield |= CLK_ADDRESS_MIRRORED_REPEAT;
      break;
    default:
      SetErrorArg(context, CL_INVALID_VALUE, addressing_mode);
      return NULL;
  }

  switch (filter_mode)
  {
    case CL_FILTER_NEAREST:
      bitfield |= CLK_FILTER_NEAREST;
      break;
    case CL_FILTER_LINEAR:
      bitfield |= CLK_FILTER_LINEAR;
      break;
    default:
      SetErrorArg(context, CL_INVALID_VALUE, filter_mode);
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
  sampler->refCount = 1;

  SetError(context, CL_SUCCESS);
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
    ReturnErrorArg(NULL, CL_INVALID_SAMPLER, sampler);
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
    ReturnErrorArg(NULL, CL_INVALID_SAMPLER, sampler);
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
  // Check sampler is valid
  if (!sampler)
  {
    ReturnErrorArg(NULL, CL_INVALID_SAMPLER, sampler);
  }

  size_t dummy = 0;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;
  union
  {
    cl_uint cluint;
    cl_context clcontext;
    cl_bool clbool;
    cl_addressing_mode claddrmode;
    cl_filter_mode clfiltmode;
  } result_data;

  switch (param_name)
  {
  case CL_SAMPLER_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data.cluint = sampler->refCount;
    break;
  case CL_SAMPLER_CONTEXT:
    result_size = sizeof(cl_context);
    result_data.clcontext = sampler->context;
    break;
  case CL_SAMPLER_NORMALIZED_COORDS:
    result_size = sizeof(cl_bool);
    result_data.clbool = sampler->normCoords;
    break;
  case CL_SAMPLER_ADDRESSING_MODE:
    result_size = sizeof(cl_addressing_mode);
    result_data.claddrmode = sampler->addressMode;
    break;
  case CL_SAMPLER_FILTER_MODE:
    result_size = sizeof(cl_filter_mode);
    result_data.clfiltmode = sampler->filterMode;
    break;
  default:
    ReturnErrorArg(sampler->context, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(sampler->context, CL_INVALID_VALUE,
                      ParamValueSizeTooSmall);
    }
    else
    {
      memcpy(param_value, &result_data, result_size);
    }
  }

  return CL_SUCCESS;
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
    SetErrorArg(NULL, CL_INVALID_CONTEXT, context);
    return NULL;
  }
  if (count == 0)
  {
    SetErrorArg(context, CL_INVALID_VALUE, count);
    return NULL;
  }
  if (!strings || !strings[0])
  {
    SetErrorArg(context, CL_INVALID_VALUE, strings);
    return NULL;
  }

  // Concatenate sources into a single string
  std::string source;
  for (unsigned i = 0; i < count; i++)
  {
    size_t length = (lengths && lengths[i]) ? lengths[i] : strlen(strings[i]);
    source.append(strings[i], length);
  }

  // Create program object
  cl_program prog = new _cl_program;
  prog->dispatch = m_dispatchTable;
  prog->program = new oclgrind::Program(context->context, source);
  prog->context = context;
  prog->refCount = 1;
  if (!prog->program)
  {
    SetError(context, CL_OUT_OF_HOST_MEMORY);
    delete prog;
    return NULL;
  }

  clRetainContext(context);

  SetError(context, CL_SUCCESS);
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
    SetErrorArg(NULL, CL_INVALID_CONTEXT, context);
    return NULL;
  }
  if (num_devices != 1 || !device_list)
  {
    SetErrorInfo(context, CL_INVALID_VALUE, "Invalid device list");
    return NULL;
  }
  if (!lengths)
  {
    SetErrorArg(context, CL_INVALID_VALUE, lengths);
    return NULL;
  }
  if (!binaries)
  {
    SetErrorArg(context, CL_INVALID_VALUE, binaries);
    return NULL;
  }
  if (device_list[0] != m_device)
  {
    SetErrorArg(context, CL_INVALID_DEVICE, device_list);
    return NULL;
  }

  // Create program object
  cl_program prog = new _cl_program;
  prog->dispatch = m_dispatchTable;
  prog->program = oclgrind::Program::createFromBitcode(context->context,
                                                       binaries[0], lengths[0]);
  prog->context = context;
  prog->refCount = 1;
  if (!prog->program)
  {
    SetError(context, CL_INVALID_BINARY);
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

  clRetainContext(context);

  SetError(context, CL_SUCCESS);
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
  if (!context)
  {
    SetError(NULL, CL_INVALID_CONTEXT);
    return NULL;
  }

  SetErrorInfo(context, CL_INVALID_VALUE, "No built-in kernels available");
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
    ReturnErrorArg(NULL, CL_INVALID_PROGRAM, program);
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
    ReturnErrorArg(NULL, CL_INVALID_PROGRAM, program);
  }

  if (--program->refCount == 0)
  {
    delete program->program;
    clReleaseContext(program->context);
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
    ReturnErrorArg(NULL, CL_INVALID_PROGRAM, program);
  }
  if (num_devices > 0 && !device_list)
  {
    ReturnErrorInfo(program->context, CL_INVALID_VALUE,
                    "num_devices >0 but device_list is NULL");
  }
  if (num_devices == 0 && device_list)
  {
    ReturnErrorInfo(program->context, CL_INVALID_VALUE,
                    "num_devices == 0 but device_list non-NULL");
  }
  if (!pfn_notify && user_data)
  {
    ReturnErrorInfo(program->context, CL_INVALID_VALUE,
                    "pfn_notify NULL but user_data non-NULL");
  }
  if (device_list && !device_list[0])
  {
    ReturnErrorArg(program->context, CL_INVALID_DEVICE, device);
  }

  // Build program
  if (!program->program->build(options))
  {
    ReturnError(program->context, CL_BUILD_PROGRAM_FAILURE);
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
    ReturnErrorArg(NULL, CL_INVALID_PROGRAM, program);
  }
  if (num_devices > 0 && !device_list)
  {
    ReturnErrorInfo(program->context, CL_INVALID_VALUE,
                    "num_devices >0 but device_list is NULL");
  }
  if (num_devices == 0 && device_list)
  {
    ReturnErrorInfo(program->context, CL_INVALID_VALUE,
                    "num_devices == 0 but device_list non-NULL");
  }
  if (!pfn_notify && user_data)
  {
    ReturnErrorInfo(program->context, CL_INVALID_VALUE,
                    "pfn_notify NULL but user_data non-NULL");
  }
  if (device_list && !device_list[0])
  {
    ReturnErrorArg(program->context, CL_INVALID_DEVICE, device);
  }

  // Prepare headers
  list<oclgrind::Program::Header> headers;
  for (unsigned i = 0; i < num_input_headers; i++)
  {
    headers.push_back(make_pair(header_include_names[i],
                                input_headers[i]->program));
  }

  // Build program
  if (!program->program->build(options, headers))
  {
    ReturnError(program->context, CL_BUILD_PROGRAM_FAILURE);
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
    SetErrorArg(NULL, CL_INVALID_CONTEXT, context);
    return NULL;
  }
  if (num_devices > 0 && !device_list)
  {
    SetErrorInfo(context, CL_INVALID_VALUE,
                 "num_devices >0 but device_list is NULL");
    return NULL;
  }
  if (num_devices == 0 && device_list)
  {
    SetErrorInfo(context, CL_INVALID_VALUE,
                 "num_devices == 0 but device_list non-NULL");
    return NULL;
  }
  if (!pfn_notify && user_data)
  {
    SetErrorInfo(context, CL_INVALID_VALUE,
                 "pfn_notify NULL but user_data non-NULL");
    return NULL;
  }
  if (device_list && !device_list[0])
  {
    SetErrorArg(context, CL_INVALID_DEVICE, device_list);
    return NULL;
  }

  // Prepare programs
  list<const oclgrind::Program*> programs;
  for (unsigned i = 0; i < num_input_programs; i++)
  {
    programs.push_back(input_programs[i]->program);
  }

  // Create program object
  cl_program prog = new _cl_program;
  prog->dispatch = m_dispatchTable;
  prog->program = oclgrind::Program::createFromPrograms(context->context,
                                                        programs);
  prog->context = context;
  prog->refCount = 1;
  if (!prog->program)
  {
    SetError(context, CL_INVALID_BINARY);
    delete prog;
    return NULL;
  }

  // Fire callback
  if (pfn_notify)
  {
    pfn_notify(prog, user_data);
  }

  clRetainContext(context);

  SetError(context, CL_SUCCESS);
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
  // Check program is valid
  if (!program)
  {
    ReturnErrorArg(NULL, CL_INVALID_PROGRAM, program);
  }
  if ((param_name == CL_PROGRAM_NUM_KERNELS ||
       param_name == CL_PROGRAM_KERNEL_NAMES) &&
      program->program->getBuildStatus() != CL_BUILD_SUCCESS)
  {
    ReturnErrorInfo(program->context, CL_INVALID_PROGRAM_EXECUTABLE,
                    "Program not successfully built");
  }

  size_t dummy;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;
  union
  {
    cl_uint cluint;
    cl_device_id device;
    cl_context context;
    size_t sizet;
  } result_data;
  const char* str = 0;
  string kernelNames;

  switch (param_name)
  {
  case CL_PROGRAM_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data.cluint = program->refCount;
    break;
  case CL_PROGRAM_CONTEXT:
    result_size = sizeof(cl_context);
    result_data.context = program->context;
    break;
  case CL_PROGRAM_NUM_DEVICES:
    result_size = sizeof(cl_uint);
    result_data.cluint = 1;
    break;
  case CL_PROGRAM_DEVICES:
    result_size = sizeof(cl_device_id);
    result_data.device = m_device;
    break;
  case CL_PROGRAM_SOURCE:
    str = program->program->getSource().c_str();
    result_size = strlen(str) + 1;
    break;
  case CL_PROGRAM_BINARY_SIZES:
    result_size = sizeof(size_t);
    result_data.sizet = program->program->getBinarySize();
    break;
  case CL_PROGRAM_BINARIES:
    result_size = sizeof(unsigned char*);
    break;
  case CL_PROGRAM_NUM_KERNELS:
    result_size = sizeof(size_t);
    result_data.sizet = program->program->getNumKernels();
    break;
  case CL_PROGRAM_KERNEL_NAMES:
  {
    list<string> names = program->program->getKernelNames();
    for (list<string>::iterator itr = names.begin(); itr != names.end(); itr++)
    {
      kernelNames += *itr;
      kernelNames += ";";
    }
    if (!kernelNames.empty())
    {
      kernelNames.erase(kernelNames.length()-1);
    }
    str = kernelNames.c_str();
    result_size = strlen(str) + 1;
    break;
  }
  default:
    ReturnErrorArg(program->context, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(NULL, CL_INVALID_VALUE, ParamValueSizeTooSmall);
    }
    else if (param_name == CL_PROGRAM_BINARIES)
    {
      program->program->getBinary(((unsigned char**)param_value)[0]);
    }
    else
    {
      if (str)
        memcpy(param_value, str, result_size);
      else
        memcpy(param_value, &result_data, result_size);
    }
  }

  return CL_SUCCESS;
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
  // Check program is valid
  if (!program)
  {
    ReturnErrorArg(NULL, CL_INVALID_PROGRAM, program);
  }

  size_t dummy;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;
  union
  {
    cl_build_status status;
    cl_program_binary_type type;
    size_t sizet;
  } result_data;
  const char* str = 0;

  switch (param_name)
  {
  case CL_PROGRAM_BUILD_STATUS:
    result_size = sizeof(cl_build_status);
    result_data.status = program->program->getBuildStatus();
    break;
  case CL_PROGRAM_BUILD_OPTIONS:
    str = program->program->getBuildOptions().c_str();
    result_size = strlen(str) + 1;
    break;
  case CL_PROGRAM_BUILD_LOG:
    str = program->program->getBuildLog().c_str();
    result_size = strlen(str) + 1;
    break;
  case CL_PROGRAM_BINARY_TYPE:
    result_size = sizeof(cl_program_binary_type);
    result_data.type = CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT;
    break;
  case CL_PROGRAM_BUILD_GLOBAL_VARIABLE_TOTAL_SIZE:
    result_size = sizeof(size_t);
    result_data.sizet = program->program->getTotalProgramScopeVarSize();
    break;
  default:
    ReturnErrorArg(program->context, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(program->context, CL_INVALID_VALUE,
                      ParamValueSizeTooSmall);
    }
    else
    {
      if (str)
        memcpy(param_value, str, result_size);
      else
        memcpy(param_value, &result_data, result_size);
    }
  }

  return CL_SUCCESS;
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
    SetError(NULL, CL_INVALID_PROGRAM);
    return NULL;
  }
  if (!kernel_name)
  {
    SetErrorArg(program->context, CL_INVALID_VALUE, kernel_name);
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
    SetErrorInfo(program->context, CL_INVALID_KERNEL_NAME,
                 "Kernel '" << kernel_name << "' not found");
    delete kernel;
    return NULL;
  }

  clRetainProgram(program);

  SetError(program->context, CL_SUCCESS);
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
    ReturnErrorArg(NULL, CL_INVALID_PROGRAM, program);
  }
  if (program->program->getBuildStatus() != CL_BUILD_SUCCESS)
  {
    ReturnErrorInfo(program->context, CL_INVALID_PROGRAM_EXECUTABLE,
                    "Program not built");
  }

  unsigned int num = program->program->getNumKernels();
  if (kernels && num_kernels < num)
  {
    ReturnErrorInfo(program->context, CL_INVALID_VALUE,
                    "num_kernels is " << num_kernels <<
                    ", but " << num << " kernels found");
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
    ReturnErrorArg(NULL, CL_INVALID_KERNEL, kernel);
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
    ReturnErrorArg(NULL, CL_INVALID_KERNEL, kernel);
  }

  if (--kernel->refCount == 0)
  {

    // Release memory allocated for image arguments
    while (!kernel->imageArgs.empty())
    {
      oclgrind::Image* img = kernel->imageArgs.top();
      kernel->imageArgs.pop();
      delete img;
    }

    delete kernel->kernel;

    clReleaseProgram(kernel->program);

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
  // Check parameters are valid
  if (!kernel)
  {
    ReturnErrorArg(NULL, CL_INVALID_KERNEL, kernel);
  }
  if (arg_index >= kernel->kernel->getNumArguments())
  {
    ReturnErrorInfo(kernel->program->context, CL_INVALID_ARG_INDEX,
                    "arg_index is " << arg_index <<
                    ", but kernel has " << kernel->kernel->getNumArguments()
                    << " arguments");
  }

  unsigned int addr = kernel->kernel->getArgumentAddressQualifier(arg_index);
  bool isSampler =
    kernel->kernel->getArgumentTypeName(arg_index) == "sampler_t";

  if (kernel->kernel->getArgumentSize(arg_index) != arg_size
      && !isSampler
      && addr != CL_KERNEL_ARG_ADDRESS_LOCAL)
  {
    ReturnErrorInfo(kernel->program->context, CL_INVALID_ARG_SIZE,
                    "arg_size is " << arg_size << ", but argument should be "
                    << kernel->kernel->getArgumentSize(arg_index) << " bytes");
  }

  // Prepare argument value
  oclgrind::TypedValue value;
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
    delete[] value.data;
    value.data = NULL;
    break;
  case CL_KERNEL_ARG_ADDRESS_GLOBAL:
  case CL_KERNEL_ARG_ADDRESS_CONSTANT:
    if (arg_value && *(cl_mem*)arg_value)
    {
      cl_mem mem = *(cl_mem*)arg_value;

      if (mem->isImage)
      {
        // Create Image struct
        oclgrind::Image *image = new oclgrind::Image;
        image->address = mem->address;
        image->format = ((cl_image*)mem)->format;
        image->desc = ((cl_image*)mem)->desc;
        *(oclgrind::Image**)value.data = image;
        // Keep a record of the image struct for releasing it later
        kernel->imageArgs.push(image);
      }
      else
      {
        memcpy(value.data, &mem->address, arg_size);
      }

      kernel->memArgs[arg_index] = mem;
    }
    else
    {
      value.setPointer(0);
      kernel->memArgs.erase(arg_index);
    }
    break;
  default:
    ReturnErrorInfo(kernel->program->context, CL_INVALID_ARG_VALUE,
                    "Unsupported address space");
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
  // Check kernel is valid
  if (!kernel)
  {
    ReturnErrorArg(NULL, CL_INVALID_KERNEL, kernel);
  }

  size_t dummy;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;
  union
  {
    cl_uint cluint;
    cl_context context;
    cl_program program;
  } result_data;
  const char* str = 0;

  switch (param_name)
  {
  case CL_KERNEL_FUNCTION_NAME:
    result_size = kernel->kernel->getName().size() + 1;
    str = kernel->kernel->getName().c_str();
    break;
  case CL_KERNEL_NUM_ARGS:
    result_size = sizeof(cl_uint);
    result_data.cluint = kernel->kernel->getNumArguments();
    break;
  case CL_KERNEL_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data.cluint = kernel->refCount;
    break;
  case CL_KERNEL_CONTEXT:
    result_size = sizeof(cl_context);
    result_data.context = kernel->program->context;
    break;
  case CL_KERNEL_PROGRAM:
    result_size = sizeof(cl_program);
    result_data.program = kernel->program;
    break;
  case CL_KERNEL_ATTRIBUTES:
    result_size = kernel->kernel->getAttributes().size() + 1;
    str = kernel->kernel->getAttributes().c_str();
    break;
  default:
    ReturnErrorArg(kernel->program->context, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(kernel->program->context, CL_INVALID_VALUE,
                      ParamValueSizeTooSmall);
    }
    else
    {
      if (str)
        memcpy(param_value, str, result_size);
      else
        memcpy(param_value, &result_data, result_size);
    }
  }

  return CL_SUCCESS;
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
  // Check parameters are valid
  if (!kernel)
  {
    ReturnErrorArg(NULL, CL_INVALID_KERNEL, kernel);
  }
  if (arg_indx >= kernel->kernel->getNumArguments())
  {
    ReturnErrorInfo(kernel->program->context, CL_INVALID_ARG_INDEX,
                    "arg_indx is " << arg_indx <<
                    ", but kernel has " << kernel->kernel->getNumArguments()
                    << " arguments");
  }

  size_t dummy = 0;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;
  union
  {
    cl_kernel_arg_address_qualifier addressQual;
    cl_kernel_arg_access_qualifier accessQual;
    cl_kernel_arg_type_qualifier typeQual;
  } result_data;

  std::string str_data;

  switch (param_name)
  {
  case CL_KERNEL_ARG_ADDRESS_QUALIFIER:
    result_size = sizeof(cl_kernel_arg_address_qualifier);
    result_data.addressQual =
      kernel->kernel->getArgumentAddressQualifier(arg_indx);
    break;
  case CL_KERNEL_ARG_ACCESS_QUALIFIER:
    result_size = sizeof(cl_kernel_arg_access_qualifier);
    result_data.accessQual =
      kernel->kernel->getArgumentAccessQualifier(arg_indx);
    break;
  case CL_KERNEL_ARG_TYPE_NAME:
    str_data = kernel->kernel->getArgumentTypeName(arg_indx).str();
    result_size = str_data.size() + 1;
    break;
  case CL_KERNEL_ARG_TYPE_QUALIFIER:
    result_size = sizeof(cl_kernel_arg_type_qualifier);
    result_data.typeQual = kernel->kernel->getArgumentTypeQualifier(arg_indx);
    break;
  case CL_KERNEL_ARG_NAME:
    str_data = kernel->kernel->getArgumentName(arg_indx).str();
    result_size = str_data.size() + 1;
    break;
  default:
    ReturnErrorArg(kernel->program->context, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(kernel->program->context, CL_INVALID_VALUE,
                      ParamValueSizeTooSmall);
    }

    if (str_data.size())
      memcpy(param_value, str_data.c_str(), result_size);
    else
      memcpy(param_value, &result_data, result_size);
  }

  return CL_SUCCESS;
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
  // Check parameters are valid
  if (!kernel)
  {
    ReturnErrorArg(NULL, CL_INVALID_KERNEL, kernel);
  }
  if (!device || device != m_device)
  {
    ReturnErrorArg(kernel->program->context, CL_INVALID_DEVICE, device);
  }

  size_t dummy;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;
  union
  {
    size_t sizet;
    size_t sizet3[3];
    cl_ulong clulong;
  } result_data;

  switch (param_name)
  {
  case CL_KERNEL_GLOBAL_WORK_SIZE:
    ReturnErrorInfo(kernel->program->context, CL_INVALID_VALUE,
                    "CL_KERNEL_GLOBAL_SIZE only valid on custom devices");
  case CL_KERNEL_WORK_GROUP_SIZE:
    result_size = sizeof(size_t);
    result_data.sizet = m_device->maxWGSize;
    break;
  case CL_KERNEL_COMPILE_WORK_GROUP_SIZE:
    result_size = sizeof(size_t[3]);
    kernel->kernel->getRequiredWorkGroupSize(result_data.sizet3);
    break;
  case CL_KERNEL_LOCAL_MEM_SIZE:
    result_size = sizeof(cl_ulong);
    result_data.clulong = kernel->kernel->getLocalMemorySize();
    break;
  case CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE:
    result_size = sizeof(size_t);
    result_data.sizet = 1;
    break;
  case CL_KERNEL_PRIVATE_MEM_SIZE:
    result_size = sizeof(cl_ulong);
    result_data.clulong = 0;
    break;
  default:
    ReturnErrorArg(kernel->program->context, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(kernel->program->context, CL_INVALID_VALUE,
                      ParamValueSizeTooSmall);
    }
    else
    {
      memcpy(param_value, &result_data, result_size);
    }
  }

  return CL_SUCCESS;
}

/* Event Object APIs  */

namespace
{
  // Utility to check if an event has completed (or terminated)
  inline bool isComplete(cl_event event)
  {
    return (event->event->state == CL_COMPLETE || event->event->state < 0);
  }
}

CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents
(
  cl_uint           num_events,
  const cl_event *  event_list
) CL_API_SUFFIX__VERSION_1_0
{
  // Check parameters
  if (!num_events)
  {
    ReturnErrorInfo(NULL, CL_INVALID_VALUE, "num_events cannot be 0");
  }
  if (!event_list)
  {
    ReturnErrorInfo(NULL, CL_INVALID_VALUE, "event_list cannot be NULL");
  }

  // Loop until all events complete
  bool complete = false;
  while (!complete)
  {
    complete = true;
    for (unsigned i = 0; i < num_events; i++)
    {
      // Skip event if already complete
      if (isComplete(event_list[i]))
      {
        continue;
      }

      // If it's not a user event, execute the associated command
      if (event_list[i]->queue)
      {
        oclgrind::Command *cmd = event_list[i]->event->command;
        event_list[i]->event->queue->execute(cmd, false);
        releaseCommand(cmd);

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
  for (unsigned i = 0; i < num_events; i++)
  {
    if (event_list[i]->event->state < 0)
    {
      ReturnErrorInfo(event_list[i]->context,
                      CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST,
                      "Event " << i <<
                      " terminated with error " << event_list[i]->event->state);
    }
  }

  return CL_SUCCESS;
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
  // Check event is valid
  if (!event)
  {
    ReturnErrorArg(NULL, CL_INVALID_EVENT, event);
  }

  size_t dummy;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;
  union
  {
    cl_command_queue queue;
    cl_context context;
    cl_command_type type;
    cl_int clint;
    cl_uint cluint;
    size_t sizet;
    size_t sizet3[3];
  } result_data;

  switch (param_name)
  {
  case CL_EVENT_COMMAND_QUEUE:
    result_size = sizeof(cl_command_queue);
    result_data.queue = event->queue;
    break;
  case CL_EVENT_CONTEXT:
    result_size = sizeof(cl_context);
    result_data.context = event->context;
    break;
  case CL_EVENT_COMMAND_TYPE:
    result_size = sizeof(cl_command_type);
    result_data.type = event->type;
    break;
  case CL_EVENT_COMMAND_EXECUTION_STATUS:
    result_size = sizeof(cl_int);
    result_data.clint = event->event->state;
    break;
  case CL_EVENT_REFERENCE_COUNT:
    result_size = sizeof(cl_uint);
    result_data.cluint = event->refCount;
    break;
  default:
    ReturnErrorArg(event->context, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(event->context, CL_INVALID_VALUE, ParamValueSizeTooSmall);
    }
    else
    {
      memcpy(param_value, &result_data, result_size);
    }
  }

  return CL_SUCCESS;
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
    SetErrorArg(NULL, CL_INVALID_CONTEXT, context);
    return NULL;
  }

  /// Create event object
  cl_event event = new _cl_event;
  event->dispatch = m_dispatchTable;
  event->context = context;
  event->queue = 0;
  event->type = CL_COMMAND_USER;
  event->event = new oclgrind::Event();
  event->event->state = CL_SUBMITTED;
  event->event->command = NULL;
  event->event->queue = NULL;
  event->refCount = 1;

  SetError(context, CL_SUCCESS);
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
    ReturnErrorArg(NULL, CL_INVALID_EVENT, event);
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
    ReturnErrorArg(NULL, CL_INVALID_EVENT, event);
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
  if (!event)
  {
    ReturnErrorArg(NULL, CL_INVALID_EVENT, event);
  }
  if (event->queue)
  {
    ReturnErrorInfo(event->context, CL_INVALID_EVENT, "Not a user event");
  }
  if (execution_status != CL_COMPLETE && execution_status >= 0)
  {
    ReturnErrorArg(event->context, CL_INVALID_VALUE, execution_status);
  }
  if (event->event->state == CL_COMPLETE || event->event->state < 0)
  {
    ReturnErrorInfo(event->context, CL_INVALID_OPERATION,
                    "Event status already set");
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
    ReturnErrorArg(NULL, CL_INVALID_EVENT, event);
  }
  if (!pfn_notify)
  {
    ReturnErrorArg(event->context, CL_INVALID_VALUE, pfn_notify);
  }
  if (command_exec_callback_type != CL_COMPLETE &&
      command_exec_callback_type != CL_SUBMITTED &&
      command_exec_callback_type != CL_RUNNING)
  {
    ReturnErrorArg(event->context, CL_INVALID_VALUE,
                   command_exec_callback_type);
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
  // Check event is valid
  if (!event)
  {
    ReturnErrorArg(NULL, CL_INVALID_EVENT, event);
  }
  if (!event->queue)
  {
    ReturnError(event->context, CL_PROFILING_INFO_NOT_AVAILABLE);
  }

  size_t dummy = 0;
  size_t& result_size = param_value_size_ret ? *param_value_size_ret : dummy;
  cl_ulong result;

  switch (param_name)
  {
  case CL_PROFILING_COMMAND_QUEUED:
    result_size = sizeof(cl_ulong);
    result = event->event->queueTime;
    break;
  case CL_PROFILING_COMMAND_SUBMIT:
    result_size = sizeof(cl_ulong);
    result = event->event->startTime;
    break;
  case CL_PROFILING_COMMAND_START:
    result_size = sizeof(cl_ulong);
    result = event->event->startTime;
    break;
  case CL_PROFILING_COMMAND_END:
    result_size = sizeof(cl_ulong);
    result = event->event->endTime;
    break;
  default:
    ReturnErrorArg(event->context, CL_INVALID_VALUE, param_name);
  }

  if (param_value)
  {
    // Check destination is large enough
    if (param_value_size < result_size)
    {
      ReturnErrorInfo(event->context, CL_INVALID_VALUE, ParamValueSizeTooSmall);
    }
    else
    {
      *(cl_ulong*)param_value = result;
    }
  }

  return CL_SUCCESS;
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }

  // TODO: Move this finish to async thread?
  oclgrind::Command *cmd = command_queue->queue->finish();
  releaseCommand(cmd);

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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!buffer)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, memobj);
  }
  if (!ptr)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_VALUE, ptr);
  }
  if (offset + cb > buffer->size)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "offset + cb (" << offset << " + " << cb <<
                    ") exceeds buffer size (" << buffer->size << " bytes)");
  }
  if (buffer->flags & (CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_WRITE_ONLY))
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                    "Buffer flags specify host will not read data");
  }

  // Enqueue command
  oclgrind::BufferCommand *cmd =
    new oclgrind::BufferCommand(oclgrind::Command::READ);
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!buffer)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, memobj);
  }
  if (!ptr)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_VALUE, ptr);
  }
  if (buffer->flags & (CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_WRITE_ONLY))
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                    "Buffer flags specify host will not read data");
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
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "Region exceeds buffer size (" <<
                    buffer->size << " bytes)");
  }

  // Enqueue command
  oclgrind::BufferRectCommand *cmd =
    new oclgrind::BufferRectCommand(oclgrind::Command::READ_RECT);
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
  asyncEnqueue(command_queue, CL_COMMAND_READ_BUFFER_RECT, cmd,
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!buffer)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, memobj);
  }
  if (!ptr)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_VALUE, ptr);
  }
  if (offset + cb > buffer->size)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "offset + cb (" << offset << " + " << cb <<
                    ") exceeds buffer size (" << buffer->size << " bytes)");
  }
  if (buffer->flags & (CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_READ_ONLY))
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                    "Buffer flags specify host will not write data");
  }

  // Enqueue command
  oclgrind::BufferCommand *cmd =
    new oclgrind::BufferCommand(oclgrind::Command::WRITE);
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!buffer)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, memobj);
  }
  if (!ptr)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_VALUE, ptr);
  }
  if (buffer->flags & (CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_READ_ONLY))
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                    "Buffer flags specify host will not write data");
  }

  // Compute pitches if necessary
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
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "Region exceeds buffer size (" <<
                    buffer->size << " bytes)");
  }

  // Enqueue command
  oclgrind::BufferRectCommand *cmd =
    new oclgrind::BufferRectCommand(oclgrind::Command::WRITE_RECT);
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
  asyncEnqueue(command_queue, CL_COMMAND_WRITE_BUFFER_RECT, cmd,
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!src_buffer)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, src_buffer);
  }
  if (!dst_buffer)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, dst_buffer);
  }
  if (dst_offset + cb > dst_buffer->size)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "dst_offset + cb (" << dst_offset << " + " << cb <<
                    ") exceeds buffer size (" << dst_buffer->size << " bytes)");
  }
  if (src_offset + cb > src_buffer->size)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "src_offset + cb (" << src_offset << " + " << cb <<
                    ") exceeds buffer size (" << src_buffer->size << " bytes)");
  }
  // If src and dst buffers are the same and if src_offset comes before
  // dst_offset and src buffer size goes beyond dst_offset then there is an
  // overlap
  if ((src_buffer == dst_buffer) &&
      (src_offset <= dst_offset) && ((src_offset + cb) > dst_offset))
  {
    ReturnErrorInfo(command_queue->context, CL_MEM_COPY_OVERLAP,
                    "src_buffer == dst_buffer and "
                    "src_offset + cb (" << src_offset << " + " << cb <<
                    ") overlaps dst_offset (" << dst_offset << ")");
  }
  // If src and dst buffers are the same and if dst_offset comes before
  // src_offset and dst buffer size goes beyond src_offset then there is an
  // overlap
  if ((src_buffer == dst_buffer) &&
      (dst_offset <= src_offset) && ((dst_offset + cb) > src_offset))
  {
    ReturnErrorInfo(command_queue->context, CL_MEM_COPY_OVERLAP,
                    "src_buffer == dst_buffer and "
                    "dst_offset + cb (" << dst_offset << " + " << cb <<
                    ") overlaps src_offset (" << src_offset << ")");
  }

  // Enqueue command
  oclgrind::CopyCommand *cmd = new oclgrind::CopyCommand();
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!src_buffer)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, src_buffer);
  }
  if (!dst_buffer)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, dst_buffer);
  }
  if (!region || region[0] == 0 || region[1] == 0 || region[2] == 0)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_VALUE, region);
  }

  // Compute pitches if necessary
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
  if (src_end > src_buffer->size)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "Region exceeds source buffer size (" <<
                    src_buffer->size << " bytes)");
  }
  if (dst_end > dst_buffer->size)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "Region exceeds destination buffer size (" <<
                    dst_buffer->size << " bytes)");
  }

  // Enqueue command
  oclgrind::CopyRectCommand *cmd = new oclgrind::CopyRectCommand();
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
  asyncEnqueue(command_queue, CL_COMMAND_COPY_BUFFER_RECT, cmd,
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!buffer)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, buffer);
  }
  if (offset + cb > buffer->size)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "offset + cb (" << offset << " + " << cb <<
                    ") exceeds buffer size (" << buffer->size << " bytes)");
  }
  if (!pattern)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_VALUE, pattern);
  }
  if (pattern_size == 0)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_VALUE, pattern_size);
  }
  if (offset%pattern_size)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "offset (" << offset << ")" <<
                    " not a multiple of pattern_size (" << pattern_size << ")");
  }
  if (cb%pattern_size)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "cb (" << cb << ")" <<
                    " not a multiple of pattern_size (" << pattern_size << ")");
  }

  // Enqueue command
  oclgrind::FillBufferCommand *cmd =
    new oclgrind::FillBufferCommand((const unsigned char*)pattern,
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!image)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, image);
  }
  if (!fill_color)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_VALUE, fill_color);
  }
  if (!region[0] || !region[1] || !region[2])
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "Values in region cannot be 0");
  }

  // Get image dimensions
  cl_image *img = (cl_image*)image;
  size_t width = img->desc.image_width;
  size_t height = img->desc.image_height;
  size_t depth = img->desc.image_depth;
  size_t arraySize = img->desc.image_array_size;
  size_t pixelSize = getPixelSize(&img->format);
  size_t row_pitch = width * pixelSize;
  size_t slice_pitch = height * row_pitch;

  if (img->desc.image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY)
    height = arraySize;
  if (img->desc.image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY)
    depth = arraySize;

  // Ensure region is within image bounds
  if (origin[0] + region[0] > width)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "origin[0] + region[0] > width ("
                    << origin[0] << " + " << region[0] << " > " << width
                    << " )");
  }
  if (origin[1] + region[1] > height)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "origin[1] + region[1] > height ("
                    << origin[1] << " + " << region[1] << " > " << height
                    << " )");
  }
  if (origin[2] + region[2] > depth)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "origin[2] + region[2] > depth ("
                    << origin[2] << " + " << region[2] << " > " << depth
                    << " )");
  }

  // Generate color data with correct order and data type
  unsigned char *color = new unsigned char[pixelSize];
  for (unsigned output = 0; output < getNumChannels(&img->format); output++)
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
        ReturnError(command_queue->context, CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
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
    case CL_HALF_FLOAT:
      ((uint16_t*)color)[output] =
        oclgrind::floatToHalf(((float*)fill_color)[input]);
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
      ReturnError(command_queue->context, CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
    }
  }

  // Enqueue command
  oclgrind::FillImageCommand *cmd =
    new oclgrind::FillImageCommand(image->address, origin, region,
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!image)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, image);
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

  // Enqueue read
  cl_int ret = clEnqueueReadBufferRect(
    command_queue, image, blocking_read,
    buffer_origin, host_origin, pixel_region,
    img_row_pitch, img_slice_pitch, row_pitch, slice_pitch,
    ptr, num_events_in_wait_list, event_wait_list, event);
  if (event && ret == CL_SUCCESS)
  {
    (*event)->type = CL_COMMAND_READ_IMAGE;
  }
  return ret;
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!image)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, image);
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

  // Enqueue write
  cl_int ret = clEnqueueWriteBufferRect(
    command_queue, image, blocking_write,
    buffer_origin, host_origin, pixel_region,
    img_row_pitch, img_slice_pitch, input_row_pitch, input_slice_pitch,
    ptr, num_events_in_wait_list, event_wait_list, event);
  if (event && ret == CL_SUCCESS)
  {
    (*event)->type = CL_COMMAND_WRITE_IMAGE;
  }
  return ret;
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!src_image)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, src_image);
  }
  if (!dst_image)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, dst_image);
  }

  cl_image *src = (cl_image*)src_image;
  cl_image *dst = (cl_image*)dst_image;
  if (src->format.image_channel_order != dst->format.image_channel_order)
  {
    ReturnErrorInfo(command_queue->context, CL_IMAGE_FORMAT_MISMATCH,
                    "Channel orders do not match");
  }
  if (src->format.image_channel_data_type != dst->format.image_channel_data_type)
  {
    ReturnErrorInfo(command_queue->context, CL_IMAGE_FORMAT_MISMATCH,
                    "Channel data types do no match");
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

  // Enqueue copy
  cl_int ret = clEnqueueCopyBufferRect(
    command_queue, src_image, dst_image,
    src_pixel_origin, dst_pixel_origin, pixel_region,
    src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch,
    num_events_in_wait_list, event_wait_list, event);
  if (event && ret == CL_SUCCESS)
  {
    (*event)->type = CL_COMMAND_COPY_IMAGE;
  }
  return ret;
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!src_image)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, src_image);
  }
  if (!dst_buffer)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, dst_buffer);
  }

  cl_image *src = (cl_image*)src_image;
  size_t pixel_size = getPixelSize(&src->format);
  size_t src_pixel_origin[3] = {src_origin[0]*pixel_size,
                                src_origin[1], src_origin[2]};
  size_t src_row_pitch = src->desc.image_width * pixel_size;
  size_t src_slice_pitch = src->desc.image_height * src_row_pitch;

  size_t pixel_region[3] = {region[0]*pixel_size, region[1], region[2]};
  size_t dst_origin[3] = {dst_offset, 0, 0};

  // Enqueue copy
  cl_int ret = clEnqueueCopyBufferRect(
    command_queue, src_image, dst_buffer,
    src_pixel_origin, dst_origin, pixel_region,
    src_row_pitch, src_slice_pitch, 0, 0,
    num_events_in_wait_list, event_wait_list, event);
  if (event && ret == CL_SUCCESS)
  {
    (*event)->type = CL_COMMAND_COPY_IMAGE_TO_BUFFER;
  }
  return ret;
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!src_buffer)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, src_buffer);
  }
  if (!dst_image)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, dst_image);
  }

  cl_image *dst = (cl_image*)dst_image;
  size_t pixel_size = getPixelSize(&dst->format);
  size_t dst_pixel_origin[3] = {dst_origin[0]*pixel_size,
                                dst_origin[1], dst_origin[2]};
  size_t dst_row_pitch = dst->desc.image_width * pixel_size;
  size_t dst_slice_pitch = dst->desc.image_height * dst_row_pitch;

  size_t pixel_region[3] = {region[0]*pixel_size, region[1], region[2]};
  size_t src_origin[3] = {src_offset, 0, 0};

  // Enqueue copy
  cl_int ret = clEnqueueCopyBufferRect(
    command_queue, src_buffer, dst_image,
    src_origin, dst_pixel_origin, pixel_region,
    0, 0, dst_row_pitch, dst_slice_pitch,
    num_events_in_wait_list, event_wait_list, event);
  if (event && ret == CL_SUCCESS)
  {
    (*event)->type = CL_COMMAND_COPY_BUFFER_TO_IMAGE;
  }
  return ret;
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
    SetErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
    return NULL;
  }
  if (!buffer)
  {
    SetErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, buffer);
    return NULL;
  }
  if (map_flags & CL_MAP_WRITE &&
      buffer->flags & (CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_READ_ONLY))
  {
    SetErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                 "Buffer flags specify host will not write data");
    return NULL;
  }
  if (map_flags & CL_MAP_READ &&
      buffer->flags & (CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_WRITE_ONLY))
  {
    SetErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                 "Buffer flags specify host will not read data");
    return NULL;
  }

  // Check map region
  if (offset + cb > buffer->size)
  {
    SetErrorInfo(command_queue->context, CL_INVALID_VALUE,
                 "offset + cb (" << offset << " + " << cb <<
                 ") exceeds buffer size (" << buffer->size << " bytes)");
    return NULL;
  }

  // Map buffer
  void *ptr = buffer->context->context->getGlobalMemory()->mapBuffer(
    buffer->address, offset, cb);
  if (ptr == NULL)
  {
    SetError(command_queue->context, CL_INVALID_VALUE);
    return NULL;
  }

  // Enqueue command
  oclgrind::MapCommand *cmd = new oclgrind::MapCommand();
  cmd->address = buffer->address;
  cmd->offset  = offset;
  cmd->size    = cb;
  cmd->flags   = map_flags;
  asyncQueueRetain(cmd, buffer);
  asyncEnqueue(command_queue, CL_COMMAND_MAP_BUFFER, cmd,
               num_events_in_wait_list, event_wait_list, event);

  SetError(command_queue->context, CL_SUCCESS);
  if (blocking_map)
  {
    SetError(command_queue->context, clFinish(command_queue));
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
    SetErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
    return NULL;
  }
  if (!image)
  {
    SetErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, image);
    return NULL;
  }
  if (!image_row_pitch)
  {
    SetErrorArg(command_queue->context, CL_INVALID_VALUE, image_row_pitch);
    return NULL;
  }
  if (map_flags & CL_MAP_WRITE &&
      image->flags & (CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_READ_ONLY))
  {
    SetErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                 "Image flags specify host will not write data");
    return NULL;
  }
  if (map_flags & CL_MAP_READ &&
      image->flags & (CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_WRITE_ONLY))
  {
    SetErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                 "Image flags specify host will not read data");
    return NULL;
  }
  if (!region[0] || !region[1] || !region[2])
  {
    SetErrorInfo(command_queue->context, CL_INVALID_VALUE,
                 "Values in region cannot be 0");
  }

  // Get image dimensions
  cl_image *img = (cl_image*)image;
  size_t width = img->desc.image_width;
  size_t height = img->desc.image_height;
  size_t depth = img->desc.image_depth;
  size_t arraySize = img->desc.image_array_size;
  size_t pixelSize = getPixelSize(&img->format);
  size_t row_pitch = width * pixelSize;
  size_t slice_pitch = height * row_pitch;

  if (img->desc.image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY)
    height = arraySize;
  if (img->desc.image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY)
    depth = arraySize;

  // Ensure region is within image bounds
  if (origin[0] + region[0] > width)
  {
    SetErrorInfo(command_queue->context, CL_INVALID_VALUE,
                 "origin[0] + region[0] > width ("
                 << origin[0] << " + " << region[0] << " > " << width
                 << " )");
  }
  if (origin[1] + region[1] > height)
  {
    SetErrorInfo(command_queue->context, CL_INVALID_VALUE,
                 "origin[1] + region[1] > height ("
                 << origin[1] << " + " << region[1] << " > " << height
                 << " )");
  }
  if (origin[2] + region[2] > depth)
  {
    SetErrorInfo(command_queue->context, CL_INVALID_VALUE,
                 "origin[2] + region[2] > depth ("
                 << origin[2] << " + " << region[2] << " > " << depth
                 << " )");
  }

  // Compute byte offset and size
  size_t offset = origin[0] * pixelSize
                + origin[1] * row_pitch
                + origin[2] * slice_pitch;
  size_t size = region[0] * pixelSize
              + (region[1]-1) * row_pitch
              + (region[2]-1) * slice_pitch;

  // Map image
  void *ptr = image->context->context->getGlobalMemory()->mapBuffer(
        image->address, offset, size);
  if (ptr == NULL)
  {
    SetError(command_queue->context, CL_INVALID_VALUE);
    return NULL;
  }

  *image_row_pitch = row_pitch;
  if (image_slice_pitch)
  {
    *image_slice_pitch = slice_pitch;
  }

  // Enqueue command
  oclgrind::MapCommand *cmd = new oclgrind::MapCommand();
  cmd->address = image->address;
  cmd->offset  = offset;
  cmd->size    = size;
  cmd->flags   = map_flags;
  asyncQueueRetain(cmd, image);
  asyncEnqueue(command_queue, CL_COMMAND_MAP_IMAGE, cmd,
               num_events_in_wait_list, event_wait_list, event);

  SetError(command_queue->context, CL_SUCCESS);
  if (blocking_map)
  {
    SetError(command_queue->context, clFinish(command_queue));
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!memobj)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_MEM_OBJECT, memobj);
  }
  if (!mapped_ptr)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_VALUE, mapped_ptr);
  }

  // Enqueue command
  oclgrind::UnmapCommand *cmd = new oclgrind::UnmapCommand();
  cmd->address = memobj->address;
  cmd->ptr     = mapped_ptr;
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }

  // Enqueue command
  oclgrind::Command *cmd = new oclgrind::Command();
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (work_dim < 1 || work_dim > 3)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_WORK_DIMENSION,
                    "Kernels must be 1, 2 or 3 dimensional (work_dim = "
                    << work_dim << ")");
  }
  if (!global_work_size)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_GLOBAL_WORK_SIZE,
                    "global_work_size cannot be NULL");
  }

  // Check global and local sizes are valid
  size_t reqdWorkGroupSize[3];
  size_t totalWGSize = 1;
  kernel->kernel->getRequiredWorkGroupSize(reqdWorkGroupSize);
  for (unsigned i = 0; i < work_dim; i++)
  {
    if (!global_work_size[i])
    {
      ReturnErrorInfo(command_queue->context, CL_INVALID_GLOBAL_WORK_SIZE,
                      "global_work_size[" << i << "] = 0");
    }
    if (kernel->kernel->requiresUniformWorkGroups() &&
        local_work_size && global_work_size[i] % local_work_size[i])
    {
      ReturnErrorInfo(command_queue->context, CL_INVALID_WORK_GROUP_SIZE,
                      "local_work_size[" << i << "]=" << local_work_size[i] <<
                      " does not divide global_work_size[" << i << "]=" <<
                      global_work_size[i]);
    }
    if (local_work_size)
    {
      if (local_work_size[i] > m_device->maxWGSize)
      {
        ReturnErrorInfo(command_queue->context, CL_INVALID_WORK_ITEM_SIZE,
                        "local_work_size[" << i << "]=" << local_work_size[i] <<
                        " exceeds device maximum of " << m_device->maxWGSize);
      }
      totalWGSize *= local_work_size[i];
    }
    if (local_work_size && reqdWorkGroupSize[i] &&
        local_work_size[i] != reqdWorkGroupSize[i])
    {
      ReturnErrorInfo(command_queue->context, CL_INVALID_WORK_GROUP_SIZE,
                      "local_work_size[" << i << "]=" << local_work_size[i] <<
                      " does not match reqd_work_group_size[" << i << "]=" <<
                      reqdWorkGroupSize[i])
    }
  }
  if (totalWGSize > m_device->maxWGSize)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_WORK_GROUP_SIZE,
                    "total work-group size (" << totalWGSize << ")"
                    " exceeds device maximum of " << m_device->maxWGSize);
  }

  // Ensure all arguments have been set
  if (!kernel->kernel->allArgumentsSet())
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_KERNEL_ARGS,
                    "Not all kernel arguments set");
  }

  // Check that local memory requirement is within device maximum
  size_t totalLocal = kernel->kernel->getLocalMemorySize();
  if (totalLocal > m_device->localMemSize)
  {
    ReturnErrorInfo(command_queue->context, CL_OUT_OF_RESOURCES,
                    "total local memory size (" << totalLocal << ")"
                    " exceeds device maximum of " << m_device->localMemSize);
  }

  // Check that constant memory requirement is within device maximum
  size_t totalConstant = 0;
  std::map<cl_uint,cl_mem>::iterator arg;
  for (arg = kernel->memArgs.begin(); arg != kernel->memArgs.end(); arg++)
  {
    if (kernel->kernel->getArgumentAddressQualifier(arg->first) ==
        CL_KERNEL_ARG_ADDRESS_CONSTANT)
      totalConstant += arg->second->size;
  }
  if (totalConstant > m_device->constantMemSize)
  {
    ReturnErrorInfo(command_queue->context, CL_OUT_OF_RESOURCES,
                    "total constant memory size (" << totalConstant << ")"
                    " exceeds device maximum of " << m_device->constantMemSize);
  }

  // Set-up offsets and sizes
  oclgrind::KernelCommand *cmd = new oclgrind::KernelCommand();
  cmd->kernel = new oclgrind::Kernel(*kernel->kernel);
  cmd->work_dim = work_dim;
  cmd->globalSize   = oclgrind::Size3(1, 1, 1);
  cmd->globalOffset = oclgrind::Size3(0, 0, 0);
  cmd->localSize    = oclgrind::Size3(1, 1, 1);
  memcpy(&cmd->globalSize, global_work_size, work_dim*sizeof(size_t));
  if (global_work_offset)
  {
    memcpy(&cmd->globalOffset, global_work_offset, work_dim*sizeof(size_t));
  }
  if (local_work_size)
  {
    memcpy(&cmd->localSize, local_work_size, work_dim*sizeof(size_t));
  }

  // Enqueue command
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }
  if (!user_func)
  {
    ReturnErrorArg(command_queue->context, CL_INVALID_VALUE, user_func);
  }
  if (!args && (cb_args > 0 || num_mem_objects > 0))
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                   "args is NULL but cb_args|num_mem_objects >0");
  }
  if (args && cb_args == 0)
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "args is non-NULL but cb_args is 0");
  }
  if (num_mem_objects > 0 && (!mem_list || !args_mem_loc))
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "num_mem_objects >0 but mem_list|args_mem_loc is NULL");
  }
  if (num_mem_objects == 0 && (mem_list || args_mem_loc))
  {
    ReturnErrorInfo(command_queue->context, CL_INVALID_VALUE,
                    "num_mem_objects is 0 but mem_list|args_mem_loc not NULL");
  }

  // Replace mem objects with real pointers
  oclgrind::Memory *memory = command_queue->context->context->getGlobalMemory();
  for (unsigned i = 0; i < num_mem_objects; i++)
  {
    if (!mem_list[i])
    {
      ReturnErrorInfo(command_queue->context, CL_INVALID_MEM_OBJECT,
                      "Memory object " << i << " is NULL");
    }

    void *addr = memory->getPointer(mem_list[i]->address);
    if (addr == NULL)
    {
      ReturnErrorInfo(command_queue->context, CL_INVALID_MEM_OBJECT,
                      "Memory object " << i << " not valid");
    }
    memcpy((void*)args_mem_loc[i], &addr, sizeof(void*));
  }

  // Create command
  oclgrind::NativeKernelCommand *cmd =
    new oclgrind::NativeKernelCommand(user_func, args, cb_args);

  // Retain memory objects
  for (unsigned i = 0; i < num_mem_objects; i++)
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }

  // Enqueue command
  oclgrind::Command *cmd = new oclgrind::Command();
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }

  // Enqueue command
  oclgrind::Command *cmd = new oclgrind::Command();
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
  ReturnError(NULL, CL_INVALID_OPERATION);
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
    ReturnErrorArg(NULL, CL_INVALID_COMMAND_QUEUE, command_queue);
  }

  // Enqueue command
  oclgrind::Command *cmd = new oclgrind::Command();
  asyncEnqueue(command_queue, CL_COMMAND_BARRIER, cmd,
               num_events, event_list, NULL);

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
  SetErrorInfo(NULL, CL_INVALID_CONTEXT, "CL/GL interop not implemented");
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
  SetErrorInfo(NULL, CL_INVALID_CONTEXT, "CL/GL interop not implemented");
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
  SetErrorInfo(NULL, CL_INVALID_CONTEXT, "CL/GL interop not implemented");
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
  SetErrorInfo(NULL, CL_INVALID_CONTEXT, "CL/GL interop not implemented");
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
  SetErrorInfo(NULL, CL_INVALID_CONTEXT, "CL/GL interop not implemented");
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
  ReturnErrorInfo(NULL, CL_INVALID_MEM_OBJECT, "CL/GL interop not implements");
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
  ReturnErrorInfo(NULL, CL_INVALID_MEM_OBJECT, "CL/GL interop not implemented");
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
  ReturnErrorInfo(NULL, CL_INVALID_CONTEXT, "CL/GL interop not implemented");
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
  ReturnErrorInfo(NULL, CL_INVALID_CONTEXT, "CL/GL interop not implemented");
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
  ReturnErrorInfo(NULL, CL_INVALID_OPERATION, "CL/GL interop not implemented");
}

CL_API_ENTRY cl_event CL_API_CALL
clCreateEventFromGLsyncKHR
(
  cl_context  context,
  cl_GLsync   cl_GLsync,
  cl_int *    errcode_ret
) CL_EXT_SUFFIX__VERSION_1_1
{
  SetErrorInfo(NULL, CL_INVALID_CONTEXT, "CL/GL interop not implemented");
  return NULL;
}

#if defined(_WIN32) && !defined(__MINGW32__) // DX extension functions

CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceIDsFromD3D10KHR
(
  cl_platform_id              platform,
  cl_d3d10_device_source_khr  d3d_device_source,
  void *                      d3d_object,
  cl_d3d10_device_set_khr     d3d_device_set,
  cl_uint                     num_entries,
  cl_device_id *              devices,
  cl_uint *                   num_devices
) CL_API_SUFFIX__VERSION_1_0
{
  ReturnErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromD3D10BufferKHR
(
  cl_context      context,
  cl_mem_flags    flags,
  ID3D10Buffer *  resource,
  cl_int *        errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  SetErrorInfo(NULL, CL_INVALID_CONTEXT, "CL/DX interop not implemented");
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromD3D10Texture2DKHR
(
  cl_context         context,
  cl_mem_flags       flags,
  ID3D10Texture2D *  resource,
  UINT               subresource,
  cl_int *           errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  SetErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromD3D10Texture3DKHR
(
  cl_context         context,
  cl_mem_flags       flags,
  ID3D10Texture3D *  resource,
  UINT               subresource,
  cl_int *           errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  SetErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueAcquireD3D10ObjectsKHR
(
  cl_command_queue  command_queue,
  cl_uint           num_objects,
  const cl_mem *    mem_objects,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
)CL_API_SUFFIX__VERSION_1_0
{
  ReturnErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReleaseD3D10ObjectsKHR
(
  cl_command_queue  command_queue,
  cl_uint           num_objects,
  const cl_mem *    mem_objects,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  ReturnErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
}

CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceIDsFromD3D11KHR
(
  cl_platform_id              platform,
  cl_d3d11_device_source_khr  d3d_device_source,
  void *                      d3d_object,
  cl_d3d11_device_set_khr     d3d_device_set,
  cl_uint                     num_entries,
  cl_device_id *              devices,
  cl_uint *                   num_devices
) CL_API_SUFFIX__VERSION_1_0
{
  ReturnErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromD3D11BufferKHR
(
  cl_context      context,
  cl_mem_flags    flags,
  ID3D11Buffer *  resource,
  cl_int *        errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  SetErrorInfo(NULL, CL_INVALID_CONTEXT, "CL/DX interop not implemented");
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromD3D11Texture2DKHR
(
  cl_context         context,
  cl_mem_flags       flags,
  ID3D11Texture2D *  resource,
  UINT               subresource,
  cl_int *           errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  SetErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromD3D11Texture3DKHR
(
  cl_context         context,
  cl_mem_flags       flags,
  ID3D11Texture3D *  resource,
  UINT               subresource,
  cl_int *           errcode_ret
) CL_API_SUFFIX__VERSION_1_0
{
  SetErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueAcquireD3D11ObjectsKHR
(
  cl_command_queue  command_queue,
  cl_uint           num_objects,
  const cl_mem *    mem_objects,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
)CL_API_SUFFIX__VERSION_1_0
{
  ReturnErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReleaseD3D11ObjectsKHR
(
  cl_command_queue  command_queue,
  cl_uint           num_objects,
  const cl_mem *    mem_objects,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_1_0
{
  ReturnErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
}

CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceIDsFromDX9MediaAdapterKHR
(
  cl_platform_id                   platform,
  cl_uint                          num_media_adapters,
  cl_dx9_media_adapter_type_khr *  media_adapter_type,
  void *                           media_adapters,
  cl_dx9_media_adapter_set_khr     media_adapter_set,
  cl_uint                          num_entries,
  cl_device_id *                   devices,
  cl_uint *                        num_devices
) CL_API_SUFFIX__VERSION_1_2
{
  ReturnErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromDX9MediaSurfaceKHR
(
  cl_context                     context,
  cl_mem_flags                   flags,
  cl_dx9_media_adapter_type_khr  adapter_type,
  void *                         surface_info,
  cl_uint                        plane,
  cl_int *                       errcode_ret
) CL_API_SUFFIX__VERSION_1_2
{
  SetErrorInfo(NULL, CL_INVALID_CONTEXT, "CL/DX interop not implemented");
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueAcquireDX9MediaSurfacesKHR
(
  cl_command_queue command_queue,
  cl_uint          num_objects,
  const cl_mem *   mem_objects,
  cl_uint          num_events_in_wait_list,
  const cl_event * event_wait_list,
  cl_event *       event
) CL_API_SUFFIX__VERSION_1_2
{
  ReturnErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReleaseDX9MediaSurfacesKHR
(
  cl_command_queue command_queue,
  cl_uint          num_objects,
  const cl_mem *   mem_objects,
  cl_uint          num_events_in_wait_list,
  const cl_event * event_wait_list,
  cl_event *       event
) CL_API_SUFFIX__VERSION_1_2
{
  ReturnErrorInfo(NULL, CL_INVALID_OPERATION, "CL/DX interop not implemented");
}

#endif // DX extension functions


/////////////////////
// OpenCL 2.0 APIs //
/////////////////////

CL_API_ENTRY cl_command_queue CL_API_CALL
clCreateCommandQueueWithProperties
(
  cl_context                  context,
  cl_device_id                device,
  const cl_queue_properties * properties,
  cl_int *                    errcode_ret
) CL_API_SUFFIX__VERSION_2_0
{
  // Check parameters
  if (!context)
  {
    SetErrorArg(NULL, CL_INVALID_CONTEXT, context);
    return NULL;
  }
  if (device != m_device)
  {
    SetErrorArg(context, CL_INVALID_DEVICE, device);
    return NULL;
  }

  // Parse properties
  cl_command_queue_properties props = 0;
  bool out_of_order = false;
  unsigned i = 0;
  while (properties && properties[i])
  {
    switch (properties[i++])
    {
    case CL_QUEUE_PROPERTIES:
      if (properties[i] & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)
      {
        out_of_order = true;
      }
      if (properties[i] &
          (CL_QUEUE_ON_DEVICE|CL_QUEUE_ON_DEVICE_DEFAULT))
      {
        SetErrorInfo(context, CL_INVALID_QUEUE_PROPERTIES,
                     "On device queues not implemented");
        return NULL;
      }
      props = properties[i];
      break;
    case CL_QUEUE_SIZE:
      SetErrorInfo(context, CL_INVALID_VALUE, "CL_QUEUE_SIZE not implemented");
      return NULL;
    default:
      SetErrorInfo(context, CL_INVALID_VALUE, properties);
      return NULL;
    }
    i++;
  }

  // Create command-queue object
  cl_command_queue queue;
  queue = new _cl_command_queue;
  queue->queue = new oclgrind::Queue(context->context, out_of_order);
  queue->dispatch = m_dispatchTable;
  queue->properties = props;
  queue->context = context;
  queue->refCount = 1;

  clRetainContext(context);

  SetError(context, CL_SUCCESS);
  return queue;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreatePipe
(
  cl_context                 context,
  cl_mem_flags               flags,
  cl_uint                    pipe_packet_size,
  cl_uint                    pipe_max_packets,
  const cl_pipe_properties * properties,
  cl_int *                   errcode_ret
) CL_API_SUFFIX__VERSION_2_0
{
  SetErrorInfo(context, CL_INVALID_OPERATION, "Unimplemented OpenCL 2.0 API");
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetPipeInfo
(
  cl_mem       pipe,
  cl_pipe_info param_name,
  size_t       param_value_size,
  void *       param_value,
  size_t *     param_value_size_ret
) CL_API_SUFFIX__VERSION_2_0
{
  ReturnErrorInfo(NULL, CL_INVALID_OPERATION, "Unimplemented OpenCL 2.0 API");
}

CL_API_ENTRY void * CL_API_CALL
clSVMAlloc
(
  cl_context       context,
  cl_svm_mem_flags flags,
  size_t           size,
  cl_uint          alignment
) CL_API_SUFFIX__VERSION_2_0
{
  notifyAPIError(context, CL_INVALID_OPERATION, __func__,
                 "Unimplemented OpenCL 2.0 API");
  return NULL;
}

CL_API_ENTRY void CL_API_CALL
clSVMFree
(
  cl_context context,
  void *     svm_pointer
) CL_API_SUFFIX__VERSION_2_0
{
  notifyAPIError(context, CL_INVALID_OPERATION, __func__,
                 "Unimplemented OpenCL 2.0 API");
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueSVMFree
(
  cl_command_queue command_queue,
  cl_uint num_svm_pointers,
  void* svm_pointers[],
  void (CL_CALLBACK* pfn_free_func)(
      cl_command_queue queue,
      cl_uint num_svm_pointers,
      void* svm_pointers[],
      void* user_data),
  void* user_data,
  cl_uint num_events_in_wait_list,
  const cl_event* event_wait_list,
  cl_event* event
) CL_API_SUFFIX__VERSION_2_0
{
  ReturnErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                  "Unimplemented OpenCL 2.0 API");
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueSVMMemcpy
(
  cl_command_queue  command_queue,
  cl_bool           blocking_copy,
  void *            dst_ptr,
  const void *      src_ptr,
  size_t            size,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_2_0
{
  ReturnErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                  "Unimplemented OpenCL 2.0 API");
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueSVMMemFill
(
  cl_command_queue command_queue,
  void *           svm_ptr,
  const void *     pattern,
  size_t           pattern_size,
  size_t           size,
  cl_uint          num_events_in_wait_list,
  const cl_event * event_wait_list,
  cl_event *       event
) CL_API_SUFFIX__VERSION_2_0
{
  ReturnErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                  "Unimplemented OpenCL 2.0 API");
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueSVMMap
(
  cl_command_queue  command_queue,
  cl_bool           blocking_map,
  cl_map_flags      flags,
  void *            svm_ptr,
  size_t            size,
  cl_uint           num_events_in_wait_list,
  const cl_event *  event_wait_list,
  cl_event *        event
) CL_API_SUFFIX__VERSION_2_0
{
  ReturnErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                  "Unimplemented OpenCL 2.0 API");
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueSVMUnmap
(
  cl_command_queue command_queue,
  void *           svm_ptr,
  cl_uint          num_events_in_wait_list,
  const cl_event * event_wait_list,
  cl_event *       event
) CL_API_SUFFIX__VERSION_2_0
{
  ReturnErrorInfo(command_queue->context, CL_INVALID_OPERATION,
                  "Unimplemented OpenCL 2.0 API");
}

CL_API_ENTRY cl_sampler CL_API_CALL
clCreateSamplerWithProperties
(
  cl_context                     context,
  const cl_sampler_properties *  sampler_properties,
  cl_int *                       errcode_ret
) CL_API_SUFFIX__VERSION_2_0
{
  // Check parameters
  if (!context)
  {
    SetErrorArg(NULL, CL_INVALID_CONTEXT, context);
    return NULL;
  }

  cl_bool             normalized_coords = CL_TRUE;
  cl_addressing_mode  addressing_mode   = CL_ADDRESS_CLAMP;
  cl_filter_mode      filter_mode       = CL_FILTER_NEAREST;

  // Parse properties
  unsigned i = 0;
  while (sampler_properties && sampler_properties[i])
  {
    switch (sampler_properties[i++])
    {
    case CL_SAMPLER_NORMALIZED_COORDS:
      normalized_coords = sampler_properties[i];
      break;
    case CL_SAMPLER_ADDRESSING_MODE:
      addressing_mode = sampler_properties[i];
      break;
    case CL_SAMPLER_FILTER_MODE:
      filter_mode = sampler_properties[i];
      break;
    default:
      SetErrorInfo(context, CL_INVALID_VALUE, sampler_properties);
      return NULL;
    }
    i++;
  }

  // Create sampler bitfield
  uint32_t bitfield = 0;

  if (normalized_coords)
  {
    bitfield |= CLK_NORMALIZED_COORDS_TRUE;
  }

  switch (addressing_mode)
  {
    case CL_ADDRESS_NONE:
      break;
    case CL_ADDRESS_CLAMP_TO_EDGE:
      bitfield |= CLK_ADDRESS_CLAMP_TO_EDGE;
      break;
    case CL_ADDRESS_CLAMP:
      bitfield |= CLK_ADDRESS_CLAMP;
      break;
    case CL_ADDRESS_REPEAT:
      bitfield |= CLK_ADDRESS_REPEAT;
      break;
    case CL_ADDRESS_MIRRORED_REPEAT:
      bitfield |= CLK_ADDRESS_MIRRORED_REPEAT;
      break;
    default:
      SetErrorArg(context, CL_INVALID_VALUE, sampler_properties);
      return NULL;
  }

  switch (filter_mode)
  {
    case CL_FILTER_NEAREST:
      bitfield |= CLK_FILTER_NEAREST;
      break;
    case CL_FILTER_LINEAR:
      bitfield |= CLK_FILTER_LINEAR;
      break;
    default:
      SetErrorArg(context, CL_INVALID_VALUE, sampler_properties);
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

  SetError(context, CL_SUCCESS);
  return sampler;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetKernelArgSVMPointer
(
  cl_kernel    kernel,
  cl_uint      arg_index,
  const void * arg_value
) CL_API_SUFFIX__VERSION_2_0
{
  ReturnErrorInfo(kernel->program->context, CL_INVALID_OPERATION,
                  "Unimplemented OpenCL 2.0 API");
}

CL_API_ENTRY cl_int CL_API_CALL
clSetKernelExecInfo
(
  cl_kernel            kernel,
  cl_kernel_exec_info  param_name,
  size_t               param_value_size,
  const void *         param_value
) CL_API_SUFFIX__VERSION_2_0
{
  ReturnErrorInfo(kernel->program->context, CL_INVALID_OPERATION,
                  "Unimplemented OpenCL 2.0 API");
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
  DISPATCH_TABLE_ENTRY(clGetDeviceIDsFromD3D10KHR),
  DISPATCH_TABLE_ENTRY(clCreateFromD3D10BufferKHR),
  DISPATCH_TABLE_ENTRY(clCreateFromD3D10Texture2DKHR),
  DISPATCH_TABLE_ENTRY(clCreateFromD3D10Texture3DKHR),
  DISPATCH_TABLE_ENTRY(clEnqueueAcquireD3D10ObjectsKHR),
  DISPATCH_TABLE_ENTRY(clEnqueueReleaseD3D10ObjectsKHR),
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
  DISPATCH_TABLE_ENTRY(clGetDeviceIDsFromD3D11KHR),
  DISPATCH_TABLE_ENTRY(clCreateFromD3D11BufferKHR),
  DISPATCH_TABLE_ENTRY(clCreateFromD3D11Texture2DKHR),
  DISPATCH_TABLE_ENTRY(clCreateFromD3D11Texture3DKHR),
  DISPATCH_TABLE_ENTRY(clCreateFromDX9MediaSurfaceKHR),
  DISPATCH_TABLE_ENTRY(clEnqueueAcquireD3D11ObjectsKHR),
  DISPATCH_TABLE_ENTRY(clEnqueueReleaseD3D11ObjectsKHR),
  DISPATCH_TABLE_ENTRY(clGetDeviceIDsFromDX9MediaAdapterKHR),
  DISPATCH_TABLE_ENTRY(clEnqueueAcquireDX9MediaSurfacesKHR),
  DISPATCH_TABLE_ENTRY(clEnqueueReleaseDX9MediaSurfacesKHR),
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

  // cl_khr_egl_image
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),
  DISPATCH_TABLE_ENTRY(NULL),

  // cl_khr_egl_event
  DISPATCH_TABLE_ENTRY(NULL),

  // OpenCL 2.0
  DISPATCH_TABLE_ENTRY(clCreateCommandQueueWithProperties),
  DISPATCH_TABLE_ENTRY(clCreatePipe),
  DISPATCH_TABLE_ENTRY(clGetPipeInfo),
  DISPATCH_TABLE_ENTRY(clSVMAlloc),
  DISPATCH_TABLE_ENTRY(clSVMFree),
  DISPATCH_TABLE_ENTRY(clEnqueueSVMFree),
  DISPATCH_TABLE_ENTRY(clEnqueueSVMMemcpy),
  DISPATCH_TABLE_ENTRY(clEnqueueSVMMemFill),
  DISPATCH_TABLE_ENTRY(clEnqueueSVMMap),
  DISPATCH_TABLE_ENTRY(clEnqueueSVMUnmap),
  DISPATCH_TABLE_ENTRY(clCreateSamplerWithProperties),
  DISPATCH_TABLE_ENTRY(clSetKernelArgSVMPointer),
  DISPATCH_TABLE_ENTRY(clSetKernelExecInfo),

  // cl_khr_sub_groups
  DISPATCH_TABLE_ENTRY(NULL),
};

#if defined(_WIN32) && !defined(OCLGRIND_ICD)

#include <Psapi.h>

// Function to replace calls to clGetPlatformIDs with
// the Oclgrind implementation.
//
// This is invoked by oclgrind.exe after this DLL is
// injected into the child process.
//
// Returns true on success, false on failure.
bool initOclgrind()
{
  // Get base address of process
  char *base = (char*)GetModuleHandle(NULL);

  // Get pointer to NT headers
  PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)(base);
  PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(base + dosHeader->e_lfanew);
  if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
  {
    std::cerr << "[Oclgrind] Invalid NT signature: "
              << ntHeaders->Signature << std::endl;
    return false;
  }

  // Get pointer to import directory
  DWORD importOffset =
    ntHeaders->OptionalHeader.
      DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
  PIMAGE_IMPORT_DESCRIPTOR importDesc =
    (PIMAGE_IMPORT_DESCRIPTOR)(base + importOffset);

  // Loop over directory entries
  while (importDesc->Name)
  {
    // Look for OpenCL.dll
    const char *modname = (const char*)(base + importDesc->Name);
    if (!stricmp(modname, "opencl.dll"))
    {
      // We use the OriginalFirstThunk to match the name,
      // and then replace the function pointer in FirstThunk
      PIMAGE_THUNK_DATA origThunk =
        (PIMAGE_THUNK_DATA)(base + importDesc->OriginalFirstThunk);
      PIMAGE_THUNK_DATA firstThunk =
        (PIMAGE_THUNK_DATA)(base + importDesc->FirstThunk);

      // Loop over functions
      while (origThunk->u1.AddressOfData)
      {
        // Skip unnamed functions
        if (!(origThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG))
        {
          // Get function name and check for clGetPlatformIDs
          PIMAGE_IMPORT_BY_NAME import =
            (PIMAGE_IMPORT_BY_NAME)(base + origThunk->u1.AddressOfData);
          if (!stricmp((char*)import->Name, "clGetPlatformIDs"))
          {
            // Make page writable temporarily
            MEMORY_BASIC_INFORMATION mbinfo;
            VirtualQuery(firstThunk, &mbinfo, sizeof(mbinfo));
            if (!VirtualProtect(mbinfo.BaseAddress, mbinfo.RegionSize,
                                PAGE_EXECUTE_READWRITE, &mbinfo.Protect))
            {
              std::cerr << "[Oclgrind] Failed to make page writeable: "
                        << GetLastError() << std::endl;
              return false;
            }

            // Replace function pointer with our implementation
            firstThunk->u1.Function = (ULONG64)clGetPlatformIDs;

            // Restore page protection
            DWORD zero = 0;
            if (!VirtualProtect(mbinfo.BaseAddress, mbinfo.RegionSize,
                                mbinfo.Protect, &zero))
            {
              std::cerr << "[Oclgrind] Failed to restore page protection: "
                        << GetLastError() << std::endl;
              return false;
            }

            return true;
          }
        }

        origThunk++;
        firstThunk++;
      }
    }
    importDesc++;
  }

  // We didn't find the function, so just warn user
  std::cerr << "[Oclgrind] Warning: unable to patch clGetPlatformIDs"
            << std::endl;

  return true;
}

#endif
