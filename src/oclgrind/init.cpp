// init.cpp (oclgrind)
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

#include <cstring>
#include <cassert>
#include <iostream>

#include "icd.h"

#include "CL/cl.h"
#include "CL/cl_gl.h"
#include "CL/cl_gl_ext.h"

using namespace std;

extern CLIicdDispatchTable *m_dispatchTable;
extern _cl_platform_id *m_platform;

CL_API_ENTRY void* CL_API_CALL
clGetExtensionFunctionAddress(const char *funcname) CL_API_SUFFIX__VERSION_1_2
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
clIcdGetPlatformIDsKHR(cl_uint num_entries,
                       cl_platform_id *platforms,
                       cl_uint *num_platforms)
{
  if (!m_platform)
  {
    m_platform = (cl_platform_id)malloc(sizeof(struct _cl_platform_id));

    cliIcdDispatchTableCreate(&m_dispatchTable);

    m_platform->version = "OpenCL 1.2 oclgrind";
    m_platform->vendor = "James Price, University of Bristol";
    m_platform->profile = "FULL_PROFILE";
    m_platform->name = "oclgrind";
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

/*
 * Prototypes for deprecated functions no longer present in cl.h
 */
extern CL_API_ENTRY cl_int CL_API_CALL
clSetCommandQueueProperty(cl_command_queue              /* command_queue */,
                          cl_command_queue_properties   /* properties */,
                          cl_bool                       /* enable */,
                          cl_command_queue_properties * /* old_properties */);

#define ICD_DISPATCH_TABLE_ENTRY(fn)                                    \
  assert(dispatchTable->entryCount < 256);                              \
  dispatchTable->entries[dispatchTable->entryCount++] = (void*)(fn)

cl_int cliIcdDispatchTableCreate(CLIicdDispatchTable **outDispatchTable)
{
  CLIicdDispatchTable *dispatchTable = NULL;
  cl_int result = CL_SUCCESS;

  // allocate the public handle
  dispatchTable = (CLIicdDispatchTable *) malloc(sizeof(*dispatchTable));
  if (!dispatchTable)
  {
    result = CL_OUT_OF_HOST_MEMORY;
    goto Error;
  }
  memset(dispatchTable, 0, sizeof(*dispatchTable));

  // OpenCL 1.0
  ICD_DISPATCH_TABLE_ENTRY ( clGetPlatformIDs              );
  ICD_DISPATCH_TABLE_ENTRY ( clGetPlatformInfo             );
  ICD_DISPATCH_TABLE_ENTRY ( clGetDeviceIDs                );
  ICD_DISPATCH_TABLE_ENTRY ( clGetDeviceInfo               );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateContext               );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateContextFromType       );
  ICD_DISPATCH_TABLE_ENTRY ( clRetainContext               );
  ICD_DISPATCH_TABLE_ENTRY ( clReleaseContext              );
  ICD_DISPATCH_TABLE_ENTRY ( clGetContextInfo              );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateCommandQueue          );
  ICD_DISPATCH_TABLE_ENTRY ( clRetainCommandQueue          );
  ICD_DISPATCH_TABLE_ENTRY ( clReleaseCommandQueue         );
  ICD_DISPATCH_TABLE_ENTRY ( clGetCommandQueueInfo         );
  ICD_DISPATCH_TABLE_ENTRY ( clSetCommandQueueProperty     );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateBuffer                );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateImage2D               );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateImage3D               );
  ICD_DISPATCH_TABLE_ENTRY ( clRetainMemObject             );
  ICD_DISPATCH_TABLE_ENTRY ( clReleaseMemObject            );
  ICD_DISPATCH_TABLE_ENTRY ( clGetSupportedImageFormats    );
  ICD_DISPATCH_TABLE_ENTRY ( clGetMemObjectInfo            );
  ICD_DISPATCH_TABLE_ENTRY ( clGetImageInfo                );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateSampler               );
  ICD_DISPATCH_TABLE_ENTRY ( clRetainSampler               );
  ICD_DISPATCH_TABLE_ENTRY ( clReleaseSampler              );
  ICD_DISPATCH_TABLE_ENTRY ( clGetSamplerInfo              );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateProgramWithSource     );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateProgramWithBinary     );
  ICD_DISPATCH_TABLE_ENTRY ( clRetainProgram               );
  ICD_DISPATCH_TABLE_ENTRY ( clReleaseProgram              );
  ICD_DISPATCH_TABLE_ENTRY ( clBuildProgram                );
  ICD_DISPATCH_TABLE_ENTRY ( clUnloadCompiler              );
  ICD_DISPATCH_TABLE_ENTRY ( clGetProgramInfo              );
  ICD_DISPATCH_TABLE_ENTRY ( clGetProgramBuildInfo         );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateKernel                );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateKernelsInProgram      );
  ICD_DISPATCH_TABLE_ENTRY ( clRetainKernel                );
  ICD_DISPATCH_TABLE_ENTRY ( clReleaseKernel               );
  ICD_DISPATCH_TABLE_ENTRY ( clSetKernelArg                );
  ICD_DISPATCH_TABLE_ENTRY ( clGetKernelInfo               );
  ICD_DISPATCH_TABLE_ENTRY ( clGetKernelWorkGroupInfo      );
  ICD_DISPATCH_TABLE_ENTRY ( clWaitForEvents               );
  ICD_DISPATCH_TABLE_ENTRY ( clGetEventInfo                );
  ICD_DISPATCH_TABLE_ENTRY ( clRetainEvent                 );
  ICD_DISPATCH_TABLE_ENTRY ( clReleaseEvent                );
  ICD_DISPATCH_TABLE_ENTRY ( clGetEventProfilingInfo       );
  ICD_DISPATCH_TABLE_ENTRY ( clFlush                       );
  ICD_DISPATCH_TABLE_ENTRY ( clFinish                      );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueReadBuffer           );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueWriteBuffer          );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueCopyBuffer           );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueReadImage            );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueWriteImage           );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueCopyImage            );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueCopyImageToBuffer    );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueCopyBufferToImage    );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueMapBuffer            );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueMapImage             );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueUnmapMemObject       );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueNDRangeKernel        );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueTask                 );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueNativeKernel         );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueMarker               );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueWaitForEvents        );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueBarrier              );
  ICD_DISPATCH_TABLE_ENTRY ( clGetExtensionFunctionAddress );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateFromGLBuffer          );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateFromGLTexture2D       );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateFromGLTexture3D       );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateFromGLRenderbuffer    );
  ICD_DISPATCH_TABLE_ENTRY ( clGetGLObjectInfo             );
  ICD_DISPATCH_TABLE_ENTRY ( clGetGLTextureInfo            );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueAcquireGLObjects     );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueReleaseGLObjects     );

  // cl_khr_gl_sharing
  ICD_DISPATCH_TABLE_ENTRY ( clGetGLContextInfoKHR         );

  // cl_khr_d3d10_sharing (windows-only)
#if 0 && defined(_WIN32)
  ICD_DISPATCH_TABLE_ENTRY ( clGetDeviceIDsFromD3D10KHR      );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateFromD3D10BufferKHR      );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateFromD3D10Texture2DKHR   );
  ICD_DISPATCH_TABLE_ENTRY ( clCreateFromD3D10Texture3DKHR   );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueAcquireD3D10ObjectsKHR );
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueReleaseD3D10ObjectsKHR );
#else
  ICD_DISPATCH_TABLE_ENTRY( NULL );
  ICD_DISPATCH_TABLE_ENTRY( NULL );
  ICD_DISPATCH_TABLE_ENTRY( NULL );
  ICD_DISPATCH_TABLE_ENTRY( NULL );
  ICD_DISPATCH_TABLE_ENTRY( NULL );
  ICD_DISPATCH_TABLE_ENTRY( NULL );
#endif

  // OpenCL 1.1
  ICD_DISPATCH_TABLE_ENTRY ( clSetEventCallback);
  ICD_DISPATCH_TABLE_ENTRY ( clCreateSubBuffer);
  ICD_DISPATCH_TABLE_ENTRY ( clSetMemObjectDestructorCallback);
  ICD_DISPATCH_TABLE_ENTRY ( clCreateUserEvent);
  ICD_DISPATCH_TABLE_ENTRY ( clSetUserEventStatus);
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueReadBufferRect);
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueWriteBufferRect);
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueCopyBufferRect);

  ICD_DISPATCH_TABLE_ENTRY ( /*clCreateSubDevicesEXT*/NULL);
  ICD_DISPATCH_TABLE_ENTRY ( /*clRetainDeviceEXT*/ NULL);
  ICD_DISPATCH_TABLE_ENTRY ( /*clReleaseDevice*/NULL);

  ICD_DISPATCH_TABLE_ENTRY ( clCreateEventFromGLsyncKHR);

  ICD_DISPATCH_TABLE_ENTRY ( clCreateSubDevices);
  ICD_DISPATCH_TABLE_ENTRY ( clRetainDevice);
  ICD_DISPATCH_TABLE_ENTRY ( clReleaseDevice);
  ICD_DISPATCH_TABLE_ENTRY ( clCreateImage);
  ICD_DISPATCH_TABLE_ENTRY ( clCreateProgramWithBuiltInKernels);
  ICD_DISPATCH_TABLE_ENTRY ( clCompileProgram);
  ICD_DISPATCH_TABLE_ENTRY ( clLinkProgram);
  ICD_DISPATCH_TABLE_ENTRY ( clUnloadPlatformCompiler);
  ICD_DISPATCH_TABLE_ENTRY ( clGetKernelArgInfo);
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueFillBuffer);
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueFillImage);
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueMigrateMemObjects);
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueMarkerWithWaitList);
  ICD_DISPATCH_TABLE_ENTRY ( clEnqueueBarrierWithWaitList);
  ICD_DISPATCH_TABLE_ENTRY ( clGetExtensionFunctionAddressForPlatform);
  ICD_DISPATCH_TABLE_ENTRY ( clCreateFromGLTexture);

  // return success
  *outDispatchTable = dispatchTable;
  return CL_SUCCESS;

Error:
  return result;
}

void cliIcdDispatchTableDestroy(CLIicdDispatchTable *dispatchTable)
{
  free(dispatchTable);
}
