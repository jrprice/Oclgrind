// WorkloadCharacterisation.h (Oclgrind)
// Copyright (c) 2017, Beau Johnston,
// The Australian National University. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/Plugin.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace llvm {
class Function;
}

namespace oclgrind {
class WorkloadCharacterisation : public Plugin {
public:
  WorkloadCharacterisation(const Context *context);
  ~WorkloadCharacterisation();

  virtual void threadMemoryLedger(size_t address, uint32_t timestep, Size3 localID);
  virtual void hostMemoryLoad(const Memory *memory, size_t address, size_t size) override;
  virtual void hostMemoryStore(const Memory *memory, size_t address, size_t size, const uint8_t *storeData) override;
  virtual void instructionExecuted(const WorkItem *workItem,
                                   const llvm::Instruction *instruction,
                                   const TypedValue &result) override;
  virtual void memoryLoad(const Memory *memory, const WorkItem *workItem, size_t address, size_t size) override;
  virtual void memoryStore(const Memory *memory, const WorkItem *workItem, size_t address, size_t size, const uint8_t *storeData) override;
  virtual void memoryAtomicLoad(const Memory *memory, const WorkItem *workItem, AtomicOp op, size_t address, size_t size) override;
  virtual void memoryAtomicStore(const Memory *memory, const WorkItem *workItem, AtomicOp op, size_t address, size_t size) override;
  virtual void kernelBegin(const KernelInvocation *kernelInvocation) override;
  virtual void kernelEnd(const KernelInvocation *kernelInvocation) override;
  virtual void workGroupBegin(const WorkGroup *workGroup) override;
  virtual void workGroupComplete(const WorkGroup *workGroup) override;
  virtual void workGroupBarrier(const WorkGroup *workGroup, uint32_t flags) override;
  virtual void workItemBegin(const WorkItem *workItem) override;
  virtual void workItemComplete(const WorkItem *workItem) override;
  virtual void workItemBarrier(const WorkItem *workItem) override;
  virtual void workItemClearBarrier(const WorkItem *workItem) override;

  struct ledgerElement {
    size_t address;
    uint32_t timestep;
  };
private:
  const KernelInvocation *m_kernelInvocation;

  // std::unordered_map<std::pair<size_t, bool>, uint32_t> m_memoryOps;
  std::unordered_map<size_t, uint32_t> m_storeOps;
  std::unordered_map<size_t, uint32_t> m_loadOps;
  std::unordered_map<std::string, size_t> m_computeOps;
  std::unordered_map<const llvm::Instruction *, std::unordered_map<uint16_t, uint32_t>> m_branchPatterns;
  std::unordered_map<const llvm::Instruction *, uint32_t> m_branchCounts;
  std::vector<uint32_t> m_instructionsToBarrier;
  std::unordered_map<uint16_t, size_t> m_instructionWidth;
  std::vector<uint32_t> m_instructionsPerWorkitem;
  uint32_t m_threads_invoked;
  uint32_t m_barriers_hit;
  int m_numberOfHostToDeviceCopiesBeforeKernelNamed;
  std::vector<std::string> m_hostToDeviceCopy;
  std::vector<std::string> m_deviceToHostCopy;
  std::string m_last_kernel_name;
  std::vector<uint32_t> m_instructionsBetweenLoadOrStore;
  std::unordered_map<std::string, size_t> m_loadInstructionLabels;
  std::unordered_map<std::string, size_t> m_storeInstructionLabels;
  uint32_t m_global_memory_access;
  uint32_t m_constant_memory_access;
  uint32_t m_local_memory_access;
  Size3 m_group_num;
  Size3 m_local_num;
  std::vector<std::vector<double>> m_psl_per_group;

  struct WorkerState {
    std::unordered_map<std::string, size_t> *computeOps;
    //std::unordered_map<std::pair<size_t, bool>, uint32_t> *memoryOps;
    std::unordered_map<size_t, uint32_t> *storeOps;
    std::unordered_map<size_t, uint32_t> *loadOps;
    // true -> load; false -> store.

    bool previous_instruction_is_branch;
    llvm::BasicBlock* target1;
    llvm::BasicBlock* target2;
    const llvm::Instruction* branch_loc;
    std::unordered_map<const llvm::Instruction*, std::vector<bool>> *branchOps;

    uint32_t threads_invoked;
    uint32_t barriers_hit;
    uint32_t instruction_count;
    uint32_t ops_between_load_or_store;
    uint32_t workitem_instruction_count;
    uint32_t global_memory_access_count;
    uint32_t constant_memory_access_count;
    uint32_t local_memory_access_count;
    std::vector<uint32_t> *instructionsBetweenBarriers;
    std::vector<uint32_t> *instructionsPerWorkitem;
    std::unordered_map<uint16_t, size_t> *instructionWidth;
    std::unordered_map<std::string, size_t> *loadInstructionLabels;
    std::unordered_map<std::string, size_t> *storeInstructionLabels;
    std::vector<uint32_t> *instructionsBetweenLoadOrStore;
    std::vector<std::pair<std::vector<double>, uint64_t>> *psl_per_barrier;
    uint64_t timestep;
    // uint32_t work_item_no;
    // uint32_t work_group_no;
    std::vector<std::vector<ledgerElement>> ledger;
  };
  static THREAD_LOCAL WorkerState m_state;

  std::mutex m_mtx;
};
} // namespace oclgrind
