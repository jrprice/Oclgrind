// MemCheckUninitialized.h (Oclgrind)
// Copyright (c) 2015, Moritz Pflanzer
// Imperial College London. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/common.h"

#include "core/Context.h"
#include "core/Memory.h"
#include "core/WorkItem.h"
#include "core/WorkGroup.h"
#include "core/Kernel.h"
#include "core/KernelInvocation.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Type.h"

#include "MemCheckUninitialized.h"
#include <mutex>

using namespace oclgrind;
using namespace std;

//void MemCheckUninitialized::memoryAllocated(const Memory *memory, size_t address,
//                                 size_t size, cl_mem_flags flags,
//                                 const uint8_t *initData)
//{
//    cout << "Memory: " << memory << ", address: " << hex << address << dec << ", size: " << size << endl;
//}

// Multiple mutexes to mitigate risk of unnecessary synchronisation in atomics
#define NUM_ATOMIC_MUTEXES 64 // Must be power of two
static std::mutex atomicShadowMutex[NUM_ATOMIC_MUTEXES];
#define ATOMIC_MUTEX(offset) \
  atomicShadowMutex[(((offset)>>2) & (NUM_ATOMIC_MUTEXES-1))]

THREAD_LOCAL ShadowContext::WorkSpace ShadowContext::m_workSpace = {NULL, NULL, NULL, 0};

MemCheckUninitialized::MemCheckUninitialized(const Context *context)
 : Plugin(context), shadowContext(sizeof(size_t)==8 ? 32 : 16)
{
    shadowContext.createMemoryPool();
}

MemCheckUninitialized::~MemCheckUninitialized()
{
    shadowContext.destroyMemoryPool();
}

void MemCheckUninitialized::kernelBegin(const KernelInvocation *kernelInvocation)
{
    const Kernel *kernel = kernelInvocation->getKernel();

    // Initialise kernel arguments and global variables
    for (auto value = kernel->values_begin(); value != kernel->values_end(); value++)
    {
        const llvm::Type *type = value->first->getType();

        if(!type->isSized())
        {
            continue;
        }

        if(type->isPointerTy())
        {
            switch(type->getPointerAddressSpace())
            {
                case AddrSpaceConstant:
                {
                    // Constants
                    // value->second.data == ptr
                    // value->second.size == ptr size
                    TypedValue cleanValue = m_pool.clone(ShadowContext::getCleanValue(value->first));
                    shadowContext.setGlobalValue(value->first, cleanValue);
                    const llvm::Type *elementTy = type->getPointerElementType();
                    allocAndStoreShadowMemory(AddrSpaceConstant, value->second.getPointer(), ShadowContext::getCleanValue(elementTy));
                    break;
                }
                case AddrSpaceGlobal:
                {
                    // Global pointer kernel arguments
                    // value->second.data == ptr
                    // value->second.size == ptr size
                    size_t address = value->second.getPointer();

                    if(m_context->getGlobalMemory()->isAddressValid(address) && !shadowContext.getGlobalMemory()->isAddressValid(address))
                    {
                        // Allocate poisoned global memory if there was no host store
                        size_t size = m_context->getGlobalMemory()->getBuffer(address)->size;
                        allocAndStoreShadowMemory(AddrSpaceGlobal, address, ShadowContext::getPoisonedValue(size));
                    }

                    m_deferredInit.push_back(*value);
                    break;
                }
                case AddrSpaceLocal:
                {
                    // Local pointer kernel arguments and local data variables
                    // value->second.data == NULL
                    // value->second.size == val size
                    if(llvm::isa<llvm::Argument>(value->first))
                    {
                        // Arguments have a private pointer
                        m_deferredInit.push_back(*value);
                    }
                    else
                    {
                        // Variables have a global pointer
                        TypedValue cleanValue = m_pool.clone(ShadowContext::getCleanValue(value->first));
                        shadowContext.setGlobalValue(value->first, cleanValue);
                    }

                    m_deferredInitGroup.push_back(*value);
                    break;
                }
                case AddrSpacePrivate:
                {
                    const llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(value->first);

                    if(A && A->hasByValAttr())
                    {
                        // ByVal kernel argument
                        // value->second.data == val
                        // value->second.size == val size
                        m_deferredInit.push_back(*value);
                    }
                    else
                    {
                        // Private struct/Union definitions with global type
                        // value->second.data == val
                        // value->second.size == val size
                        m_deferredInit.push_back(*value);
                        TypedValue cleanValue = m_pool.clone(ShadowContext::getCleanValue(value->first));
                        shadowContext.setGlobalValue(value->first, cleanValue);
                    }
                    break;
                }
                default:
                    FATAL_ERROR("Unsupported addressspace %d", type->getPointerAddressSpace());
            }
        }
        else
        {
            // Non pointer type kernel arguments
            // value->second.data == val
            // value->second.size == val size
            m_deferredInit.push_back(*value);
        }
    }
}

void MemCheckUninitialized::workItemBegin(const WorkItem *workItem)
{
    shadowContext.createMemoryPool();
    shadowContext.allocateWorkItems();
    ShadowWorkItem *shadowWI = shadowContext.createShadowWorkItem(workItem);

    for(auto value : m_deferredInit)
    {
        const llvm::Type *type = value.first->getType();

        if(type->isPointerTy())
        {
            switch(type->getPointerAddressSpace())
            {
                case AddrSpaceGlobal:
                    {
                        // Global pointer kernel arguments
                        // value.second.data == ptr
                        // value.second.size == ptr size
                        shadowWI->setValue(value.first, ShadowContext::getCleanValue(type));
                        break;
                    }
                case AddrSpaceLocal:
                    {
                        // Local pointer kernel arguments
                        // value.second.data == NULL
                        // value.second.size == val size
                        shadowWI->setValue(value.first, ShadowContext::getCleanValue(value.first));
                        break;
                    }
                case AddrSpacePrivate:
                    {
                        const llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(value.first);

                        if(A && A->hasByValAttr())
                        {
                            // ByVal kernel argument
                            // value.second.data == val
                            // value.second.size == val size
                            size_t address = workItem->getOperand(value.first).getPointer();
                            TypedValue cleanValue = ShadowContext::getCleanValue(value.second.size);
                            allocAndStoreShadowMemory(AddrSpacePrivate, address, cleanValue, workItem);
                            shadowWI->setValue(value.first, ShadowContext::getCleanValue(value.first));
                        }
                        else
                        {
                            // Private struct/Union definitions with global type
                            // value.second.data == NULL
                            // value.second.size == val size
                            size_t address = workItem->getOperand(value.first).getPointer();
                            TypedValue cleanValue = ShadowContext::getCleanValue(value.second.size);
                            allocAndStoreShadowMemory(AddrSpacePrivate, address, cleanValue, workItem);
                        }
                        break;
                    }
            }
        }
        else
        {
            // Non pointer type kernel arguments
            // value->second.data == val
            // value->second.size == val size
            shadowWI->setValue(value.first, ShadowContext::getCleanValue(value.first));
        }
    }

#ifdef DUMP_SHADOW
    //shadowContext.dump(workItem);
#endif
}

void MemCheckUninitialized::workGroupBegin(const WorkGroup *workGroup)
{
    shadowContext.allocateWorkGroups();
    ShadowWorkGroup *shadowWG = shadowContext.createShadowWorkGroup(workGroup);

    for(auto value : m_deferredInitGroup)
    {
        // Local data variables
        // value->second.data == NULL
        // value->second.size == val size
        size_t address = workGroup->getLocalMemoryAddress(value.first);
        TypedValue v;

        if(llvm::isa<llvm::Argument>(value.first))
        {
            //TODO: Local memory clean or poisoned? May need to differentiate between kernel argument (?) and variable (poisoned)
            v = ShadowContext::getPoisonedValue(value.second.size);
        }
        else
        {
            v = ShadowContext::getPoisonedValue(value.second.size);
        }

        allocAndStoreShadowMemory(AddrSpaceLocal, address, v, NULL, workGroup);
    }
}

void MemCheckUninitialized::workItemComplete(const WorkItem *workItem)
{
    shadowContext.destroyShadowWorkItem(workItem);
    shadowContext.freeWorkItems();
    shadowContext.destroyMemoryPool();
}

void MemCheckUninitialized::workGroupComplete(const WorkGroup *workGroup)
{
    shadowContext.destroyShadowWorkGroup(workGroup);
    shadowContext.freeWorkGroups();
}

void MemCheckUninitialized::hostMemoryStore(const Memory *memory,
                             size_t address, size_t size,
                             const uint8_t *storeData)
{
    if(memory->getAddressSpace() == AddrSpaceGlobal)
    {
        TypedValue v = ShadowContext::getCleanValue(size);
        allocAndStoreShadowMemory(AddrSpaceGlobal, address, v);
    }
}

void MemCheckUninitialized::instructionExecuted(const WorkItem *workItem,
                                        const llvm::Instruction *instruction,
                                        const TypedValue& result)
{
#ifdef DUMP_SHADOW
    cout << "++++++++++++++++++++++++++++++++++++++++++++" << endl;
    instruction->dump();
#endif

    switch(instruction->getOpcode())
    {
        case llvm::Instruction::Add:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::Alloca:
        {
            const llvm::AllocaInst *allocaInst = ((const llvm::AllocaInst*)instruction);

            size_t address = result.getPointer();

            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, ShadowContext::getCleanValue(instruction));

            TypedValue v = ShadowContext::getPoisonedValue(allocaInst->getAllocatedType());
            allocAndStoreShadowMemory(AddrSpacePrivate, address, v, workItem);
            break;
        }
        case llvm::Instruction::And:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::AShr:
        {
            TypedValue S0 = shadowContext.getValue(workItem, instruction->getOperand(0));
            TypedValue S1 = shadowContext.getValue(workItem, instruction->getOperand(1));

            if(!ShadowContext::isCleanValue(S1))
            {
                shadowContext.getShadowWorkItem(workItem)->setValue(instruction, ShadowContext::getPoisonedValue(instruction));
            }
            else
            {
                TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);
                TypedValue Shift = workItem->getOperand(instruction->getOperand(1));
                uint64_t shiftMask = (S0.num > 1 ? S0.size : max((size_t)S0.size, sizeof(uint32_t))) * 8 - 1;

                for (unsigned i = 0; i < S0.num; i++)
                {
                    newShadow.setUInt(S0.getSInt(i) >> (Shift.getUInt(i) & shiftMask), i);
                }

                shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            }

            break;
        }
        case llvm::Instruction::BitCast:
        {
            TypedValue shadow = shadowContext.getValue(workItem, instruction->getOperand(0));
            TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);

            memcpy(newShadow.data, shadow.data, newShadow.size*newShadow.num);
            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::Br:
        {
            checkAllOperandsDefined(workItem, instruction);
#ifdef DUMP_SHADOW
            // Insert pseudo value to keep numbering
            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, ShadowContext::getCleanValue(3));
#endif
            break;
        }
        case llvm::Instruction::Call:
        {
            const llvm::CallInst *callInst = ((const llvm::CallInst*)instruction);
            const llvm::Function *function = callInst->getCalledFunction();

            // Check for indirect function calls
            if (!function)
            {
                // Resolve indirect function pointer
                const llvm::Value *func = callInst->getCalledValue();
                const llvm::Value *funcPtr = ((const llvm::User*)func)->getOperand(0);
                function = (const llvm::Function*)funcPtr;
            }

            assert(!function->isVarArg() && "Variadic functions are not supported!");

            ShadowWorkItem *shadowWI = shadowContext.getShadowWorkItem(workItem);

            // For inline asm, do the usual thing: check argument shadow and mark all
            // outputs as clean. Note that any side effects of the inline asm that are
            // not immediately visible in its constraints are not handled.
            if (callInst->isInlineAsm())
            {
                checkAllOperandsDefined(workItem, instruction);
                shadowWI->setValue(instruction, ShadowContext::getCleanValue(instruction));
                break;
            }

            if(const llvm::IntrinsicInst *II = llvm::dyn_cast<const llvm::IntrinsicInst>(instruction))
            {
                handleIntrinsicInstruction(workItem, II);
                break;
            }

            if(function->isDeclaration())
            {
                if(!handleBuiltinFunction(workItem, function->getName().str(), callInst, result))
                {
                    // Handle external function calls
                    checkAllOperandsDefined(workItem, instruction);

                    if(callInst->getType()->isSized())
                    {
                        // Set return value only if function is non-void
                        shadowWI->setValue(instruction, ShadowContext::getCleanValue(instruction));
                    }
                }
                break;
            }

            assert(!llvm::isa<const llvm::IntrinsicInst>(instruction) && "intrinsics are handled elsewhere");

            // Fresh values for function
            ShadowValues *values = shadowWI->createCleanShadowValues();

            llvm::Function::const_arg_iterator argItr;
            for (argItr = function->arg_begin(); argItr != function->arg_end(); argItr++)
            {
                const llvm::Value *Val = callInst->getArgOperand(argItr->getArgNo());

                if (!Val->getType()->isSized())
                {
                    continue;
                }

                if(argItr->hasByValAttr())
                {
                    assert(Val->getType()->isPointerTy() && "ByVal argument is not a pointer!");
                    //// Make new copy of shadow in private memory
                    size_t origShadowAddress = workItem->getOperand(Val).getPointer();
                    size_t newShadowAddress = workItem->getOperand(argItr).getPointer();
                    unsigned char *origShadowData = (unsigned char*)shadowWI->getMemoryPointer(origShadowAddress);
                    size_t size = getTypeSize(argItr->getType()->getPointerElementType());

                    // Set new shadow memory
                    TypedValue v = ShadowContext::getCleanValue(size);
                    memcpy(v.data, origShadowData, size);
                    allocAndStoreShadowMemory(AddrSpacePrivate, newShadowAddress, v, workItem);
                    values->setValue(argItr, ShadowContext::getCleanValue(argItr));
                }
                else
                {
                    TypedValue newShadow = shadowContext.getMemoryPool()->clone(shadowContext.getValue(workItem, Val));
                    values->setValue(argItr, newShadow);
                }
            }

            // Now, get the shadow for the RetVal.
            if(callInst->getType()->isSized())
            {
                values->setCall(callInst);
            }

            shadowWI->pushValues(values);

            break;
        }
        case llvm::Instruction::ExtractElement:
        {
            const llvm::ExtractElementInst *extractInst = ((const llvm::ExtractElementInst*)instruction);

            TypedValue indexShadow = shadowContext.getValue(workItem, extractInst->getIndexOperand());

            if(!ShadowContext::isCleanValue(indexShadow))
            {
                logUninitializedIndex();
            }

            TypedValue vectorShadow = shadowContext.getValue(workItem, extractInst->getVectorOperand());
            TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);

            unsigned index = workItem->getOperand(extractInst->getIndexOperand()).getUInt();
            memcpy(newShadow.data, vectorShadow.data + newShadow.size*index, newShadow.size);

            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::ExtractValue:
        {
            const llvm::ExtractValueInst *extractInst = ((const llvm::ExtractValueInst*)instruction);

            const llvm::Value *Agg = extractInst->getAggregateOperand();
            TypedValue ResShadow = shadowContext.getMemoryPool()->clone(result);

            llvm::ArrayRef<unsigned int> indices = extractInst->getIndices();

            // Compute offset for target value
            int offset = 0;
            const llvm::Type *type = Agg->getType();
            for (unsigned i = 0; i < indices.size(); i++)
            {
                if (type->isArrayTy())
                {
                    type = type->getArrayElementType();
                    offset += getTypeSize(type) * indices[i];
                }
                else if (type->isStructTy())
                {
                    offset += getStructMemberOffset((const llvm::StructType*)type, indices[i]);
                    type = type->getStructElementType(indices[i]);
                }
                else
                {
                    FATAL_ERROR("Unsupported aggregate type: %d", type->getTypeID())
                }
            }

            // Copy target value to result
            memcpy(ResShadow.data, shadowContext.getValue(workItem, Agg).data + offset, getTypeSize(type));

            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, ResShadow);
            break;
        }
        case llvm::Instruction::FAdd:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::FCmp:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::FDiv:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::FMul:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::FPExt:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::FPToSI:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::FPToUI:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::FPTrunc:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::FRem:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::FSub:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::GetElementPtr:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::ICmp:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::InsertElement:
        {
            TypedValue indexShadow = shadowContext.getValue(workItem, instruction->getOperand(2));

            if(!ShadowContext::isCleanValue(indexShadow))
            {
                logUninitializedIndex();
            }

            TypedValue vectorShadow = shadowContext.getValue(workItem, instruction->getOperand(0));
            TypedValue elementShadow = shadowContext.getValue(workItem, instruction->getOperand(1));
            TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);

            unsigned index = workItem->getOperand(instruction->getOperand(2)).getUInt();
            memcpy(newShadow.data, vectorShadow.data, newShadow.size*newShadow.num);
            memcpy(newShadow.data + index*newShadow.size, elementShadow.data, newShadow.size);

            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::InsertValue:
        {
            const llvm::InsertValueInst *insertInst = (const llvm::InsertValueInst*)instruction;

            TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);

            // Load original aggregate data
            const llvm::Value *agg = insertInst->getAggregateOperand();
            memcpy(newShadow.data, shadowContext.getValue(workItem, agg).data, newShadow.size*newShadow.num);

            // Compute offset for inserted value
            int offset = 0;
            llvm::ArrayRef<unsigned int> indices = insertInst->getIndices();
            const llvm::Type *type = agg->getType();
            for (unsigned i = 0; i < indices.size(); i++)
            {
                if (type->isArrayTy())
                {
                    type = type->getArrayElementType();
                    offset += getTypeSize(type) * indices[i];
                }
                else if (type->isStructTy())
                {
                    offset += getStructMemberOffset((const llvm::StructType*)type, indices[i]);
                    type = type->getStructElementType(indices[i]);
                }
                else
                {
                    FATAL_ERROR("Unsupported aggregate type: %d", type->getTypeID())
                }
            }

            // Copy inserted value into result
            const llvm::Value *value = insertInst->getInsertedValueOperand();
            memcpy(newShadow.data + offset, shadowContext.getValue(workItem, value).data, getTypeSize(value->getType()));

            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::IntToPtr:
        {
            TypedValue shadow = shadowContext.getValue(workItem, instruction->getOperand(0));
            TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                newShadow.setPointer(shadow.getUInt(i), i);
            }

            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::Load:
        {
            assert(instruction->getType()->isSized() && "Load type must have size");
            const llvm::LoadInst *loadInst = ((const llvm::LoadInst*)instruction);
            const llvm::Value *Addr = loadInst->getPointerOperand();

            size_t address = workItem->getOperand(Addr).getPointer();
            unsigned addrSpace = loadInst->getPointerAddressSpace();

            TypedValue v = shadowContext.getMemoryPool()->clone(result);
            loadShadowMemory(addrSpace, address, v, workItem);
            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, v);

//            if (ClCheckAccessAddress)
//                insertShadowCheck(I.getPointerOperand(), &I);
//
//            if (I.isAtomic())
//                I.setOrdering(addAcquireOrdering(I.getOrdering()));

            break;
        }
        case llvm::Instruction::LShr:
        {
            TypedValue S0 = shadowContext.getValue(workItem, instruction->getOperand(0));
            TypedValue S1 = shadowContext.getValue(workItem, instruction->getOperand(1));

            if(!ShadowContext::isCleanValue(S1))
            {
                shadowContext.getShadowWorkItem(workItem)->setValue(instruction, ShadowContext::getPoisonedValue(instruction));
            }
            else
            {
                TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);
                TypedValue Shift = workItem->getOperand(instruction->getOperand(1));
                uint64_t shiftMask = (S0.num > 1 ? S0.size : max((size_t)S0.size, sizeof(uint32_t))) * 8 - 1;

                for (unsigned i = 0; i < S0.num; i++)
                {
                    newShadow.setUInt(S0.getUInt(i) >> (Shift.getUInt(i) & shiftMask), i);
                }

                shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            }

            break;
        }
        case llvm::Instruction::Mul:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::Or:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::PHI:
        {
            //FIXME: m_position is private
            const llvm::PHINode *phiNode = (const llvm::PHINode*)instruction;
            //const llvm::Value *value = phiNode->getIncomingValueForBlock(
            //        (const llvm::BasicBlock*)m_position->prevBlock);

            //TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);

            //memcpy(newShadow.data, getValue(value).data, newShadow.size*newShadow.num);
            //setValue(instruction, newShadow);

            bool poisoned = false;

            for(int i = 0; i < phiNode->getNumIncomingValues(); ++i)
            {
                const llvm::Value *V = phiNode->getIncomingValue(i);

                if(!shadowContext.hasValue(workItem, V))
                {
                    continue;
                }

                if(!ShadowContext::isCleanValue(shadowContext.getValue(workItem, V)))
                {
                    shadowContext.getShadowWorkItem(workItem)->setValue(instruction, ShadowContext::getPoisonedValue(instruction));
                    poisoned = true;
                    break;
                }
            }

            if(!poisoned)
            {
                shadowContext.getShadowWorkItem(workItem)->setValue(instruction, ShadowContext::getCleanValue(instruction));
            }
            break;
        }
        case llvm::Instruction::PtrToInt:
        {
            TypedValue shadow = shadowContext.getValue(workItem, instruction->getOperand(0));
            TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                newShadow.setUInt(shadow.getPointer(i), i);
            }

            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::Ret:
        {
            const llvm::ReturnInst *retInst = ((const llvm::ReturnInst*)instruction);
            const llvm::Value *RetVal = retInst->getReturnValue();

            ShadowWorkItem *shadowWI = shadowContext.getShadowWorkItem(workItem);

            if(RetVal)
            {
                //Value *ShadowPtr = getValuePtrForRetval(RetVal, IRB);
                //if (CheckReturnValue) {
                //    insertShadowCheck(RetVal, &I);
                //    Value *Shadow = getCleanValue(RetVal);
                //    IRB.CreateAlignedStore(Shadow, ShadowPtr, kShadowTLSAlignment);
                //} else {
                TypedValue retValShadow = shadowContext.getMemoryPool()->clone(shadowContext.getValue(workItem, RetVal));
                const llvm::CallInst *callInst = shadowWI->getCall();
                shadowWI->popValues();
                shadowWI->setValue(callInst, retValShadow);
                //}
            }
            else
            {
#ifdef DUMP_SHADOW
                // Insert pseudo value to keep numbering
                shadowContext.getShadowWorkItem(workItem)->setValue(instruction, ShadowContext::getCleanValue(3));
#endif
                shadowWI->popValues();
            }

            break;
        }
        case llvm::Instruction::SDiv:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::Select:
        {
            const llvm::SelectInst *selectInst = (const llvm::SelectInst*)instruction;

            TypedValue opCondition = workItem->getOperand(selectInst->getCondition());
            TypedValue conditionShadow = shadowContext.getValue(workItem, selectInst->getCondition());
            TypedValue newShadow;

            if(!ShadowContext::isCleanValue(conditionShadow))
            {
                newShadow = ShadowContext::getPoisonedValue(instruction);
            }
            else
            {
                newShadow = shadowContext.getMemoryPool()->clone(result);

                for(unsigned i = 0; i < result.num; i++)
                {
                    const bool cond = selectInst->getCondition()->getType()->isVectorTy() ?
                        opCondition.getUInt(i) :
                        opCondition.getUInt();
                    const llvm::Value *op = cond ?
                        selectInst->getTrueValue() :
                        selectInst->getFalseValue();

                    memcpy(newShadow.data + i*newShadow.size,
                            shadowContext.getValue(workItem, op).data + i*newShadow.size,
                            newShadow.size);
                }
            }

            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::SExt:
        {
            const llvm::Value *operand = instruction->getOperand(0);
            TypedValue shadow = shadowContext.getValue(workItem, operand);
            TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                int64_t val = shadow.getSInt(i);
                if (operand->getType()->getPrimitiveSizeInBits() == 1)
                {
                    val = val ? -1 : 0;
                }
                newShadow.setSInt(val, i);
            }

            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);

            break;
        }
        case llvm::Instruction::Shl:
        {
            TypedValue S0 = shadowContext.getValue(workItem, instruction->getOperand(0));
            TypedValue S1 = shadowContext.getValue(workItem, instruction->getOperand(1));

            if(!ShadowContext::isCleanValue(S1))
            {
                shadowContext.getShadowWorkItem(workItem)->setValue(instruction, ShadowContext::getPoisonedValue(instruction));
            }
            else
            {
                TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);
                TypedValue Shift = workItem->getOperand(instruction->getOperand(1));
                uint64_t shiftMask = (S0.num > 1 ? S0.size : max((size_t)S0.size, sizeof(uint32_t))) * 8 - 1;

                for (unsigned i = 0; i < S0.num; i++)
                {
                    newShadow.setUInt(S0.getUInt(i) << (Shift.getUInt(i) & shiftMask), i);
                }

                shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            }

            break;
        }
        case llvm::Instruction::ShuffleVector:
        {
            const llvm::ShuffleVectorInst *shuffleInst = (const llvm::ShuffleVectorInst*)instruction;

            TypedValue maskShadow = shadowContext.getValue(workItem, shuffleInst->getMask());

            if(!ShadowContext::isCleanValue(maskShadow))
            {
                logUninitializedMask();
            }

            const llvm::Value *v1 = shuffleInst->getOperand(0);
            const llvm::Value *v2 = shuffleInst->getOperand(1);
            TypedValue mask = workItem->getOperand(shuffleInst->getMask());
            TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);

            unsigned num = v1->getType()->getVectorNumElements();
            for (unsigned i = 0; i < newShadow.num; i++)
            {
                if (shuffleInst->getMask()->getAggregateElement(i)->getValueID() == llvm::Value::UndefValueVal)
                {
                    // Don't care / undef
                    continue;
                }

                const llvm::Value *src = v1;
                unsigned int index = mask.getUInt(i);
                if (index >= num)
                {
                    index -= num;
                    src = v2;
                }

                memcpy(newShadow.data + i*newShadow.size, shadowContext.getValue(workItem, src).data + index*newShadow.size, newShadow.size);
            }

            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::SIToFP:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::SRem:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::Store:
        {
            PARANOID_CHECK(workItem, instruction);
            const llvm::StoreInst *storeInst = ((const llvm::StoreInst*)instruction);
            const llvm::Value *Val = storeInst->getValueOperand();
            const llvm::Value *Addr = storeInst->getPointerOperand();

            size_t address = workItem->getOperand(Addr).getPointer();
            unsigned addrSpace = storeInst->getPointerAddressSpace();

            TypedValue shadowVal = storeInst->isAtomic() ? ShadowContext::getCleanValue(Val) : shadowContext.getValue(workItem, Val);

            if(addrSpace != AddrSpacePrivate)
            {
                if(!ShadowContext::isCleanValue(shadowVal))
                {
                    logUninitializedWrite(addrSpace, address);
                }
            }

            storeShadowMemory(addrSpace, address, shadowVal, workItem);

#ifdef DUMP_SHADOW
            // Insert pseudo value to keep numbering
            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, ShadowContext::getCleanValue(3));
#endif
            break;
        }
        case llvm::Instruction::Sub:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::Switch:
        {
            checkAllOperandsDefined(workItem, instruction);
#ifdef DUMP_SHADOW
            // Insert pseudo value to keep numbering
            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, ShadowContext::getCleanValue(3));
#endif
            break;
        }
        case llvm::Instruction::Trunc:
        {
            TypedValue shadow = shadowContext.getValue(workItem, instruction->getOperand(0));
            TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                memcpy(newShadow.data+i*newShadow.size, shadow.data+i*shadow.size, newShadow.size);
            }

            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::UDiv:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::UIToFP:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::URem:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::Unreachable:
            FATAL_ERROR("Encountered unreachable instruction");
        case llvm::Instruction::Xor:
        {
            SimpleOr(workItem, instruction);
            break;
        }
        case llvm::Instruction::ZExt:
        {
            TypedValue shadow = shadowContext.getValue(workItem, instruction->getOperand(0));
            TypedValue newShadow = shadowContext.getMemoryPool()->clone(result);

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                newShadow.setUInt(shadow.getUInt(i), i);
            }

            shadowContext.getShadowWorkItem(workItem)->setValue(instruction, newShadow);
            break;
        }
        default:
            FATAL_ERROR("Unsupported instruction: %s", instruction->getOpcodeName());
    }

#ifdef DUMP_SHADOW
    //shadowContext.dump(workItem);
#endif
}

void MemCheckUninitialized::memoryMap(const Memory *memory, size_t address, size_t offset, size_t size, cl_map_flags flags)
{
    if(!(flags & CL_MAP_READ))
    {
        allocAndStoreShadowMemory(memory->getAddressSpace(), address + offset, ShadowContext::getCleanValue(size));
    }
}

void MemCheckUninitialized::SimpleOr(const WorkItem *workItem, const llvm::Instruction *I)
{
    PARANOID_CHECK(workItem, I);
    for(llvm::Instruction::const_op_iterator OI = I->op_begin(); OI != I->op_end(); ++OI)
    {
        if(!ShadowContext::isCleanValue(shadowContext.getValue(workItem, OI->get())))
        {
            shadowContext.getShadowWorkItem(workItem)->setValue(I, ShadowContext::getPoisonedValue(I));
            return;
        }
    }

    shadowContext.getShadowWorkItem(workItem)->setValue(I, ShadowContext::getCleanValue(I));
}

void MemCheckUninitialized::SimpleOrAtomic(const WorkItem *workItem, const llvm::CallInst *CI)
{
    unsigned addrSpace = CI->getArgOperand(0)->getType()->getPointerAddressSpace();
    size_t address = workItem->getOperand(CI->getArgOperand(0)).getPointer();
    TypedValue argShadow = shadowContext.getValue(workItem, CI->getArgOperand(1));
    TypedValue oldShadow = {
        4,
        1,
        new unsigned char[4]
    };

    TypedValue newShadow;

    if(addrSpace == AddrSpaceGlobal)
    {
        getShadowMemory(AddrSpaceGlobal)->lock(address);
    }

    loadShadowMemory(addrSpace, address, oldShadow, workItem);

    if(!ShadowContext::isCleanValue(argShadow) || !ShadowContext::isCleanValue(oldShadow))
    {
        newShadow = ShadowContext::getPoisonedValue(4);
    }
    else
    {
        newShadow = ShadowContext::getCleanValue(4);
    }

    storeShadowMemory(addrSpace, address, newShadow, workItem);

    if(addrSpace == AddrSpaceGlobal)
    {
        getShadowMemory(AddrSpaceGlobal)->unlock(address);
    }

    shadowContext.getShadowWorkItem(workItem)->setValue(CI, shadowContext.getMemoryPool()->clone(oldShadow));
    delete[] oldShadow.data;
}

void MemCheckUninitialized::allocAndStoreShadowMemory(unsigned addrSpace, size_t address, TypedValue SM, const WorkItem *workItem, const WorkGroup *workGroup)
{
    switch(addrSpace)
    {
        case AddrSpacePrivate:
        {
            if(!workItem)
            {
                FATAL_ERROR("Work item needed to store private memory!");
            }

            shadowContext.getShadowWorkItem(workItem)->allocMemory(address, SM.size*SM.num);
            break;
        }
        case AddrSpaceLocal:
        {
            if(!workGroup)
            {
                if(!workItem)
                {
                    FATAL_ERROR("Work item or work group needed to store local memory!");
                }

                workGroup = workItem->getWorkGroup();
            }

            shadowContext.getShadowWorkGroup(workGroup)->allocMemory(address, SM.size*SM.num);
            break;
        }
        case AddrSpaceConstant:
            //Do nothing
            //TODO: Eventually store value
            break;
        case AddrSpaceGlobal:
            shadowContext.getGlobalMemory()->allocate(address, SM.size*SM.num);
            break;
        default:
            FATAL_ERROR("Unsupported addressspace %d", addrSpace);
    }

    storeShadowMemory(addrSpace, address, SM, workItem, workGroup);
}

Memory* MemCheckUninitialized::getMemory(unsigned addrSpace, const WorkItem *workItem, const WorkGroup *workGroup) const
{
    switch(addrSpace)
    {
        case AddrSpacePrivate:
        {
            if(!workItem)
            {
                FATAL_ERROR("Work item needed to access private memory!");
            }

            return workItem->getPrivateMemory();
        }
        case AddrSpaceLocal:
        {
            if(!workGroup)
            {
                if(!workItem)
                {
                    FATAL_ERROR("Work item or work group needed to access local memory!");
                }

                workGroup = workItem->getWorkGroup();
            }

            return workGroup->getLocalMemory();
        }
        //case AddrSpaceConstant:
        //    break;
        case AddrSpaceGlobal:
            return m_context->getGlobalMemory();
        default:
            FATAL_ERROR("Unsupported addressspace %d", addrSpace);
    }
}

ShadowMemory* MemCheckUninitialized::getShadowMemory(unsigned addrSpace, const WorkItem *workItem, const WorkGroup *workGroup) const
{
    switch(addrSpace)
    {
        case AddrSpacePrivate:
        {
            if(!workItem)
            {
                FATAL_ERROR("Work item needed to access private memory!");
            }

            return shadowContext.getShadowWorkItem(workItem)->getPrivateMemory();
        }
        case AddrSpaceLocal:
        {
            if(!workGroup)
            {
                if(!workItem)
                {
                    FATAL_ERROR("Work item or work group needed to access local memory!");
                }

                workGroup = workItem->getWorkGroup();
            }

            return shadowContext.getShadowWorkGroup(workGroup)->getLocalMemory();
        }
        //case AddrSpaceConstant:
        //    break;
        case AddrSpaceGlobal:
            return shadowContext.getGlobalMemory();
        default:
            FATAL_ERROR("Unsupported addressspace %d", addrSpace);
    }
}

void MemCheckUninitialized::storeShadowMemory(unsigned addrSpace, size_t address, TypedValue SM, const WorkItem *workItem, const WorkGroup *workGroup)
{
#ifdef DUMP_SHADOW
    cout << "Store " << hex << SM << " to space " << dec << addrSpace << " at address " << hex << address << endl;
#endif
    switch(addrSpace)
    {
        case AddrSpacePrivate:
        {
            if(!workItem)
            {
                FATAL_ERROR("Work item needed to store private memory!");
            }

            shadowContext.getShadowWorkItem(workItem)->storeMemory(SM.data, address, SM.size*SM.num);
            break;
        }
        case AddrSpaceLocal:
        {
            if(!workGroup)
            {
                if(!workItem)
                {
                    FATAL_ERROR("Work item or work group needed to store local memory!");
                }

                workGroup = workItem->getWorkGroup();
            }

            shadowContext.getShadowWorkGroup(workGroup)->storeMemory(SM.data, address, SM.size*SM.num);
            break;
        }
        case AddrSpaceConstant:
            //Do nothing
            //TODO: Eventually store value
            break;
        case AddrSpaceGlobal:
            shadowContext.getGlobalMemory()->store(SM.data, address, SM.size*SM.num);
            break;
        default:
            FATAL_ERROR("Unsupported addressspace %d", addrSpace);
    }
}

void MemCheckUninitialized::loadShadowMemory(unsigned addrSpace, size_t address, TypedValue &SM, const WorkItem *workItem, const WorkGroup *workGroup)
{
    switch(addrSpace)
    {
        case AddrSpacePrivate:
        {
            if(!workItem)
            {
                FATAL_ERROR("Work item needed to load private memory!");
            }

            shadowContext.getShadowWorkItem(workItem)->loadMemory(SM.data, address, SM.size*SM.num);
            break;
        }
        case AddrSpaceLocal:
        {
            if(!workGroup)
            {
                if(!workItem)
                {
                    FATAL_ERROR("Work item or work group needed to load local memory!");
                }

                workGroup = workItem->getWorkGroup();
            }

            shadowContext.getShadowWorkGroup(workGroup)->loadMemory(SM.data, address, SM.size*SM.num);
            break;
        }
        case AddrSpaceConstant:
            memset(SM.data, 0, SM.size*SM.num);
            break;
        case AddrSpaceGlobal:
            shadowContext.getGlobalMemory()->load(SM.data, address, SM.size*SM.num);
            break;
        default:
            FATAL_ERROR("Unsupported addressspace %d", addrSpace);
    }
#ifdef DUMP_SHADOW
    cout << "Loaded " << hex << SM << " from space " << dec << addrSpace << " at address " << hex << address << endl;
#endif
}

bool MemCheckUninitialized::checkAllOperandsDefined(const WorkItem *workItem, const llvm::Instruction *I)
{
    for(llvm::Instruction::const_op_iterator OI = I->op_begin(); OI != I->op_end(); ++OI)
    {
        if(!ShadowContext::isCleanValue(shadowContext.getValue(workItem, OI->get())))
        {
#ifdef DUMP_SHADOW
            OI->get()->dump();
            cout << "Shadow value: " << shadowContext.getValue(workItem, OI->get()) << endl;
#endif
            logUninitializedCF();
#ifdef DUMP_SHADOW
            shadowContext.dump(workItem);
#endif
            return false;
        }
    }

    return true;
}

void MemCheckUninitialized::copyShadowMemoryStrided(unsigned dstAddrSpace, size_t dst, unsigned srcAddrSpace, size_t src, size_t num, size_t stride, unsigned size, const WorkItem *workItem, const WorkGroup *workGroup)
{
    TypedValue v = {
        size,
        1,
        new unsigned char[size]
    };

    for (unsigned i = 0; i < num; i++)
    {
        loadShadowMemory(srcAddrSpace, src, v, workItem, workGroup);
        storeShadowMemory(dstAddrSpace, dst, v, workItem, workGroup);
        src += stride * size;
        dst += stride * size;
    }

    delete[] v.data;
}

void MemCheckUninitialized::copyShadowMemory(unsigned dstAddrSpace, size_t dst, unsigned srcAddrSpace, size_t src, unsigned size, const WorkItem *workItem, const WorkGroup *workGroup)
{
    copyShadowMemoryStrided(dstAddrSpace, dst, srcAddrSpace, src, 1, 1, size, workItem, workGroup);
}

std::string MemCheckUninitialized::extractUnmangledName(const std::string fullname)
{
    // Extract unmangled name
    if(fullname.compare(0,2, "_Z") == 0)
    {
        int len = atoi(fullname.c_str() + 2);
        int start = fullname.find_first_not_of("0123456789", 2);
        return fullname.substr(start, len);
    }
    else
    {
        return fullname;
    }
}

bool MemCheckUninitialized::handleBuiltinFunction(const WorkItem *workItem, string name, const llvm::CallInst *CI, const TypedValue result)
{
    name = extractUnmangledName(name);

    if(name == "async_work_group_copy" ||
       name == "async_work_group_strided_copy")
    {
        int arg = 0;

        // Get src/dest addresses
        const llvm::Value *destOp = CI->getArgOperand(arg++);
        const llvm::Value *srcOp = CI->getArgOperand(arg++);
        size_t dst = workItem->getOperand(destOp).getPointer();
        size_t src = workItem->getOperand(srcOp).getPointer();

        // Get size of copy
        unsigned elemSize = getTypeSize(destOp->getType()->getPointerElementType());
        uint64_t num = workItem->getOperand(CI->getArgOperand(arg++)).getUInt();

        // Get stride
        size_t stride = 1;

        if(name == "async_work_group_strided_copy")
        {
            stride = workItem->getOperand(CI->getArgOperand(arg++)).getUInt();
        }

        const llvm::Value *eventOp = CI->getArgOperand(arg++);
        TypedValue eventShadow = shadowContext.getValue(workItem, eventOp);

        // Get type of copy
        AddressSpace dstAddrSpace = AddrSpaceLocal;
        AddressSpace srcAddrSpace = AddrSpaceLocal;

        if(destOp->getType()->getPointerAddressSpace() == AddrSpaceLocal)
        {
            srcAddrSpace = AddrSpaceGlobal;
        }
        else
        {
            dstAddrSpace = AddrSpaceGlobal;
        }

        copyShadowMemoryStrided(dstAddrSpace, dst, srcAddrSpace, src, num, stride, elemSize, workItem);
        shadowContext.getShadowWorkItem(workItem)->setValue(CI, eventShadow);
        return true;
    }
    else if(name == "wait_group_events")
    {
        uint64_t num = workItem->getOperand(CI->getArgOperand(0)).getUInt();
        size_t address = workItem->getOperand(CI->getArgOperand(1)).getPointer();

        TypedValue eventShadow = {
            sizeof(size_t),
            1,
            new unsigned char[sizeof(size_t)]
        };

        for(unsigned i = 0; i < num; ++i)
        {
            loadShadowMemory(AddrSpacePrivate, address, eventShadow, workItem);

            if(!ShadowContext::isCleanValue(eventShadow))
            {
                logUninitializedCF();
                delete[] eventShadow.data;
                return true;
            }

            address += sizeof(size_t);
        }

        delete[] eventShadow.data;
        return true;
    }
    else if(name.compare(0, 6, "atomic") == 0)
    {
        if(name.compare(6, string::npos, "cmpxchg") == 0)
        {
            unsigned addrSpace = CI->getArgOperand(0)->getType()->getPointerAddressSpace();
            size_t address = workItem->getOperand(CI->getArgOperand(0)).getPointer();
            uint32_t cmp = workItem->getOperand(CI->getArgOperand(1)).getUInt();
            uint32_t old = workItem->getOperand(CI).getUInt();
            TypedValue argShadow = shadowContext.getValue(workItem, CI->getArgOperand(2));
            TypedValue oldShadow = {
                4,
                1,
                new unsigned char[4]
            };

            // Perform cmpxchg
            if(addrSpace == AddrSpaceGlobal)
            {
                getShadowMemory(AddrSpaceGlobal)->lock(address);
            }

            loadShadowMemory(addrSpace, address, oldShadow, workItem);

            if(old == cmp)
            {
                storeShadowMemory(addrSpace, address, argShadow, workItem);
            }

            if(addrSpace == AddrSpaceGlobal)
            {
                getShadowMemory(AddrSpaceGlobal)->unlock(address);
            }

            shadowContext.getShadowWorkItem(workItem)->setValue(CI, shadowContext.getMemoryPool()->clone(oldShadow));
            delete[] oldShadow.data;
            return true;
        }
        else
        {
            SimpleOrAtomic(workItem, CI);
            return true;
        }
    }
    else if(name == "fract")
    {
        unsigned addrSpace = CI->getArgOperand(1)->getType()->getPointerAddressSpace();
        size_t iptr = workItem->getOperand(CI->getArgOperand(1)).getPointer();
        TypedValue argShadow = shadowContext.getValue(workItem, CI->getArgOperand(0));
        TypedValue newElemShadow;
        TypedValue newShadow = shadowContext.getMemoryPool()->clone(argShadow);

        for(unsigned i = 0; i < result.num; ++i)
        {
            if(!ShadowContext::isCleanValue(argShadow, i))
            {
                newElemShadow = ShadowContext::getPoisonedValue(result.size);
            }
            else
            {
                newElemShadow = ShadowContext::getCleanValue(result.size);
            }

            size_t offset = i*result.size;
            storeShadowMemory(addrSpace, iptr + offset, newElemShadow);
            memcpy(newShadow.data, newElemShadow.data, result.size);
        }

        shadowContext.getShadowWorkItem(workItem)->setValue(CI, newShadow);
        return true;
    }

    return false;
}

void MemCheckUninitialized::handleIntrinsicInstruction(const WorkItem *workItem, const llvm::IntrinsicInst *I)
{
    switch (I->getIntrinsicID())
    {
        case llvm::Intrinsic::fmuladd:
        {
            SimpleOr(workItem, I);
            break;
        }
        case llvm::Intrinsic::memcpy:
        {
            const llvm::MemCpyInst *memcpyInst = (const llvm::MemCpyInst*)I;
            size_t dst = workItem->getOperand(memcpyInst->getDest()).getPointer();
            size_t src = workItem->getOperand(memcpyInst->getSource()).getPointer();
            size_t size = workItem->getOperand(memcpyInst->getLength()).getUInt();
            unsigned dstAddrSpace = memcpyInst->getDestAddressSpace();
            unsigned srcAddrSpace = memcpyInst->getSourceAddressSpace();

            copyShadowMemory(dstAddrSpace, dst, srcAddrSpace, src, size, workItem);
            break;
        }
        case llvm::Intrinsic::memset:
        {
            const llvm::MemSetInst *memsetInst = (const llvm::MemSetInst*)I;
            size_t dst = workItem->getOperand(memsetInst->getDest()).getPointer();
            unsigned size = workItem->getOperand(memsetInst->getLength()).getUInt();
            unsigned addrSpace = memsetInst->getDestAddressSpace();

            TypedValue shadowValue = {
                size,
                1,
                new unsigned char[size]
            };

            memset(shadowValue.data, shadowContext.getValue(workItem, memsetInst->getArgOperand(1)).getUInt(), size);
            storeShadowMemory(addrSpace, dst, shadowValue, workItem);

            delete[] shadowValue.data;

            break;
        }
        case llvm::Intrinsic::dbg_declare:
            //Do nothing
            break;
        default:
            FATAL_ERROR("Unsupported intrinsic %s", llvm::Intrinsic::getName(I->getIntrinsicID()).c_str());
    }
}

void MemCheckUninitialized::logUninitializedWrite(unsigned int addrSpace, size_t address) const
{
  Context::Message msg(WARNING, m_context);
  msg << "Uninitialized value written to "
      << getAddressSpaceName(addrSpace)
      << " memory address 0x" << hex << address << endl
      << msg.INDENT
      << "Kernel: " << msg.CURRENT_KERNEL << endl
      << "Entity: " << msg.CURRENT_ENTITY << endl
      << msg.CURRENT_LOCATION << endl;
  msg.send();
}

void MemCheckUninitialized::logUninitializedCF() const
{
  Context::Message msg(WARNING, m_context);
  msg << "Controlflow depends on uninitialized value" << endl
      << msg.INDENT
      << "Kernel: " << msg.CURRENT_KERNEL << endl
      << "Entity: " << msg.CURRENT_ENTITY << endl
      << msg.CURRENT_LOCATION << endl;
  msg.send();
}

void MemCheckUninitialized::logUninitializedIndex() const
{
  Context::Message msg(WARNING, m_context);
  msg << "Instruction depends on an uninitialized index value" << endl
      << msg.INDENT
      << "Kernel: " << msg.CURRENT_KERNEL << endl
      << "Entity: " << msg.CURRENT_ENTITY << endl
      << msg.CURRENT_LOCATION << endl;
  msg.send();
}

void MemCheckUninitialized::logUninitializedMask() const
{
  Context::Message msg(WARNING, m_context);
  msg << "Instruction depends on an uninitialized mask" << endl
      << msg.INDENT
      << "Kernel: " << msg.CURRENT_KERNEL << endl
      << "Entity: " << msg.CURRENT_ENTITY << endl
      << msg.CURRENT_LOCATION << endl;
  msg.send();
}

ShadowWorkItem::ShadowWorkItem(unsigned bufferBits) :
    m_memory(new ShadowMemory(AddrSpacePrivate, bufferBits)), m_values()
{
    pushValues(createCleanShadowValues());
}

ShadowWorkGroup::ShadowWorkGroup(unsigned bufferBits) :
    //FIXME: Hard coded values
    m_memory(new ShadowMemory(AddrSpaceLocal, sizeof(size_t) == 8 ? 16 : 8))
{
}

ShadowWorkGroup::~ShadowWorkGroup()
{
    delete m_memory;
}

ShadowMemory::ShadowMemory(AddressSpace addrSpace, unsigned bufferBits) :
    m_addrSpace(addrSpace), m_map(), m_numBitsAddress((sizeof(size_t)<<3) - bufferBits), m_numBitsBuffer(bufferBits)
{
}

ShadowWorkItem::~ShadowWorkItem()
{
    delete m_memory;

    while(!m_values.empty())
    {
        ShadowValues *values= m_values.top();
        m_values.pop();
        delete values;
    }
}

ShadowMemory::~ShadowMemory()
{
    clear();
}

ShadowValues* ShadowWorkItem::createCleanShadowValues()
{
    return new ShadowValues();
}

ShadowContext::ShadowContext(unsigned bufferBits) :
    m_globalMemory(new ShadowMemory(AddrSpaceGlobal, bufferBits)), m_globalValues(), m_numBitsBuffer(bufferBits)
{
}

ShadowContext::~ShadowContext()
{
    delete m_globalMemory;
}

void ShadowContext::createMemoryPool()
{
    if(m_workSpace.poolUsers == 0)
    {
        m_workSpace.memoryPool = new MemoryPool();
    }

    ++m_workSpace.poolUsers;
}

void ShadowContext::allocateWorkItems()
{
    if(!m_workSpace.workItems)
    {
        m_workSpace.workItems = new ShadowItemMap();
    }
}

void ShadowContext::allocateWorkGroups()
{
    if(!m_workSpace.workGroups)
    {
        m_workSpace.workGroups = new ShadowGroupMap();
    }
}

void ShadowContext::freeWorkItems()
{
    if(m_workSpace.workItems && !m_workSpace.workItems->size())
    {
        delete m_workSpace.workItems;
        m_workSpace.workItems = NULL;
    }
}

void ShadowContext::freeWorkGroups()
{
    if(m_workSpace.workGroups && !m_workSpace.workGroups->size())
    {
        delete m_workSpace.workGroups;
        m_workSpace.workGroups = NULL;
    }
}

ShadowWorkItem* ShadowContext::createShadowWorkItem(const WorkItem *workItem)
{
    assert(!m_workSpace.workItems->count(workItem) && "Workitems may only have one shadow");
    ShadowWorkItem *sWI = new ShadowWorkItem(m_numBitsBuffer);
    (*m_workSpace.workItems)[workItem] = sWI;
    return sWI;
}

ShadowWorkGroup* ShadowContext::createShadowWorkGroup(const WorkGroup *workGroup)
{
    assert(!m_workSpace.workGroups->count(workGroup) && "Workgroups may only have one shadow");
    ShadowWorkGroup *sWG = new ShadowWorkGroup(m_numBitsBuffer);
    (*m_workSpace.workGroups)[workGroup] = sWG;
    return sWG;
}

void ShadowContext::destroyMemoryPool()
{
    --m_workSpace.poolUsers;

    if(m_workSpace.poolUsers == 0)
    {
        delete m_workSpace.memoryPool;
    }
}

void ShadowContext::destroyShadowWorkItem(const WorkItem *workItem)
{
    assert(m_workSpace.workItems->count(workItem) && "No shadow for workitem found!");
    delete (*m_workSpace.workItems)[workItem];
    m_workSpace.workItems->erase(workItem);
}

void ShadowContext::destroyShadowWorkGroup(const WorkGroup *workGroup)
{
    assert(m_workSpace.workGroups->count(workGroup) && "No shadow for workgroup found!");
    delete (*m_workSpace.workGroups)[workGroup];
    m_workSpace.workGroups->erase(workGroup);
}

TypedValue ShadowContext::getCleanValue(unsigned size)
{
    TypedValue v = {
        size,
        1,
        m_workSpace.memoryPool->alloc(size)
    };

    memset(v.data, 0, size);

    return v;
}

TypedValue ShadowContext::getCleanValue(TypedValue v)
{
    TypedValue c = {
        v.size,
        v.num,
        m_workSpace.memoryPool->alloc(v.size*v.num)
    };

    memset(c.data, 0, v.size*v.num);

    return c;
}

TypedValue ShadowContext::getCleanValue(const llvm::Value *V)
{
    pair<unsigned,unsigned> size = getValueSize(V);
    TypedValue v = {
        size.first,
        size.second,
        m_workSpace.memoryPool->alloc(size.first*size.second)
    };

    memset(v.data, 0, v.size*v.num);

    return v;
}

TypedValue ShadowContext::getCleanValue(const llvm::Type *Ty)
{
    unsigned size = getTypeSize(Ty);
    TypedValue v = {
        size,
        1,
        m_workSpace.memoryPool->alloc(size)
    };

    memset(v.data, 0, v.size);

    return v;
}

TypedValue ShadowContext::getValue(const WorkItem *workItem, const llvm::Value *V) const
{
    if(m_globalValues.count(V))
    {
        return m_globalValues.at(V);
    }
    else
    {
        return m_workSpace.workItems->at(workItem)->getValue(V);
    }
}

TypedValue ShadowContext::getPoisonedValue(unsigned size)
{
    TypedValue v = {
        size,
        1,
        m_workSpace.memoryPool->alloc(size)
    };

    memset(v.data, -1, size);

    return v;
}

TypedValue ShadowContext::getPoisonedValue(TypedValue v)
{
    TypedValue p = {
        v.size,
        v.num,
        m_workSpace.memoryPool->alloc(v.size*v.num)
    };

    memset(p.data, -1, v.size*v.num);

    return p;
}

TypedValue ShadowContext::getPoisonedValue(const llvm::Value *V)
{
    pair<unsigned,unsigned> size = getValueSize(V);
    TypedValue v = {
        size.first,
        size.second,
        m_workSpace.memoryPool->alloc(size.first*size.second)
    };

    memset(v.data, -1, v.size*v.num);

    return v;
}

TypedValue ShadowContext::getPoisonedValue(const llvm::Type *Ty)
{
    unsigned size = getTypeSize(Ty);
    TypedValue v = {
        size,
        1,
        m_workSpace.memoryPool->alloc(size)
    };

    memset(v.data, -1, v.size);

    return v;
}

void ShadowMemory::allocate(size_t address, size_t size)
{
    size_t index = extractBuffer(address);

    if(m_map.count(index))
    {
        deallocate(address);
    }

    Buffer *buffer = new Buffer();
    buffer->size   = size;
    buffer->flags  = 0;
    buffer->data   = new unsigned char[size];

    m_map[index] = buffer;
}

void ShadowMemory::deallocate(size_t address)
{
    size_t index = extractBuffer(address);

    assert(m_map.count(index) && "Cannot deallocate non existing memory!");

    delete[] m_map.at(index)->data;
    delete m_map.at(index);
    m_map.at(index) = NULL;
}

void ShadowMemory::clear()
{
    MemoryMap::iterator mItr;
    for(mItr = m_map.begin(); mItr != m_map.end(); ++mItr)
    {
        delete[] mItr->second->data;
        delete mItr->second;
    }
}

void ShadowContext::dump(const WorkItem *workItem) const
{
    dumpGlobalValues();
    m_globalMemory->dump();
    if(m_workSpace.workGroups && m_workSpace.workGroups->size())
    {
        m_workSpace.workGroups->begin()->second->dump();
    }
    if(m_workSpace.workItems && m_workSpace.workItems->size())
    {
        if(workItem)
        {
            cout << "Item " << workItem->getGlobalID() << endl;
            m_workSpace.workItems->at(workItem)->dump();
        }
        else
        {
            ShadowItemMap::const_iterator itr;
            for(itr = m_workSpace.workItems->begin(); itr != m_workSpace.workItems->end(); ++itr)
            {
                cout << "Item " << itr->first->getGlobalID() << endl;
                itr->second->dump();
            }
        }
    }
}

void ShadowWorkItem::dump() const
{
    if(!m_values.empty())
    {
        m_values.top()->dump();
    }
    dumpMemory();
}

void ShadowContext::dumpGlobalValues() const
{
    cout << "==== ShadowMap (global) =======" << endl;

    UnorderedTypedValueMap::const_iterator itr;
    unsigned num = 1;

    for(itr = m_globalValues.begin(); itr != m_globalValues.end(); ++itr)
    {
        if(itr->first->hasName())
        {
            cout << "%" << itr->first->getName().str() << ": " << itr->second << endl;
        }
        else
        {
            cout << "%" << dec << num++ << ": " << itr->second << endl;
        }
    }

    cout << "=======================" << endl;
}

void ShadowMemory::dump() const
{
    cout << "====== ShadowMem (" << getAddressSpaceName(m_addrSpace) << ") ======";

    for(unsigned b = 0, o = 1; b < m_map.size(); o++)
    {
        if(!m_map.count(b+o))
        {
            continue;
        }

        for(unsigned i = 0; i < m_map.at(b+o)->size; i++)
        {
            if (i%4 == 0)
            {
                cout << endl << hex << uppercase
                    << setw(16) << setfill(' ') << right
                    << ((((size_t)b+o)<<m_numBitsAddress) | i) << ":";
            }
            cout << " " << hex << uppercase << setw(2) << setfill('0')
                << (int)m_map.at(b+o)->data[i];
        }

        ++b;
        o = 0;
    }
    cout << endl;

    cout << "=======================" << endl;
}

ShadowValues::ShadowValues()
{
}

ShadowValues::~ShadowValues()
{
}

void ShadowValues::dump() const
{
    cout << "==== ShadowMap (private) =======" << endl;

#ifdef DUMP_SHADOW
    std::list<const llvm::Value*>::const_iterator itr;
    unsigned num = 1;

    for(itr = m_valuesList.begin(); itr != m_valuesList.end(); ++itr)
    {
        if((*itr)->hasName())
        {
            cout << "%" << (*itr)->getName().str() << ": " << m_values.at(*itr) << endl;
        }
        else
        {
            cout << "%" << dec << num++ << ": " << m_values.at(*itr) << endl;
        }
    }
#else
    cout << endl << "Dump not activated!" << endl;
#endif

    cout << "=======================" << endl;
}

size_t ShadowMemory::extractBuffer(size_t address) const
{
    return (address >> m_numBitsAddress);
}

size_t ShadowMemory::extractOffset(size_t address) const
{
    return (address & (((size_t)-1) >> m_numBitsBuffer));
}

void* ShadowMemory::getPointer(size_t address) const
{
    size_t index = extractBuffer(address);
    size_t offset= extractOffset(address);

    assert(m_map.count(index) && "No shadow memory found!");

    return m_map.at(index)->data + offset;
}

bool ShadowMemory::isAddressValid(size_t address, size_t size) const
{
    size_t index = extractBuffer(address);
    size_t offset = extractOffset(address);
    return m_map.count(index) && (offset + size <= m_map.at(index)->size);
}

TypedValue ShadowValues::getValue(const llvm::Value *V) const
{
    if (llvm::isa<llvm::Instruction>(V)) {
        // For instructions the shadow is already stored in the map.
        assert(m_values.count(V) && "No shadow for instruction value");
        return m_values.at(V);
    }
    else if (llvm::isa<llvm::UndefValue>(V)) {
        return ShadowContext::getPoisonedValue(V);
    }
    else if (llvm::isa<llvm::Argument>(V)) {
        // For arguments the shadow is already stored in the map.
        assert(m_values.count(V) && "No shadow for argument value");
        return m_values.at(V);
    }
    else
    {
        // For everything else the shadow is zero.
        return ShadowContext::getCleanValue(V);
    }
}

void ShadowMemory::load(unsigned char *dst, size_t address, size_t size) const
{
    size_t index = extractBuffer(address);
    size_t offset = extractOffset(address);

    if(isAddressValid(address, size))
    {
        assert(m_map.count(index) && "No shadow memory found!");
        memcpy(dst, m_map.at(index)->data + offset, size);
    }
    else
    {
        TypedValue v = ShadowContext::getPoisonedValue(size);
        memcpy(dst, v.data, size);
    }
}

void ShadowMemory::lock(size_t address) const
{
    size_t offset = extractOffset(address);
    ATOMIC_MUTEX(offset).lock();
}

bool ShadowContext::isCleanValue(TypedValue v)
{
    return (ShadowContext::getCleanValue(v) == v);
}

bool ShadowContext::isCleanValue(TypedValue v, unsigned offset)
{
    TypedValue c = ShadowContext::getCleanValue(v.size);
    return !memcmp(c.data + offset*v.size, v.data + offset*v.size, v.size);
}

void ShadowContext::setGlobalValue(const llvm::Value *V, TypedValue SV)
{
    assert(!m_globalValues.count(V) && "Values may only have one shadow");
    m_globalValues[V] = SV;
}

void ShadowValues::setValue(const llvm::Value *V, TypedValue SV)
{
#ifdef DUMP_SHADOW
    if(!m_values.count(V))
    {
        m_valuesList.push_back(V);
    }
    else
    {
        cout << "Shadow for value " << V->getName().str() << " reset!" << endl;
    }
#endif
    m_values[V] = SV;
}

void ShadowMemory::store(const unsigned char *src, size_t address, size_t size)
{
    size_t index = extractBuffer(address);
    size_t offset = extractOffset(address);

    if(isAddressValid(address, size))
    {
        assert(m_map.count(index) && "Cannot store to unallocated memory!");
        memcpy(m_map.at(index)->data + offset, src, size);
    }
}

void ShadowMemory::unlock(size_t address) const
{
    size_t offset = extractOffset(address);
    ATOMIC_MUTEX(offset).unlock();
}
