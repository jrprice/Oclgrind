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

#define DUMP_SHADOW

namespace oclgrind
{
    class ShadowValues
    {
        public:
            void dump() const;
            inline const llvm::CallInst* getCall() const
            {
                return m_call;
            }
            TypedValue getValue(const llvm::Value *V) const;
            inline void setCall(const llvm::CallInst *CI)
            {
                m_call = CI;
            }
            void setValue(const llvm::Value *V, TypedValue SV);

        private:
            const llvm::CallInst *m_call;
            TypedValueMap m_values;
#ifdef DUMP_SHADOW
            std::list<const llvm::Value*> m_valuesList;
#endif
    };

    class ShadowContext
    {
        public:
            ShadowContext(unsigned bufferBits);
            virtual ~ShadowContext();

            ShadowValues* createCleanShadowValues();
            void dump() const;
            void dumpGlobalValues() const;
            void dumpMemory() const;
            inline void dumpValues() const
            {
                m_values.top()->dump();
            }
            inline const llvm::CallInst* getCall() const
            {
                return m_values.top()->getCall();
            }
            static TypedValue getCleanValue(unsigned size);
            static TypedValue getCleanValue(const llvm::Type *Ty);
            static TypedValue getCleanValue(const llvm::Value *V);
            void* getMemoryPointer(size_t address) const;
            static TypedValue getPoisonedValue(unsigned size);
            static TypedValue getPoisonedValue(const llvm::Type *Ty);
            static TypedValue getPoisonedValue(const llvm::Value *V);
            TypedValue getGlobalValue(const llvm::Value *V) const;
            TypedValue getValue(const llvm::Value *V) const;
            void loadMemory(unsigned char *dst, size_t address, size_t size=1) const;
            inline void popValues()
            {
                m_values.pop();
            }
            inline void pushValues(ShadowValues *values)
            {
                m_values.push(values);
            }
            inline void setCall(const llvm::CallInst* CI)
            {
                m_values.top()->setCall(CI);
            }
            void setGlobalValue(const llvm::Value *V, TypedValue SV);
            inline void setValue(const llvm::Value *V, TypedValue SV)
            {
                m_values.top()->setValue(V, SV);
            }
            void storeMemory(const unsigned char *src, size_t address, size_t size=1);

        private:
#ifdef DUMP_SHADOW
            typedef std::unordered_map<size_t, std::pair<size_t, unsigned char*> > MemoryMap;
#else
            typedef std::unordered_map<size_t, unsigned char*> MemoryMap;
#endif
            typedef std::stack<ShadowValues*> ValuesStack;

            TypedValueMap m_globalValues;
            MemoryMap m_memory;
            unsigned m_numBitsAddress;
            unsigned m_numBitsBuffer;
            static MemoryPool m_pool;
            ValuesStack m_values;

            void allocateMemory(size_t address, size_t size);
            void clearMemory();
            size_t extractBuffer(size_t address) const;
            size_t extractOffset(size_t address) const;
    };

    class MemCheckUninitialized : public Plugin
    {
        public:
            MemCheckUninitialized(const Context *context);

            virtual void kernelBegin(const KernelInvocation *kernelInvocation) override;
            virtual void workItemBegin(const WorkItem *workItem) override;
            virtual void instructionExecuted(const WorkItem *workItem,
                    const llvm::Instruction *instruction,
                    const TypedValue& result) override;
            //virtual void memoryAllocated(const Memory *memory, size_t address,
            //                             size_t size, cl_mem_flags flags,
            //                             const uint8_t *initData);
        private:
            std::list<const llvm::Value*> m_deferredInit;
            mutable MemoryPool m_pool;
            ShadowContext shadowContext;

            void checkAllOperandsDefined(const llvm::Instruction *I);
            void copyShadowMemory(unsigned dstAddrSpace, size_t dst,
                                  unsigned srcAddrSpace, size_t src, size_t size);
            void handleIntrinsicInstruction(const WorkItem *workItem, const llvm::IntrinsicInst *I);

            void loadShadowMemory(unsigned addrSpace, size_t address, TypedValue &SM);
            void storeShadowMemory(unsigned addrSpace, size_t address, TypedValue SM);

            void SimpleOr(const llvm::Instruction *I);

            void logUninitializedWrite(unsigned int addrSpace, size_t address) const;
            void logUninitializedCF() const;
            void logUninitializedIndex() const;
            void logUninitializedMask() const;
    };
}
