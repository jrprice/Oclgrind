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
    class ShadowFrame
    {
#define ALLOW_DUMP
        public:
            ShadowFrame(unsigned bufferBits);
            virtual ~ShadowFrame();

            void dump() const;
            void dumpMemory() const;
            void dumpValues() const;
            void* getMemoryPointer(size_t address) const;
            TypedValue getValue(const llvm::Value *V) const;
            void loadMemory(unsigned char *dst, size_t address, size_t size=1) const;
            inline const llvm::CallInst* popCall()
            {
                const llvm::CallInst *call = m_callInstructions.top();
                m_callInstructions.pop();
                return call;
            }
            inline void pushCall(const llvm::CallInst *CI)
            {
                m_callInstructions.push(CI);
            }
            void setValue(const llvm::Value *V, TypedValue SV);
            void storeMemory(const unsigned char *src, size_t address, size_t size=1);

        private:
#ifdef ALLOW_DUMP
            typedef std::map<size_t, std::pair<size_t, unsigned char*> > MemoryMap;
#else
            typedef std::map<size_t, unsigned char*> MemoryMap;
#endif

            void allocateMemory(size_t address, size_t size);
            void clearMemory();
            size_t extractBuffer(size_t address) const;
            size_t extractOffset(size_t address) const;

            std::stack<const llvm::CallInst*> m_callInstructions;
            MemoryMap m_memory;
            unsigned m_numBitsAddress;
            unsigned m_numBitsBuffer;
            TypedValueMap m_values;
            std::list<const llvm::Value*> m_valuesList;
    };

    class ShadowContext
    {
        public:
            ShadowContext(unsigned bufferBits);
            virtual ~ShadowContext();

            ShadowFrame* createShadowFrame();
            inline void dump() const
            {
                m_stack.top()->dump();
            }
            inline void dumpMemory() const
            {
                m_stack.top()->dumpMemory();
            }
            inline void dumpValues() const
            {
                m_stack.top()->dumpValues();
            }
            static TypedValue getCleanValue(const llvm::Type *Ty);
            static TypedValue getCleanValue(const llvm::Value *V);
            inline void* getMemoryPointer(size_t address) const
            {
                return m_stack.top()->getMemoryPointer(address);
            }
            static TypedValue getPoisonedValue(const llvm::Type *Ty);
            static TypedValue getPoisonedValue(const llvm::Value *V);
            inline void* getMemoryPointer(size_t address)
            {
                return m_stack.top()->getMemoryPointer(address);
            }
            inline TypedValue getValue(const llvm::Value *V) const
            {
                return m_stack.top()->getValue(V);
            }
            inline void loadMemory(unsigned char *dst, size_t address, size_t size=1) const
            {
                m_stack.top()->loadMemory(dst, address, size);
            }
            inline void pop()
            {
                m_stack.pop();
            }
            inline const llvm::CallInst* popCall()
            {
                return m_stack.top()->popCall();
            }
            inline void push(ShadowFrame *frame)
            {
                m_stack.push(frame);
            }
            inline void pushCall(const llvm::CallInst *CI)
            {
                m_stack.top()->pushCall(CI);
            }
            inline void setValue(const llvm::Value *V, TypedValue TV)
            {
                m_stack.top()->setValue(V, TV);
            }
            inline void storeMemory(const unsigned char *src, size_t address, size_t size=1)
            {
                m_stack.top()->storeMemory(src, address, size);
            }

        private:
            typedef std::stack<ShadowFrame*> ShadowStack;

            unsigned m_numBitsBuffer;
            static MemoryPool m_pool;
            ShadowStack m_stack;
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
