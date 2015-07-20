// MemCheckUninitialized.h (Oclgrind)
// Copyright (c) 2015, Moritz Pflanzer
// Imperial College London. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/Plugin.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IntrinsicInst.h"

namespace oclgrind
{
  class MemCheckUninitialized : public Plugin
  {
  public:
      MemCheckUninitialized(const Context *context);

      virtual void kernelBegin(const KernelInvocation *kernelInvocation) override;
      virtual void instructionExecuted(const WorkItem *workItem,
                                       const llvm::Instruction *instruction,
                                       const TypedValue& result) override;
      //virtual void memoryAllocated(const Memory *memory, size_t address,
      //                             size_t size, cl_mem_flags flags,
      //                             const uint8_t *initData);
  private:
    std::map<unsigned, Memory*> ShadowMem;
    mutable MemoryPool m_pool;
    TypedValueMap ShadowMap;
    std::list<const llvm::Value*> ShadowList;
    std::vector<const llvm::CallInst*> CallInstructions;

    TypedValue getCleanShadow(const llvm::Value *V);
    TypedValue getCleanShadow(llvm::Type *Ty);
    TypedValue getPoisonedShadow(const llvm::Value *V);
    TypedValue getPoisonedShadow(llvm::Type *Ty);
    TypedValue getShadow(const llvm::Value *V);
    void setShadow(const llvm::Value *V, TypedValue SV);

    void checkAllOperandsDefined(const llvm::Instruction *I);
    void handleIntrinsicInstruction(const WorkItem *workItem, const llvm::IntrinsicInst *I);

    Memory *getMemory(unsigned addrSpace);

    void setShadowMem(unsigned addrSpace, size_t address, TypedValue SM);
    void getShadowMem(unsigned addrSpace, size_t address, TypedValue &SM);

    void SimpleOr(const llvm::Instruction *I);

    void dumpShadowMap();
    void dumpShadowMem(unsigned addrSpace);
    void logUninitializedWrite(unsigned int addrSpace, size_t address) const;
    void logUninitializedCF() const;
    void logUninitializedIndex() const;
    void logUninitializedMask() const;
  };
}
