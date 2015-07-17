// MemCheckUninitialized.h (Oclgrind)
// Copyright (c) 2015, Moritz Pflanzer
// Imperial College London. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/Plugin.h"
#include "llvm/IR/ValueMap.h"

namespace oclgrind
{
  class MemCheckUninitialized : public Plugin
  {
  public:
      typedef std::map<const llvm::Value*, size_t> ValueAddressMap;

      MemCheckUninitialized(const Context *context);

      virtual void instructionExecuted(const WorkItem *workItem,
                                       const llvm::Instruction *instruction,
                                       const TypedValue& result) override;

  private:
    llvm::LLVMContext *llvmContext;
    ValueAddressMap AddressMap;
    Memory *ShadowMem;
    mutable MemoryPool m_pool;
    TypedValueMap ShadowMap;

    TypedValue getCleanShadow(const llvm::Value *V);
    llvm::Constant *getPoisonedShadow(llvm::Type *ShadowTy);
    TypedValue getPoisonedShadow(const llvm::Value *V);
    TypedValue getShadow(const llvm::Value *V);
    llvm::Type *getShadowTy(const llvm::Value *V);
    llvm::Type *getShadowTy(llvm::Type *OrigTy);
    void setShadow(const llvm::Value *V, TypedValue SV);

    void setShadowMem(size_t address, TypedValue SM);
    bool isCleanShadowMem(size_t address, unsigned size);

    void dumpShadowMap();
    void dumpShadowMem();
    void logError(unsigned int addrSpace, size_t address) const;
  };
}
