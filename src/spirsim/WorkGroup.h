#include "common.h"

namespace spirsim
{
  class Memory;
  class Kernel;
  class WorkItem;

  class WorkGroup
  {
  public:
    enum AsyncCopyType{GLOBAL_TO_LOCAL, LOCAL_TO_GLOBAL};
    typedef struct _AsyncCopy
    {
      const llvm::Instruction *instruction;
      AsyncCopyType type;
      size_t dest;
      size_t src;
      size_t size;

      bool operator== (_AsyncCopy) const;
    } AsyncCopy;

  public:
    WorkGroup(const Kernel& kernel, Memory &globalMem,
              unsigned int workDim,
              size_t wgid_x, size_t wgid_y, size_t wgid_z,
              const size_t wgsize[3],
              const size_t globalSize[3]);
    virtual ~WorkGroup();

    uint64_t async_copy(AsyncCopy copy, uint64_t event);
    void dumpLocalMemory() const;
    void dumpPrivateMemory() const;
    const size_t* getGlobalSize() const;
    const size_t* getGroupID() const;
    const size_t* getGroupSize() const;
    Memory* getLocalMemory() const;
    unsigned int getWorkDim() const;
    void run(const Kernel& kernel, bool outputInstructions=false);
    void wait_event(uint64_t event);

  private:
    unsigned int m_workDim;
    size_t m_globalSize[3];
    size_t m_groupID[3];
    size_t m_groupSize[3];
    size_t m_totalWorkItems;
    Memory *m_localMemory;
    Memory& m_globalMemory;
    WorkItem **m_workItems;

    uint64_t m_nextEvent;
    std::map< uint64_t, std::list<AsyncCopy> > m_pendingEvents;
    std::set<uint64_t> m_waitEvents;
  };
}
