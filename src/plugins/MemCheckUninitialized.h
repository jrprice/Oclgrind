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
            std::list<const llvm::Value*> m_valuesList;
    };

    class ShadowContext
    {
        public:
            ShadowContext(unsigned bufferBits);
            virtual ~ShadowContext();

            ShadowValues* createCleanShadowValues();
            void dump() const;
            void dumpMemory() const;
            inline void dumpValues() const
            {
                m_values.top()->dump();
            }
            inline const llvm::CallInst* getCall() const
            {
                return m_values.top()->getCall();
            }
            static TypedValue getCleanValue(const llvm::Type *Ty);
            static TypedValue getCleanValue(const llvm::Value *V);
            void* getMemoryPointer(size_t address) const;
            static TypedValue getPoisonedValue(const llvm::Type *Ty);
            static TypedValue getPoisonedValue(const llvm::Value *V);
            inline TypedValue getValue(const llvm::Value *V) const
            {
                return m_values.top()->getValue(V);
            }
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
            inline void setValue(const llvm::Value *V, TypedValue TV)
            {
                m_values.top()->setValue(V, TV);
            }
            void storeMemory(const unsigned char *src, size_t address, size_t size=1);

        private:
#define ALLOW_DUMP
#ifdef ALLOW_DUMP
            typedef std::map<size_t, std::pair<size_t, unsigned char*> > MemoryMap;
#else
            typedef std::map<size_t, unsigned char*> MemoryMap;
#endif
            typedef std::stack<ShadowValues*> ValuesStack;

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
            virtual void instructionExecuted(const WorkItem *workItem,
                    const llvm::Instruction *instruction,
                    const TypedValue& result) override;
            //virtual void memoryAllocated(const Memory *memory, size_t address,
            //                             size_t size, cl_mem_flags flags,
            //                             const uint8_t *initData);
        private:
            mutable MemoryPool m_pool;
            ShadowContext ShadowContext;

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
