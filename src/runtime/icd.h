// icd.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#ifndef _ICD_H_
#define _ICD_H_

// Rename OpenCL API functions to avoid clashes with ICD library
#ifdef OCLGRIND_ICD
#define clGetPlatformIDs _clGetPlatformIDs
#define clGetPlatformInfo _clGetPlatformInfo
#define clGetDeviceIDs _clGetDeviceIDs
#define clGetDeviceInfo _clGetDeviceInfo
#define clCreateSubDevices _clCreateSubDevices
#define clRetainDevice _clRetainDevice
#define clReleaseDevice _clReleaseDevice
#define clCreateContext _clCreateContext
#define clCreateContextFromType _clCreateContextFromType
#define clRetainContext _clRetainContext
#define clReleaseContext _clReleaseContext
#define clGetContextInfo _clGetContextInfo
#define clCreateCommandQueue _clCreateCommandQueue
#define clSetCommandQueueProperty _clSetCommandQueueProperty
#define clRetainCommandQueue _clRetainCommandQueue
#define clReleaseCommandQueue _clReleaseCommandQueue
#define clGetCommandQueueInfo _clGetCommandQueueInfo
#define clCreateBuffer _clCreateBuffer
#define clCreateSubBuffer _clCreateSubBuffer
#define clCreateImage _clCreateImage
#define clCreateImage2D _clCreateImage2D
#define clCreateImage3D _clCreateImage3D
#define clRetainMemObject _clRetainMemObject
#define clReleaseMemObject _clReleaseMemObject
#define clGetSupportedImageFormats _clGetSupportedImageFormats
#define clGetMemObjectInfo _clGetMemObjectInfo
#define clGetImageInfo _clGetImageInfo
#define clSetMemObjectDestructorCallback _clSetMemObjectDestructorCallback
#define clCreateSampler _clCreateSampler
#define clRetainSampler _clRetainSampler
#define clReleaseSampler _clReleaseSampler
#define clGetSamplerInfo _clGetSamplerInfo
#define clCreateProgramWithSource _clCreateProgramWithSource
#define clCreateProgramWithBinary _clCreateProgramWithBinary
#define clCreateProgramWithBuiltInKernels _clCreateProgramWithBuiltInKernels
#define clRetainProgram _clRetainProgram
#define clReleaseProgram _clReleaseProgram
#define clBuildProgram _clBuildProgram
#define clUnloadCompiler _clUnloadCompiler
#define clCompileProgram _clCompileProgram
#define clLinkProgram _clLinkProgram
#define clUnloadPlatformCompiler _clUnloadPlatformCompiler
#define clGetProgramInfo _clGetProgramInfo
#define clGetProgramBuildInfo _clGetProgramBuildInfo
#define clCreateKernel _clCreateKernel
#define clCreateKernelsInProgram _clCreateKernelsInProgram
#define clRetainKernel _clRetainKernel
#define clReleaseKernel _clReleaseKernel
#define clSetKernelArg _clSetKernelArg
#define clGetKernelInfo _clGetKernelInfo
#define clGetKernelArgInfo _clGetKernelArgInfo
#define clGetKernelWorkGroupInfo _clGetKernelWorkGroupInfo
#define clWaitForEvents _clWaitForEvents
#define clGetEventInfo _clGetEventInfo
#define clCreateUserEvent _clCreateUserEvent
#define clRetainEvent _clRetainEvent
#define clReleaseEvent _clReleaseEvent
#define clSetUserEventStatus _clSetUserEventStatus
#define clSetEventCallback _clSetEventCallback
#define clGetEventProfilingInfo _clGetEventProfilingInfo
#define clFlush _clFlush
#define clFinish _clFinish
#define clEnqueueReadBuffer _clEnqueueReadBuffer
#define clEnqueueReadBufferRect _clEnqueueReadBufferRect
#define clEnqueueWriteBuffer _clEnqueueWriteBuffer
#define clEnqueueWriteBufferRect _clEnqueueWriteBufferRect
#define clEnqueueCopyBuffer _clEnqueueCopyBuffer
#define clEnqueueCopyBufferRect _clEnqueueCopyBufferRect
#define clEnqueueFillBuffer _clEnqueueFillBuffer
#define clEnqueueFillImage _clEnqueueFillImage
#define clEnqueueReadImage _clEnqueueReadImage
#define clEnqueueWriteImage _clEnqueueWriteImage
#define clEnqueueCopyImage _clEnqueueCopyImage
#define clEnqueueCopyImageToBuffer _clEnqueueCopyImageToBuffer
#define clEnqueueCopyBufferToImage _clEnqueueCopyBufferToImage
#define clEnqueueMapBuffer _clEnqueueMapBuffer
#define clEnqueueMapImage _clEnqueueMapImage
#define clEnqueueUnmapMemObject _clEnqueueUnmapMemObject
#define clEnqueueMigrateMemObjects _clEnqueueMigrateMemObjects
#define clEnqueueNDRangeKernel _clEnqueueNDRangeKernel
#define clEnqueueTask _clEnqueueTask
#define clEnqueueNativeKernel _clEnqueueNativeKernel
#define clGetExtensionFunctionAddressForPlatform _clGetExtensionFunctionAddressForPlatform
#define clEnqueueMarkerWithWaitList _clEnqueueMarkerWithWaitList
#define clEnqueueBarrierWithWaitList _clEnqueueBarrierWithWaitList
#define clSetPrintfCallback _clSetPrintfCallback
#define clEnqueueMarker _clEnqueueMarker
#define clEnqueueWaitForEvents _clEnqueueWaitForEvents
#define clEnqueueBarrier _clEnqueueBarrier
#define clCreateFromGLBuffer _clCreateFromGLBuffer
#define clCreateFromGLTexture _clCreateFromGLTexture
#define clCreateFromGLTexture2D _clCreateFromGLTexture2D
#define clCreateFromGLTexture3D _clCreateFromGLTexture3D
#define clCreateFromGLRenderbuffer _clCreateFromGLRenderbuffer
#define clGetGLObjectInfo _clGetGLObjectInfo
#define clGetGLTextureInfo _clGetGLTextureInfo
#define clEnqueueAcquireGLObjects _clEnqueueAcquireGLObjects
#define clEnqueueReleaseGLObjects _clEnqueueReleaseGLObjects
#define clGetGLContextInfoKHR _clGetGLContextInfoKHR
#define clCreateEventFromGLsyncKHR _clCreateEventFromGLsyncKHR
#endif // OCLGRIND_ICD

#include <list>
#include <map>
#include <stack>
#include <stdint.h>

#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS

#include "CL/cl.h"
#include "CL/cl_ext.h"
#include "CL/cl_gl.h"
#include "CL/cl_gl_ext.h"
#if defined(_WIN32) && !defined(__MINGW32__)
#include "CL/cl_d3d11.h"
#include "CL/cl_d3d10.h"
#include "CL/cl_dx9_media_sharing.h"
#endif

namespace oclgrind
{
  class Context;
  class Kernel;
  class Program;
  class Queue;
  struct Command;
  struct Event;
  struct Image;
}

struct _cl_platform_id
{
  void *dispatch;
};

struct _cl_device_id
{
  void **dispatch;
  size_t globalMemSize;
  size_t constantMemSize;
  size_t localMemSize;
  size_t maxWGSize;
};

struct _cl_context
{
  void *dispatch;
  oclgrind::Context *context;
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
  oclgrind::Queue *queue;
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
  oclgrind::Program *program;
  cl_context context;
  unsigned int refCount;
};

struct _cl_kernel
{
  void *dispatch;
  oclgrind::Kernel *kernel;
  cl_program program;
  std::map<cl_uint, cl_mem> memArgs;
  std::stack<oclgrind::Image*> imageArgs;
  unsigned int refCount;
};

struct _cl_event
{
  void *dispatch;
  cl_context context;
  cl_command_queue queue;
  cl_command_type type;
  oclgrind::Event *event;
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
