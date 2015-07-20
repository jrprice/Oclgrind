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
    class ShadowMemory
    {
#define ALLOW_DUMP
        public:
            ShadowMemory(unsigned bufferBits);
            virtual ~ShadowMemory();
            void dump() const;
            void* getPointer(size_t address) const;
            void load(unsigned char *dst, size_t address, size_t size=1) const;
            void store(const unsigned char *src, size_t address, size_t size=1);

        private:
#ifdef ALLOW_DUMP
            typedef std::map<size_t, std::pair<size_t, unsigned char*> > MemoryMap;
#else
            typedef std::map<size_t, unsigned char*> MemoryMap;
#endif

            mutable MemoryMap m_memory;
            unsigned m_numBitsBuffer;
            unsigned m_numBitsAddress;

            void allocate(size_t address, size_t size);
            size_t extractBuffer(size_t address) const;
            size_t extractOffset(size_t address) const;
    };

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
            std::map<unsigned, ShadowMemory*> ShadowMem;
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

            ShadowMemory *getShadowMemory(unsigned addrSpace);

            void storeShadowMemory(unsigned addrSpace, size_t address, TypedValue SM);
            void loadShadowMemory(unsigned addrSpace, size_t address, TypedValue &SM);

            void SimpleOr(const llvm::Instruction *I);

            void dumpShadowMap();
            void dumpShadowMem(unsigned addrSpace);
            void logUninitializedWrite(unsigned int addrSpace, size_t address) const;
            void logUninitializedCF() const;
            void logUninitializedIndex() const;
            void logUninitializedMask() const;
    };
}
