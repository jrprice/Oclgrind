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

//#define DUMP_SHADOW

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
            inline bool hasValue(const llvm::Value* V) const
            {
                return m_values.count(V);
            }
            void loadMemory(unsigned char *dst, size_t address, size_t size=1) const;
            void setValue(const llvm::Value *V, TypedValue SV);

        private:
            const llvm::CallInst *m_call;
            TypedValueMap m_values;
#ifdef DUMP_SHADOW
            std::list<const llvm::Value*> m_valuesList;
#endif
    };

    class ShadowWorkItem
    {
        public:
            ShadowWorkItem(unsigned bufferBits);
            virtual ~ShadowWorkItem();

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
            void* getMemoryPointer(size_t address) const;
            inline TypedValue getValue(const llvm::Value *V) const
            {
                return m_values.top()->getValue(V);
            }
            inline bool hasValue(const llvm::Value* V) const
            {
                return m_values.top()->hasValue(V);
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

            MemoryMap m_memory;
            unsigned m_numBitsAddress;
            unsigned m_numBitsBuffer;
            ValuesStack m_values;

            void allocateMemory(size_t address, size_t size);
            void clearMemory();
            size_t extractBuffer(size_t address) const;
            size_t extractOffset(size_t address) const;
    };

    class ShadowContext
    {
        public:
            ShadowContext(unsigned bufferBits);

            ShadowWorkItem* createShadowWorkItem(const WorkItem *workItem);
            void destroyShadowWorkItem(const WorkItem *workItem);
            void dump() const;
            void dumpGlobalValues() const;
            static TypedValue getCleanValue(unsigned size);
            static TypedValue getCleanValue(const llvm::Type *Ty);
            static TypedValue getCleanValue(const llvm::Value *V);
            TypedValue getGlobalValue(const llvm::Value *V) const;
            static TypedValue getPoisonedValue(unsigned size);
            static TypedValue getPoisonedValue(const llvm::Type *Ty);
            static TypedValue getPoisonedValue(const llvm::Value *V);
            inline ShadowWorkItem* getShadowWorkItem(const WorkItem *workItem) const
            {
                std::lock_guard<std::mutex> lock(m_workItems_mutex);
                return m_workItems.at(workItem);
            }
            TypedValue getValue(const WorkItem *workItem, const llvm::Value *V) const;
            inline bool hasValue(const WorkItem *workItem, const llvm::Value* V) const
            {
                std::lock_guard<std::mutex> lock(m_workItems_mutex);
                return m_globalValues.count(V) || m_workItems.at(workItem)->hasValue(V);
            }
            void setGlobalValue(const llvm::Value *V, TypedValue SV);

        private:
            TypedValueMap m_globalValues;
            unsigned m_numBitsBuffer;
            static MemoryPool m_pool;
            std::unordered_map<const WorkItem*, ShadowWorkItem*> m_workItems;
            static std::mutex m_workItems_mutex;
    };

    class MemCheckUninitialized : public Plugin
    {
        public:
            MemCheckUninitialized(const Context *context);

            virtual void kernelBegin(const KernelInvocation *kernelInvocation) override;
            virtual void workItemBegin(const WorkItem *workItem) override;
            virtual void workItemComplete(const WorkItem *workItem);
            virtual void instructionExecuted(const WorkItem *workItem,
                    const llvm::Instruction *instruction,
                    const TypedValue& result) override;
            //virtual void memoryAllocated(const Memory *memory, size_t address,
            //                             size_t size, cl_mem_flags flags,
            //                             const uint8_t *initData);
        private:
            std::list<const llvm::Value*> m_deferredInit;
            ShadowContext shadowContext;

            void checkAllOperandsDefined(const WorkItem *workItem, const llvm::Instruction *I);
            void copyShadowMemory(const WorkItem *workItem,
                                  unsigned dstAddrSpace, size_t dst,
                                  unsigned srcAddrSpace, size_t src, size_t size);
            void handleIntrinsicInstruction(const WorkItem *workItem, const llvm::IntrinsicInst *I);

            void loadShadowMemory(const WorkItem *workItem,
                                  unsigned addrSpace, size_t address, TypedValue &SM);
            void storeShadowMemory(const WorkItem *workItem,
                                   unsigned addrSpace, size_t address, TypedValue SM);

            void SimpleOr(const WorkItem *workItem, const llvm::Instruction *I);

            void logUninitializedWrite(unsigned int addrSpace, size_t address) const;
            void logUninitializedCF() const;
            void logUninitializedIndex() const;
            void logUninitializedMask() const;
    };
}
