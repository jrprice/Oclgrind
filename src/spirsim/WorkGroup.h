#include "common.h"

namespace spirsim
{
  class Memory;
  class Kernel;
  class WorkItem;

  class WorkGroup
  {
  public:
    WorkGroup(const Kernel& kernel, Memory &globalMem,
              unsigned int workDim,
              size_t wgid_x, size_t wgid_y, size_t wgid_z,
              const size_t wgsize[3],
              const size_t globalSize[3]);
    virtual ~WorkGroup();

    void dumpLocalMemory() const;
    void dumpPrivateMemory() const;
    const size_t* getGlobalSize() const;
    const size_t* getGroupID() const;
    const size_t* getGroupSize() const;
    Memory* getLocalMemory() const;
    unsigned int getWorkDim() const;
    void run(const Kernel& kernel, bool outputInstructions=false);

  private:
    unsigned int m_workDim;
    size_t m_globalSize[3];
    size_t m_groupID[3];
    size_t m_groupSize[3];
    size_t m_totalWorkItems;
    Memory *m_localMemory;
    Memory& m_globalMemory;
    WorkItem **m_workItems;
  };
}
