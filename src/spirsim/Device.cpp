#include "common.h"
#include <istream>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/Support/InstIterator.h"

#include "Kernel.h"
#include "Memory.h"
#include "Device.h"
#include "WorkGroup.h"

using namespace spirsim;
using namespace std;

Device::Device()
{
  m_globalMemory = new Memory();
  m_outputMask = 0;

  // Check environment variables for output masks
  const char *env;

  env = getenv("OCLGRIND_OUTPUT_PRIVATE_MEM");
  if (env && strcmp(env, "1") == 0)
  {
    m_outputMask |= OUTPUT_PRIVATE_MEM;
  }
  env = getenv("OCLGRIND_OUTPUT_LOCAL_MEM");
  if (env && strcmp(env, "1") == 0)
  {
    m_outputMask |= OUTPUT_LOCAL_MEM;
  }
  env = getenv("OCLGRIND_OUTPUT_GLOBAL_MEM");
  if (env && strcmp(env, "1") == 0)
  {
    m_outputMask |= OUTPUT_GLOBAL_MEM;
  }
  env = getenv("OCLGRIND_OUTPUT_INSTRUCTIONS");
  if (env && strcmp(env, "1") == 0)
  {
    m_outputMask |= OUTPUT_INSTRUCTIONS;
  }
}

Device::~Device()
{
  delete m_globalMemory;
}

Memory* Device::getGlobalMemory() const
{
  return m_globalMemory;
}

void Device::run(const Kernel& kernel,
                    const size_t ndrange[3], const size_t wgsize[3])
{
  // Create work-groups
  size_t numGroups[3] = {ndrange[0]/wgsize[0],
                         ndrange[1]/wgsize[1],
                         ndrange[2]/wgsize[2]};
  size_t totalNumGroups = numGroups[0]*numGroups[1]*numGroups[2];
  for (int k = 0; k < numGroups[2]; k++)
  {
    for (int j = 0; j < numGroups[1]; j++)
    {
      for (int i = 0; i < numGroups[0]; i++)
      {
        if (m_outputMask &
            (OUTPUT_INSTRUCTIONS | OUTPUT_PRIVATE_MEM | OUTPUT_LOCAL_MEM))
        {
          cout << endl << BIG_SEPARATOR << endl;
          cout << "Work-group ("
               << i << ","
               << j << ","
               << k
               << ")" << endl;
          cout << BIG_SEPARATOR << endl;
        }

        WorkGroup *workGroup = new WorkGroup(kernel, *m_globalMemory,
                                             i, j, k, wgsize);

        workGroup->run(kernel, m_outputMask & OUTPUT_INSTRUCTIONS);

        // Dump contents of memories
        if (m_outputMask & OUTPUT_PRIVATE_MEM)
        {
          workGroup->dumpPrivateMemory();
        }
        if (m_outputMask & OUTPUT_LOCAL_MEM)
        {
          workGroup->dumpLocalMemory();
        }

        delete workGroup;
      }
    }
  }

  // Output global memory dump if required
  if (m_outputMask & OUTPUT_GLOBAL_MEM)
  {
    cout << endl << BIG_SEPARATOR << endl << "Global Memory:";
    m_globalMemory->dump();
    cout << BIG_SEPARATOR << endl;
  }
}

void Device::setOutputMask(unsigned char mask)
{
  m_outputMask = mask;
}
