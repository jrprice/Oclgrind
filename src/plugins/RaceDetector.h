// RaceDetector.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/Plugin.h"

#include <mutex>

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
    virtual void memoryDeallocated(const Memory *memory,
                                   size_t address) override;
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
    virtual void workGroupBegin(const WorkGroup *workGroup) override;
    virtual void workGroupComplete(const WorkGroup *workGroup) override;

  private:
    struct MemoryAccess
    {
    private:
      size_t entity;
      const llvm::Instruction *instruction;

      uint8_t info;
      static const unsigned SET_BIT     = 0;
      static const unsigned STORE_BIT   = 1;
      static const unsigned ATOMIC_BIT  = 2;
      static const unsigned WG_BIT      = 3;
      uint8_t storeData;

    public:
      void clear();

      bool isSet() const;

      bool isAtomic() const;
      bool isLoad() const;
      bool isStore() const;
      bool isWorkGroup() const;
      bool isWorkItem() const;

      size_t getEntity() const;
      const llvm::Instruction* getInstruction() const;

      uint8_t getStoreData() const;
      void    setStoreData(uint8_t);

      MemoryAccess();
      MemoryAccess(const WorkGroup *workGroup, const WorkItem *workItem,
                   bool store, bool atomic);

      bool operator==(const MemoryAccess& other) const;
    };
    struct AccessRecord
    {
      MemoryAccess load;
      MemoryAccess store;
    };
    typedef std::vector<MemoryAccess> AccessList;
    typedef std::unordered_map<
      size_t,AccessRecord,
      std::hash<size_t>,
      std::equal_to<size_t>,
      PoolAllocator<std::pair<const size_t,AccessRecord>,8192>
      > AccessMap;

    std::unordered_map<size_t,std::vector<AccessRecord>> m_globalAccesses;
    std::map< size_t,std::mutex* > m_globalMutexes;

    struct WorkGroupState
    {
      size_t numWorkItems;
      std::vector<AccessMap> wiLocal;
      std::vector<AccessMap> wiGlobal;
      AccessMap wgGlobal;
    };
    struct WorkerState
    {
      std::unordered_map<const WorkGroup*,WorkGroupState> *groups;
    };
    static THREAD_LOCAL WorkerState m_state;

    struct Race
    {
      unsigned addrspace;
      size_t address;
      MemoryAccess a, b;
    };
    typedef std::list<Race> RaceList;

    bool m_allowUniformWrites;
    const KernelInvocation *m_kernelInvocation;

    std::mutex kernelRacesMutex;
    RaceList kernelRaces;

    size_t getAccessWorkGroup(const MemoryAccess& access) const;

    bool check(const MemoryAccess& a, const MemoryAccess& b) const;
    void insert(AccessRecord& record, const MemoryAccess& access) const;
    void insertKernelRace(const Race& race);
    void insertRace(RaceList& races, const Race& race) const;
    void logRace(const Race& race) const;
    void registerAccess(const Memory *memory,
                        const WorkGroup *workGroup,
                        const WorkItem *workItem,
                        size_t address, size_t size, bool atomic,
                        const uint8_t *storeData = NULL);
    void syncWorkItems(const Memory *memory,
                       WorkGroupState& state,
                       std::vector<AccessMap>& accesses);
  };
}
