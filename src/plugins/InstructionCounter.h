// InstructionCounter.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
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
  class InstructionCounter : public Plugin
  {
  public:
    InstructionCounter(const Context *context) : Plugin(context){};

    virtual void instructionExecuted(const WorkItem *workItem,
                                     const llvm::Instruction *instruction,
                                     const TypedValue& result) override;
    virtual void kernelBegin(const KernelInvocation *kernelInvocation) override;
    virtual void kernelEnd(const KernelInvocation *kernelInvocation) override;
    virtual void workGroupBegin(const WorkGroup *workGroup) override;
    virtual void workGroupComplete(const WorkGroup *workGroup) override;

  private:
    std::vector<size_t> m_instructionCounts;
    std::vector<size_t> m_memopBytes;
    std::vector<const llvm::Function*> m_functions;

    struct WorkerState
    {
      std::vector<size_t> *instCounts;
      std::vector<size_t> *memopBytes;
      std::vector<const llvm::Function*> *functions;
    };
    static THREAD_LOCAL WorkerState m_state;

    std::mutex m_mtx;

    std::string getOpcodeName(unsigned opcode) const;
  };
}
