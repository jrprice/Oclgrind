// icd.h (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#ifndef _ICD_H_
#define _ICD_H_

// Rename OpenCL API functions to avoid clashes with ICD library
#ifdef OCLGRIND_ICD
#define clGetPlatformIDs _clGetPlatformIDs_
#define clGetPlatformInfo _clGetPlatformInfo_
#define clGetDeviceIDs _clGetDeviceIDs_
#define clGetDeviceInfo _clGetDeviceInfo_
#define clCreateSubDevices _clCreateSubDevices_
#define clRetainDevice _clRetainDevice_
#define clReleaseDevice _clReleaseDevice_
#define clCreateContext _clCreateContext_
#define clCreateContextFromType _clCreateContextFromType_
#define clRetainContext _clRetainContext_
#define clReleaseContext _clReleaseContext_
#define clGetContextInfo _clGetContextInfo_
#define clCreateCommandQueue _clCreateCommandQueue_
#define clSetCommandQueueProperty _clSetCommandQueueProperty_
#define clRetainCommandQueue _clRetainCommandQueue_
#define clReleaseCommandQueue _clReleaseCommandQueue_
#define clGetCommandQueueInfo _clGetCommandQueueInfo_
#define clCreateBuffer _clCreateBuffer_
#define clCreateSubBuffer _clCreateSubBuffer_
#define clCreateImage _clCreateImage_
#define clCreateImage2D _clCreateImage2D_
#define clCreateImage3D _clCreateImage3D_
#define clRetainMemObject _clRetainMemObject_
#define clReleaseMemObject _clReleaseMemObject_
#define clGetSupportedImageFormats _clGetSupportedImageFormats_
#define clGetMemObjectInfo _clGetMemObjectInfo_
#define clGetImageInfo _clGetImageInfo_
#define clSetMemObjectDestructorCallback _clSetMemObjectDestructorCallback_
#define clCreateSampler _clCreateSampler_
#define clRetainSampler _clRetainSampler_
#define clReleaseSampler _clReleaseSampler_
#define clGetSamplerInfo _clGetSamplerInfo_
#define clCreateProgramWithSource _clCreateProgramWithSource_
#define clCreateProgramWithBinary _clCreateProgramWithBinary_
#define clCreateProgramWithBuiltInKernels _clCreateProgramWithBuiltInKernels_
#define clRetainProgram _clRetainProgram_
#define clReleaseProgram _clReleaseProgram_
#define clBuildProgram _clBuildProgram_
#define clUnloadCompiler _clUnloadCompiler_
#define clCompileProgram _clCompileProgram_
#define clLinkProgram _clLinkProgram_
#define clUnloadPlatformCompiler _clUnloadPlatformCompiler_
#define clGetProgramInfo _clGetProgramInfo_
#define clGetProgramBuildInfo _clGetProgramBuildInfo_
#define clCreateKernel _clCreateKernel_
#define clCreateKernelsInProgram _clCreateKernelsInProgram_
#define clRetainKernel _clRetainKernel_
#define clReleaseKernel _clReleaseKernel_
#define clSetKernelArg _clSetKernelArg_
#define clGetKernelInfo _clGetKernelInfo_
#define clGetKernelArgInfo _clGetKernelArgInfo_
#define clGetKernelWorkGroupInfo _clGetKernelWorkGroupInfo_
#define clWaitForEvents _clWaitForEvents_
#define clGetEventInfo _clGetEventInfo_
#define clCreateUserEvent _clCreateUserEvent_
#define clRetainEvent _clRetainEvent_
#define clReleaseEvent _clReleaseEvent_
#define clSetUserEventStatus _clSetUserEventStatus_
#define clSetEventCallback _clSetEventCallback_
#define clGetEventProfilingInfo _clGetEventProfilingInfo_
#define clFlush _clFlush_
#define clFinish _clFinish_
#define clEnqueueReadBuffer _clEnqueueReadBuffer_
#define clEnqueueReadBufferRect _clEnqueueReadBufferRect_
#define clEnqueueWriteBuffer _clEnqueueWriteBuffer_
#define clEnqueueWriteBufferRect _clEnqueueWriteBufferRect_
#define clEnqueueCopyBuffer _clEnqueueCopyBuffer_
#define clEnqueueCopyBufferRect _clEnqueueCopyBufferRect_
#define clEnqueueFillBuffer _clEnqueueFillBuffer_
#define clEnqueueFillImage _clEnqueueFillImage_
#define clEnqueueReadImage _clEnqueueReadImage_
#define clEnqueueWriteImage _clEnqueueWriteImage_
#define clEnqueueCopyImage _clEnqueueCopyImage_
#define clEnqueueCopyImageToBuffer _clEnqueueCopyImageToBuffer_
#define clEnqueueCopyBufferToImage _clEnqueueCopyBufferToImage_
#define clEnqueueMapBuffer _clEnqueueMapBuffer_
#define clEnqueueMapImage _clEnqueueMapImage_
#define clEnqueueUnmapMemObject _clEnqueueUnmapMemObject_
#define clEnqueueMigrateMemObjects _clEnqueueMigrateMemObjects_
#define clEnqueueNDRangeKernel _clEnqueueNDRangeKernel_
#define clEnqueueTask _clEnqueueTask_
#define clEnqueueNativeKernel _clEnqueueNativeKernel_
#define clGetExtensionFunctionAddressForPlatform _clGetExtensionFunctionAddressForPlatform_
#define clEnqueueMarkerWithWaitList _clEnqueueMarkerWithWaitList_
#define clEnqueueBarrierWithWaitList _clEnqueueBarrierWithWaitList_
#define clSetPrintfCallback _clSetPrintfCallback_
#define clEnqueueMarker _clEnqueueMarker_
#define clEnqueueWaitForEvents _clEnqueueWaitForEvents_
#define clEnqueueBarrier _clEnqueueBarrier_
#define clCreateFromGLBuffer _clCreateFromGLBuffer_
#define clCreateFromGLTexture _clCreateFromGLTexture_
#define clCreateFromGLTexture2D _clCreateFromGLTexture2D_
#define clCreateFromGLTexture3D _clCreateFromGLTexture3D_
#define clCreateFromGLRenderbuffer _clCreateFromGLRenderbuffer_
#define clGetGLObjectInfo _clGetGLObjectInfo_
#define clGetGLTextureInfo _clGetGLTextureInfo_
#define clEnqueueAcquireGLObjects _clEnqueueAcquireGLObjects_
#define clEnqueueReleaseGLObjects _clEnqueueReleaseGLObjects_
#define clGetGLContextInfoKHR _clGetGLContextInfoKHR_
#define clCreateEventFromGLsyncKHR _clCreateEventFromGLsyncKHR_
#endif // OCLGRIND_ICD

#include <cstdint>
#include <list>
#include <map>
#include <stack>

#include "CL/cl.h"
#include "CL/cl_ext.h"
#include "CL/cl_gl.h"
#include "CL/cl_gl_ext.h"

namespace spirsim
{
  class Device;
  class Kernel;
  class Program;
  class Queue;
  struct Event;
}

struct _cl_platform_id
{
  void *dispatch;
  const char *profile;
  const char *version;
  const char *name;
  const char *vendor;
  const char *extensions;
  const char *suffix;
};

struct _cl_device_id
{
  void **dispatch;
};

struct _cl_context
{
  void *dispatch;
  spirsim::Device *device;
  void (CL_CALLBACK *notify)(const char *, const void *, size_t, void *);
  void *data;
  cl_context_properties *properties;
  size_t szProperties;
  unsigned int refCount;
};

struct _cl_command_queue
{
  void *dispatch;
  cl_command_queue_properties properties;
  cl_context context;
  spirsim::Queue *queue;
  unsigned int refCount;
};

struct _cl_mem
{
  void *dispatch;
  cl_context context;
  cl_mem parent;
  size_t address;
  size_t size;
  size_t offset;
  cl_mem_flags flags;
  bool isImage;
  void *hostPtr;
  std::stack< std::pair<void (CL_CALLBACK*)(cl_mem, void *), void*> > callbacks;
  unsigned int refCount;
};

struct cl_image : _cl_mem
{
  cl_image_format format;
  cl_image_desc desc;
};

struct _cl_program
{
  void *dispatch;
  spirsim::Program *program;
  cl_context context;
  unsigned int refCount;
};

struct _cl_kernel
{
  void *dispatch;
  spirsim::Kernel *kernel;
  cl_program program;
  std::map<cl_uint, cl_mem> memArgs;
  unsigned int refCount;
};

struct _cl_event
{
  void *dispatch;
  cl_context context;
  cl_command_queue queue;
  cl_command_type type;
  spirsim::Event *event;
  std::list< std::pair<void (CL_CALLBACK*)(cl_event, cl_int, void*), void*> > callbacks;
  unsigned int refCount;
};

struct _cl_sampler
{
  void *dispatch;
  cl_context context;
  cl_bool normCoords;
  cl_addressing_mode addressMode;
  cl_filter_mode filterMode;
  uint32_t sampler;
  unsigned int refCount;
};

extern void *m_dispatchTable[256];

#endif // _ICD_H_
