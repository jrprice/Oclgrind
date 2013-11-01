// icd.h (oclgrind)
// Copyright (C) 2013 James Price
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

#ifndef _ICD_H_
#define _ICD_H_

// Need to rename all CL API functions to prevent ICD loader functions calling
// themselves via the dispatch table. Include this before cl headers.
#include "rename_api.h"

#include <CL/cl.h>
#include <list>
#include <map>
#include <stack>

#ifndef CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#endif

#ifndef CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#endif

#define CL_PLATFORM_ICD_SUFFIX_KHR 0x0920

namespace spirsim
{
  class Device;
  class Kernel;
  class Program;
  class Queue;
  struct Event;
}

typedef struct CLIicdDispatchTable_st
{
  void *entries[256];
  int entryCount;
} CLIicdDispatchTable;

struct _cl_platform_id
{
  CLIicdDispatchTable *dispatch;
  const char *profile;
  const char *version;
  const char *name;
  const char *vendor;
  const char *extensions;
  const char *suffix;
};

struct _cl_device_id
{
  CLIicdDispatchTable* dispatch;
};

struct _cl_context
{
  CLIicdDispatchTable* dispatch;
  spirsim::Device *device;
  void (CL_CALLBACK *notify)(const char *, const void *, size_t, void *);
  void *data;
  cl_context_properties *properties;
  size_t szProperties;
  unsigned int refCount;
};

struct _cl_command_queue
{
  CLIicdDispatchTable* dispatch;
  cl_command_queue_properties properties;
  cl_context context;
  spirsim::Queue *queue;
  unsigned int refCount;
};

struct _cl_mem
{
  CLIicdDispatchTable* dispatch;
  cl_context context;
  cl_mem parent;
  size_t address;
  size_t size;
  size_t offset;
  cl_mem_flags flags;
  bool isImage;
  void *hostPtr;
  std::stack< std::pair<void (CL_CALLBACK *)(cl_mem, void *), void*> > callbacks;
  unsigned int refCount;
};

struct cl_image : _cl_mem
{
  cl_image_format format;
  cl_image_desc desc;
};

struct _cl_program
{
  CLIicdDispatchTable* dispatch;
  spirsim::Program *program;
  cl_context context;
  unsigned int refCount;
};

struct _cl_kernel
{
  CLIicdDispatchTable* dispatch;
  spirsim::Kernel *kernel;
  cl_program program;
  std::map<cl_uint, cl_mem> memArgs;
  unsigned int refCount;
};

struct _cl_event
{
  CLIicdDispatchTable* dispatch;
  cl_context context;
  cl_command_queue queue;
  cl_command_type type;
  spirsim::Event *event;
  std::list< std::pair<void (CL_CALLBACK *)(cl_event, cl_int, void *), void*> > callbacks;
  unsigned int refCount;
};

struct _cl_sampler
{
  CLIicdDispatchTable* dispatch;
  uint32_t sampler;
  unsigned int refCount;
};

cl_int cliIcdDispatchTableCreate(CLIicdDispatchTable **outDispatchTable);

CL_API_ENTRY cl_int CL_API_CALL
clIcdGetPlatformIDsKHR(cl_uint num_entries,
                       cl_platform_id *platforms,
                       cl_uint *num_platforms);

#endif /* _ICD_STRUCTS_H_ */
