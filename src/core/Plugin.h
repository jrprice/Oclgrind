// Plugin.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#pragma once

#include "common.h"

namespace oclgrind
{
  class Context;
  class Kernel;
  class KernelInvocation;
  class Memory;
  class WorkGroup;
  class WorkItem;

  class Plugin
  {
  public:
    Plugin(const Context *context);
    virtual ~Plugin();

    virtual void hostMemoryLoad(const Memory *memory,
                                size_t address, size_t size){}
    virtual void hostMemoryStore(const Memory *memory,
                                 size_t address, size_t size,
                                 const uint8_t *storeData){}
    virtual void instructionExecuted(const WorkItem *workItem,
                                     const llvm::Instruction *instruction,
                                     const TypedValue& result){}
    virtual void kernelBegin(const KernelInvocation *kernelInvocation){}
    virtual void kernelEnd(const KernelInvocation *kernelInvocation){}
    virtual void log(MessageType type, const char *message){}
    virtual void memoryAllocated(const Memory *memory, size_t address,
                                 size_t size, cl_mem_flags flags,
                                 const uint8_t *initData){}
    virtual void memoryAtomicLoad(const Memory *memory,
                                  const WorkItem *workItem,
                                  AtomicOp op, size_t address, size_t size){}
    virtual void memoryAtomicStore(const Memory *memory,
                                   const WorkItem *workItem,
                                   AtomicOp op, size_t address, size_t size){}
    virtual void memoryDeallocated(const Memory *memory, size_t address){}
    virtual void memoryLoad(const Memory *memory, const WorkItem *workItem,
                            size_t address, size_t size){}
    virtual void memoryLoad(const Memory *memory, const WorkGroup *workGroup,
                            size_t address, size_t size){}
    virtual void memoryMap(const Memory *memory, size_t address,
                           size_t offset, size_t size, cl_map_flags flags){}
    virtual void memoryStore(const Memory *memory, const WorkItem *workItem,
                             size_t address, size_t size,
                             const uint8_t *storeData){}
    virtual void memoryStore(const Memory *memory, const WorkGroup *workGroup,
                             size_t address, size_t size,
                             const uint8_t *storeData){}
    virtual void memoryUnmap(const Memory *memory, size_t address,
                             const void *ptr){}
    virtual void workGroupBarrier(const WorkGroup *workGroup, uint32_t flags){}
    virtual void workGroupBegin(const WorkGroup *workGroup){}
    virtual void workGroupComplete(const WorkGroup *workGroup){}
    virtual void workItemBegin(const WorkItem *workItem){}
    virtual void workItemComplete(const WorkItem *workItem){}
    virtual void workItemBarrier(const WorkItem *workItem){}
    virtual void workItemClearBarrier(const WorkItem *workItem){}

    virtual bool isThreadSafe() const;

  protected:
    const Context *m_context;
  };
}
