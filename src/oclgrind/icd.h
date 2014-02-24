// icd.h (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#ifndef _ICD_H_
#define _ICD_H_

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
