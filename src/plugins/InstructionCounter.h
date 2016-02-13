// InstructionCounter.h (Oclgrind)
// Copyright (c) 2013-2016, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/Plugin.h"

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

    virtual bool isThreadSafe() const override;

  private:
    std::vector<size_t> m_instructionCounts;
    std::vector<size_t> m_memopBytes;
    std::vector<const llvm::Function*> m_functions;

    std::string getOpcodeName(unsigned opcode) const;
  };
}
