#pragma once

#include "common.h"

namespace oclgrind
{
  class Device;
  class Kernel;
  class Memory;
  class WorkGroup;

  class Plugin
  {
  public:
    Plugin(Device *device);

    virtual void instructionExecuted(const llvm::Instruction *instruction,
                                     const TypedValue& result){}
    virtual void kernelBegin(const Kernel *kernel){}
    virtual void kernelEnd(const Kernel *kernel){}
    virtual void memoryAllocated(const Memory *memory, size_t address,
                                 size_t size){}
    virtual void memoryAtomic(const Memory *memory, size_t address,
                              size_t size){}
    virtual void memoryDeallocated(const Memory *memory, size_t address){}
    virtual void memoryLoad(const Memory *memory, size_t address, size_t size){}
    virtual void memoryStore(const Memory *memory, size_t address, size_t size,
                             const uint8_t *storeData){}
    virtual void workGroupBarrier(const WorkGroup *workGroup, uint32_t flags){}

  protected:
    Device *m_device;
  };
}
