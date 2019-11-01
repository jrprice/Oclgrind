// WorkGroup.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

#define CLK_LOCAL_MEM_FENCE  (1<<0)
#define CLK_GLOBAL_MEM_FENCE (1<<1)

namespace oclgrind
{
  class Context;
  class Memory;
  class Kernel;
  class KernelInvocation;
  class WorkItem;

  class WorkGroup
  {
  public:
    enum AsyncCopyType{GLOBAL_TO_LOCAL, LOCAL_TO_GLOBAL};

  private:
    // Comparator for ordering work-items
    struct WorkItemCmp
    {
      bool operator()(const WorkItem *lhs, const WorkItem *rhs) const;
    };
    std::set<WorkItem*, WorkItemCmp> m_running;

    struct AsyncCopy
    {
      const llvm::Instruction *instruction;
      AsyncCopyType type;
      size_t dest;
      size_t src;
      size_t size;
      size_t num;
      size_t srcStride;
      size_t destStride;

      size_t event;
    };

    struct Barrier
    {
      const llvm::Instruction *instruction;
      std::set<WorkItem*, WorkItemCmp> workItems;

      uint64_t fence;
      std::list<size_t> events;
    };

  public:
    WorkGroup(const KernelInvocation *kernelInvocation, Size3 wgid);
    WorkGroup(const KernelInvocation *kernelInvocation, Size3 wgid, Size3 size);
    virtual ~WorkGroup();

    size_t async_copy(
      const WorkItem *workItem,
      const llvm::Instruction *instruction,
      AsyncCopyType type,
      size_t dest,
      size_t src,
      size_t size,
      size_t num,
      size_t srcStride,
      size_t destStride,
      size_t event);
    void clearBarrier();
    const llvm::Instruction* getCurrentBarrier() const;
    Size3 getGroupID() const;
    size_t getGroupIndex() const;
    Size3 getGroupSize() const;
    Memory* getLocalMemory() const;
    size_t getLocalMemoryAddress(const llvm::Value *value) const;
    WorkItem *getNextWorkItem() const;
    WorkItem *getWorkItem(Size3 localID) const;
    bool hasBarrier() const;
    void notifyBarrier(WorkItem *workItem, const llvm::Instruction *instruction,
                       uint64_t fence,
                       std::list<size_t> events=std::list<size_t>());
    void notifyFinished(WorkItem *workItem);

  private:
    size_t m_groupIndex;
    Size3 m_groupID;
    Size3 m_groupSize;
    const Context *m_context;

    Memory *m_localMemory;
    std::map<const llvm::Value*,size_t> m_localAddresses;

    std::vector<WorkItem*> m_workItems;

    Barrier *m_barrier;
    size_t m_nextEvent;
    std::list< std::pair<AsyncCopy,std::set<const WorkItem*> > > m_asyncCopies;
    std::map < size_t, std::list<AsyncCopy> > m_events;
  };
}
