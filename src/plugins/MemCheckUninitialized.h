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

    class ShadowMemory
    {
        public:
            ShadowMemory(AddressSpace addrSpace, unsigned bufferBits);
            virtual ~ShadowMemory();

            void dump() const;
            void* getPointer(size_t address) const;
            void load(unsigned char *dst, size_t address, size_t size=1) const;
            void store(const unsigned char *src, size_t address, size_t size=1);

        private:
#ifdef DUMP_SHADOW
            typedef std::unordered_map<size_t, std::pair<size_t, unsigned char*> > MemoryMap;
#else
            typedef std::unordered_map<size_t, unsigned char*> MemoryMap;
#endif

            AddressSpace m_addrSpace;
            MemoryMap m_map;
            unsigned m_numBitsAddress;
            unsigned m_numBitsBuffer;

            void allocate(size_t address, size_t size);
            void clear();
            size_t extractBuffer(size_t address) const;
            size_t extractOffset(size_t address) const;
    };

    class ShadowWorkItem
    {
        public:
            ShadowWorkItem(unsigned bufferBits);
            virtual ~ShadowWorkItem();

            ShadowValues* createCleanShadowValues();
            void dump() const;
            inline void dumpMemory() const
            {
                m_memory.dump();
            }
            inline void dumpValues() const
            {
                m_values.top()->dump();
            }
            inline const llvm::CallInst* getCall() const
            {
                return m_values.top()->getCall();
            }
            inline void* getMemoryPointer(size_t address) const
            {
                return m_memory.getPointer(address);
            }
            inline TypedValue getValue(const llvm::Value *V) const
            {
                return m_values.top()->getValue(V);
            }
            inline bool hasValue(const llvm::Value* V) const
            {
                return m_values.top()->hasValue(V);
            }
            inline void loadMemory(unsigned char *dst, size_t address, size_t size=1) const
            {
                m_memory.load(dst, address, size);
            }
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
            inline void storeMemory(const unsigned char *src, size_t address, size_t size=1)
            {
                m_memory.store(src, address, size);
            }

        private:
            typedef std::stack<ShadowValues*> ValuesStack;

            ShadowMemory m_memory;
            ValuesStack m_values;
    };

    class ShadowWorkGroup
    {
        public:
            ShadowWorkGroup(unsigned bufferBits);

            inline void dump() const
            {
                m_memory.dump();
            }
            inline void dumpMemory() const
            {
                m_memory.dump();
            }
            inline void* getMemoryPointer(size_t address) const
            {
                return m_memory.getPointer(address);
            }
            inline void loadMemory(unsigned char *dst, size_t address, size_t size=1) const
            {
                m_memory.load(dst, address, size);
            }
            inline void storeMemory(const unsigned char *src, size_t address, size_t size=1)
            {
                m_memory.store(src, address, size);
            }

        private:
            ShadowMemory m_memory;
    };

    class ShadowContext
    {
        public:
            ShadowContext(unsigned bufferBits);

            void allocateWorkItems();
            void allocateWorkGroups();
            ShadowWorkItem* createShadowWorkItem(const WorkItem *workItem);
            ShadowWorkGroup* createShadowWorkGroup(const WorkGroup *workGroup);
            void destroyShadowWorkItem(const WorkItem *workItem);
            void destroyShadowWorkGroup(const WorkGroup *workGroup);
            void dump(const WorkItem *workItem) const;
            void dumpGlobalValues() const;
            void freeWorkItems();
            void freeWorkGroups();
            static TypedValue getCleanValue(unsigned size);
            static TypedValue getCleanValue(const llvm::Type *Ty);
            static TypedValue getCleanValue(const llvm::Value *V);
            TypedValue getGlobalValue(const llvm::Value *V) const;
            static TypedValue getPoisonedValue(unsigned size);
            static TypedValue getPoisonedValue(const llvm::Type *Ty);
            static TypedValue getPoisonedValue(const llvm::Value *V);
            inline ShadowWorkItem* getShadowWorkItem(const WorkItem *workItem) const
            {
                return m_workSpace.workItems->at(workItem);
            }
            inline ShadowWorkGroup* getShadowWorkGroup(const WorkGroup *workGroup) const
            {
                return m_workSpace.workGroups->at(workGroup);
            }
            TypedValue getValue(const WorkItem *workItem, const llvm::Value *V) const;
            inline bool hasValue(const WorkItem *workItem, const llvm::Value* V) const
            {
                return m_globalValues.count(V) || m_workSpace.workItems->at(workItem)->hasValue(V);
            }
            void setGlobalValue(const llvm::Value *V, TypedValue SV);

        private:
            TypedValueMap m_globalValues;
            unsigned m_numBitsBuffer;
            static MemoryPool m_pool;
            typedef std::map<const WorkItem*, ShadowWorkItem*> ShadowItemMap;
            typedef std::map<const WorkGroup*, ShadowWorkGroup*> ShadowGroupMap;
            struct WorkItems
            {
                ShadowItemMap *workItems;
                ShadowGroupMap *workGroups;
            };
            static THREAD_LOCAL WorkItems m_workSpace;
    };

    class MemCheckUninitialized : public Plugin
    {
        public:
            MemCheckUninitialized(const Context *context);

            virtual void kernelBegin(const KernelInvocation *kernelInvocation) override;
            virtual void workItemBegin(const WorkItem *workItem) override;
            virtual void workItemComplete(const WorkItem *workItem);
            virtual void workGroupBegin(const WorkGroup *workGroup);
            virtual void workGroupComplete(const WorkGroup *workGroup);
            virtual void instructionExecuted(const WorkItem *workItem,
                    const llvm::Instruction *instruction,
                    const TypedValue& result) override;
            //virtual void memoryAllocated(const Memory *memory, size_t address,
            //                             size_t size, cl_mem_flags flags,
            //                             const uint8_t *initData);
        private:
            std::list<std::pair<const llvm::Value*, TypedValue> > m_deferredInit;
            std::list<std::pair<const llvm::Value*, TypedValue> > m_deferredInitGroup;
            ShadowContext shadowContext;

            void checkAllOperandsDefined(const WorkItem *workItem, const llvm::Instruction *I);
            void copyShadowMemory(const WorkItem *workItem,
                                  unsigned dstAddrSpace, size_t dst,
                                  unsigned srcAddrSpace, size_t src, size_t size);
            void copyShadowMemory(const WorkGroup *workGroup,
                                  unsigned dstAddrSpace, size_t dst,
                                  unsigned srcAddrSpace, size_t src, size_t size);
            void handleIntrinsicInstruction(const WorkItem *workItem, const llvm::IntrinsicInst *I);

            void loadShadowMemory(unsigned addrSpace, size_t address, TypedValue &SM,
                                  const WorkItem *workItem = NULL, const WorkGroup *workGroup = NULL);
            void storeShadowMemory(unsigned addrSpace, size_t address, TypedValue SM,
                                   const WorkItem *workItem = NULL, const WorkGroup *workGroup = NULL);

            void SimpleOr(const WorkItem *workItem, const llvm::Instruction *I);

            void logUninitializedWrite(unsigned int addrSpace, size_t address) const;
            void logUninitializedCF() const;
            void logUninitializedIndex() const;
            void logUninitializedMask() const;
    };
}
