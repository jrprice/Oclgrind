// RaceDetector.h (Oclgrind)
// Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/Plugin.h"

namespace oclgrind
{
  class RaceDetector : public Plugin
  {
  public:
    RaceDetector(const Context *context);

    virtual void kernelBegin(const KernelInvocation *kernelInvocation) override;
    virtual void kernelEnd(const KernelInvocation *kernelInvocation) override;
    virtual void memoryAllocated(const Memory *memory, size_t address,
                                 size_t size, cl_mem_flags flags,
                                 const uint8_t *initData) override;
    virtual void memoryAtomicLoad(const Memory *memory,
                                  const WorkItem *workItem,
                                  AtomicOp op,
                                  size_t address, size_t size) override;
    virtual void memoryAtomicStore(const Memory *memory,
                                   const WorkItem *workItem,
                                   AtomicOp op,
                                   size_t address, size_t size) override;
    virtual void memoryDeallocated(const Memory *memory, size_t address);
    virtual void memoryLoad(const Memory *memory, const WorkItem *workItem,
                            size_t address, size_t size) override;
    virtual void memoryLoad(const Memory *memory, const WorkGroup *workGroup,
                            size_t address, size_t size) override;
    virtual void memoryStore(const Memory *memory, const WorkItem *workItem,
                             size_t address, size_t size,
                             const uint8_t *storeData) override;
    virtual void memoryStore(const Memory *memory, const WorkGroup *workGroup,
                             size_t address, size_t size,
                             const uint8_t *storeData) override;
    virtual void workGroupBarrier(const WorkGroup *workGroup,
                                  uint32_t flags) override;

  private:
    struct MemoryAccess
    {
    private:
      Size3 entity;
      const llvm::Instruction *instruction;

      uint8_t info;
      static const unsigned STORE_BIT   = 0;
      static const unsigned ATOMIC_BIT  = 1;
      static const unsigned WG_BIT      = 2;
      static const unsigned WG_SYNC_BIT = 3;
      uint8_t storeData;

    public:
      bool isAtomic() const;
      bool isLoad() const;
      bool isStore() const;
      bool isWorkGroup() const;
      bool isWorkItem() const;

      bool hasWorkGroupSync() const;
      void setWorkGroupSync();

      Size3 getEntity() const;
      const llvm::Instruction* getInstruction() const;

      uint8_t getStoreData() const;
      void    setStoreData(uint8_t);

      MemoryAccess(const WorkGroup *workGroup, const WorkItem *workItem,
                   bool store, bool atomic);
    };
    typedef std::vector<MemoryAccess> AccessList;
    typedef std::map< size_t, std::vector<AccessList> > AccessMap;

    AccessMap m_globalAccesses;
    std::map< size_t,std::mutex* > m_globalMutexes;

    struct LocalState
    {
      std::map<const Memory*, AccessMap> *map;
    };
    static THREAD_LOCAL LocalState m_localAccesses;

    bool m_allowUniformWrites;
    const KernelInvocation *m_kernelInvocation;

    Size3 getAccessWorkGroup(const MemoryAccess& access) const;

    bool check(const MemoryAccess& first, const MemoryAccess& second) const;
    void logRace(const Memory *memory, size_t address,
                 const MemoryAccess& first,
                 const MemoryAccess& second) const;
    void registerAccess(const Memory *memory,
                        const WorkGroup *workGroup,
                        const WorkItem *workItem,
                        size_t address, size_t size, bool atomic,
                        const uint8_t *storeData = NULL);
  };
}
