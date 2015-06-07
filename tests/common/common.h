#include <CL/cl.h>

typedef struct
{
  cl_platform_id   platform;
  cl_device_id     device;
  cl_context       context;
  cl_command_queue queue;
  cl_program       program;
} Context;

void checkError(cl_int err, const char *operation);

Context createContext(const char *source);
void    releaseContext(Context cl);
