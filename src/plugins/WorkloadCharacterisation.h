// WorkloadCharacterisation.h (Oclgrind)
// Copyright (c) 2017, Beau Johnston,
// The Australian National University. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/Plugin.h"

#include <mutex>

namespace llvm
{
  class Function;
}

namespace oclgrind
{
  class WorkloadCharacterisation : public Plugin
  {
  public:
    WorkloadCharacterisation(const Context *context);
    ~WorkloadCharacterisation();

    virtual void hostMemoryLoad(const Memory *memory,size_t address, size_t size) override;
    virtual void hostMemoryStore(const Memory *memory, size_t address, size_t size,const uint8_t *storeData) override;
    virtual void instructionExecuted(const WorkItem *workItem,
                                     const llvm::Instruction *instruction,
                                     const TypedValue& result) override;
    virtual void memoryLoad(const Memory *memory, const WorkItem *workItem,size_t address, size_t size) override;
    virtual void memoryStore(const Memory *memory, const WorkItem *workItem,size_t address, size_t size,  const uint8_t *storeData) override;
    virtual void memoryAtomicLoad(const Memory *memory, const WorkItem *workItem, AtomicOp op, size_t address, size_t size) override;
    virtual void memoryAtomicStore(const Memory *memory, const WorkItem *workItem, AtomicOp op, size_t address, size_t size) override;
    virtual void kernelBegin(const KernelInvocation *kernelInvocation) override;
    virtual void kernelEnd(const KernelInvocation *kernelInvocation) override;
    virtual void workGroupBegin(const WorkGroup *workGroup) override;
    virtual void workGroupComplete(const WorkGroup *workGroup) override;
    virtual void workItemBegin(const WorkItem *workItem) override;
    virtual void workItemComplete(const WorkItem *workItem) override;
    virtual void workItemBarrier(const WorkItem *workItem) override;
    virtual void workItemClearBarrier(const WorkItem *workItem) override;

  private:
    std::vector<std::pair<size_t,size_t>> m_memoryOps;
    std::unordered_map<std::string,size_t> m_computeOps;
    std::unordered_map<unsigned,std::vector<bool>> m_branchOps;
    std::vector<float> m_instructionsToBarrier;
    std::vector<size_t> m_instructionWidth;
    std::vector<unsigned int> m_instructionsPerWorkitem;
    unsigned m_threads_invoked;
    unsigned m_barriers_hit;
    std::vector<std::string> m_hostToDeviceCopy;
    std::vector<std::string> m_deviceToHostCopy;
    std::string m_last_kernel_name;
    std::vector<unsigned> m_instructionsBetweenLoadOrStore;
    std::unordered_map<std::string,size_t> m_loadInstructionLabels;
    std::unordered_map<std::string,size_t> m_storeInstructionLabels;

    struct WorkerState
    {
      std::unordered_map<std::string,size_t> *computeOps;
      std::vector<std::pair<size_t,size_t>> *memoryOps;
      bool previous_instruction_is_branch;
      std::string target1, target2;
      unsigned branch_loc;
      std::unordered_map<unsigned,std::vector<bool>> *branchOps;
      unsigned threads_invoked;
      unsigned barriers_hit;
      unsigned instruction_count;
      unsigned ops_between_load_or_store;
      unsigned workitem_instruction_count;
      std::vector<unsigned> *instructionsBetweenBarriers;
      std::vector<unsigned> *instructionsPerWorkitem;
      std::vector<unsigned> *instructionWidth;
      std::unordered_map<std::string,size_t> *loadInstructionLabels;
      std::unordered_map<std::string,size_t> *storeInstructionLabels;
      std::vector<unsigned> *instructionsBetweenLoadOrStore;
    };
    static THREAD_LOCAL WorkerState m_state;

    std::mutex m_mtx;

  };
}
