#pragma once

#include "core/common.h"

namespace oclgrind
{
  class Kernel;

  class Plugin
  {
  public:
    virtual void instructionExecuted(const llvm::Instruction *instruction,
                                     const TypedValue& result){};
    virtual void kernelBegin(const Kernel *kernel){};
    virtual void kernelEnd(const Kernel *kernel){};
  };
}
