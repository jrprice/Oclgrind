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
                                     const TypedValue& result);
    virtual void kernelBegin(KernelInvocation *kernelInvocation);
    virtual void kernelEnd(KernelInvocation *kernelInvocation);
  private:
    std::vector<size_t> m_instructionCounts;
    std::vector<size_t> m_memopBytes;
    std::vector<const llvm::Function*> m_functions;

    std::string getOpcodeName(unsigned opcode) const;
  };
}
