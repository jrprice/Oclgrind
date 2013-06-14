#include "common.h"

class Memory;
class Kernel;
class WorkItem;

class WorkGroup
{
public:
  WorkGroup(const Kernel& kernel, Memory &globalMem,
            size_t wgid_x, size_t wgid_y, size_t wgid_z,
            const size_t wgsize[3]);
  virtual ~WorkGroup();

  void dumpLocalMemory() const;
  void dumpPrivateMemory() const;
  const size_t* getGroupID() const;
  const size_t* getGroupSize() const;
  Memory* getLocalMemory() const;
  void run(const Kernel& kernel, bool outputInstructions=false);

private:
  size_t m_groupID[3];
  size_t m_groupSize[3];
  size_t m_totalWorkItems;
  Memory *m_localMemory;
  Memory& m_globalMemory;
  WorkItem **m_workItems;
};
