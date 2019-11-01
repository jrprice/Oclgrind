// MemCheck.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/Plugin.h"

namespace llvm
{
    class GetElementPtrInst;
}

namespace oclgrind
{
  class MemCheck : public Plugin
  {
  public:
    MemCheck(const Context *context);

    virtual void instructionExecuted(const WorkItem *workItem,
                                     const llvm::Instruction *instruction,
                                     const TypedValue& result) override;
    virtual void memoryAtomicLoad(const Memory *memory,
                                  const WorkItem *workItem,
                                  AtomicOp op,
                                  size_t address, size_t size) override;
    virtual void memoryAtomicStore(const Memory *memory,
                                   const WorkItem *workItem,
                                   AtomicOp op,
                                   size_t address, size_t size) override;
    virtual void memoryLoad(const Memory *memory, const WorkItem *workItem,
                            size_t address, size_t size) override;
    virtual void memoryLoad(const Memory *memory, const WorkGroup *workGroup,
                            size_t address, size_t size) override;
    virtual void memoryMap(const Memory *memory, size_t address,
                           size_t offset, size_t size,
                           cl_map_flags flags) override;
    virtual void memoryStore(const Memory *memory, const WorkItem *workItem,
                             size_t address, size_t size,
                             const uint8_t *storeData) override;
    virtual void memoryStore(const Memory *memory, const WorkGroup *workGroup,
                             size_t address, size_t size,
                             const uint8_t *storeData) override;
    virtual void memoryUnmap(const Memory *memory, size_t address,
                             const void *ptr) override;

  private:
    void checkArrayAccess(const WorkItem *workItem,
                          const llvm::GetElementPtrInst *GEPI) const;
    void checkLoad(const Memory *memory, size_t address, size_t size) const;
    void checkStore(const Memory *memory, size_t address, size_t size) const;
    void logInvalidAccess(bool read, unsigned addrSpace,
                          size_t address, size_t size) const;

    struct MapRegion
    {
      size_t address;
      size_t offset;
      size_t size;
      const void *ptr;
      enum {READ, WRITE} type;
    };
    std::list<MapRegion> m_mapRegions;
  };
}
