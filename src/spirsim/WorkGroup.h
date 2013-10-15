// WorkGroup.h (oclgrind)
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

#include "common.h"

namespace spirsim
{
  class Device;
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
      size_t num;
      size_t srcStride;
      size_t destStride;

      bool operator== (_AsyncCopy) const;
    } AsyncCopy;

  public:
    WorkGroup(Device *device, const Kernel& kernel, Memory &globalMem,
              unsigned int workDim,
              size_t wgid_x, size_t wgid_y, size_t wgid_z,
              const size_t globalOffset[3],
              const size_t wgsize[3],
              const size_t globalSize[3]);
    virtual ~WorkGroup();

    uint64_t async_copy(AsyncCopy copy, uint64_t event);
    void clearBarrier();
    const size_t* getGlobalOffset() const;
    const size_t* getGlobalSize() const;
    const size_t* getGroupID() const;
    const size_t* getGroupSize() const;
    Memory* getLocalMemory() const;
    WorkItem *getNextWorkItem() const;
    unsigned int getWorkDim() const;
    WorkItem *getWorkItem(size_t localID[3]) const;
    bool hasBarrier() const;
    void notifyBarrier(WorkItem *workItem);
    void notifyFinished(WorkItem *workItem);
    void wait_event(uint64_t event);

  private:
    unsigned int m_workDim;
    size_t m_globalOffset[3];
    size_t m_globalSize[3];
    size_t m_groupID[3];
    size_t m_groupSize[3];
    size_t m_totalWorkItems;
    Device *m_device;
    const Kernel& m_kernel;
    Memory *m_localMemory;
    WorkItem **m_workItems;

    // Comparator for ordering work-items
    struct WorkItemCmp
    {
      bool operator()(const WorkItem *lhs, const WorkItem *rhs) const;
    };
    std::set<WorkItem*, WorkItemCmp> m_running;
    std::set<WorkItem*, WorkItemCmp> m_barrier;

    uint64_t m_nextEvent;
    std::map< uint64_t, std::list<AsyncCopy> > m_pendingEvents;
    std::set<uint64_t> m_waitEvents;
  };
}
