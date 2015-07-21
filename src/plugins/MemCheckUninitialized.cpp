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
#include "core/Kernel.h"
#include "core/KernelInvocation.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Type.h"

#include "MemCheckUninitialized.h"

using namespace oclgrind;
using namespace std;

//void MemCheckUninitialized::memoryAllocated(const Memory *memory, size_t address,
//                                 size_t size, cl_mem_flags flags,
//                                 const uint8_t *initData)
//{
//    cout << "Memory: " << memory << ", address: " << hex << address << dec << ", size: " << size << endl;
//}

MemCheckUninitialized::MemCheckUninitialized(const Context *context)
 : Plugin(context), ShadowContext(sizeof(size_t)==8 ? 32 : 16)
{
}

void MemCheckUninitialized::kernelBegin(const KernelInvocation *kernelInvocation)
{
    const Kernel *kernel = kernelInvocation->getKernel();

    // Initialise kernel arguments and global variables
    for (auto value = kernel->values_begin(); value != kernel->values_end(); value++)
    {
        if(const llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(value->first))
        {
            if(A->getType()->isSized())
            {
                ShadowContext.setValue(A, ShadowContext::getCleanValue(A));
            }
        }
        else
        {
            //pair<unsigned,unsigned> size = getValueSize(value->first);
            //TypedValue v = {
            //    size.first,
            //    size.second,
            //    m_pool.alloc(size.first*size.second)
            //};

            const llvm::Type *type = value->first->getType();
            if (type->isPointerTy() && type->getPointerAddressSpace() == AddrSpacePrivate)
            {
                size_t address = value->second.getPointer();
                //TODO: Clean or poisoned?
                storeShadowMemory(AddrSpacePrivate, address, ShadowContext::getCleanValue(value->first));
                //TODO: Do I have to set the shadow?
                ShadowContext.setValue(value->first, ShadowContext::getCleanValue(value->first));
            }
            else if(type->getPointerAddressSpace() == AddrSpaceConstant)
            {
                //TODO: Do I have to set the shadow?
                ShadowContext.setValue(value->first, ShadowContext::getCleanValue(value->first));
            }
            //else if (type->isPointerTy() &&
            //        type->getPointerAddressSpace() == AddrSpaceLocal)
            //{
            //    v.setPointer(m_workGroup->getLocalMemoryAddress(value->first));
            //}
            else
            {
                value->first->dump();
                FATAL_ERROR("Missing initialization");
            //    memcpy(v.data, value->second.data, v.size*v.num);
            }
        }
    }

    ShadowContext.dump();
}

void MemCheckUninitialized::instructionExecuted(const WorkItem *workItem,
                                        const llvm::Instruction *instruction,
                                        const TypedValue& result)
{
    cout << "++++++++++++++++++++++++++++++++++++++++++++" << endl;
    instruction->dump();

    switch(instruction->getOpcode())
    {
        case llvm::Instruction::Add:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::Alloca:
        {
            const llvm::AllocaInst *allocaInst = ((const llvm::AllocaInst*)instruction);

            size_t address = result.getPointer();

            ShadowContext.setValue(instruction, ShadowContext::getCleanValue(instruction));

            TypedValue v = ShadowContext::getPoisonedValue(allocaInst->getAllocatedType());
            storeShadowMemory(AddrSpacePrivate, address, v);
            break;
        }
        case llvm::Instruction::And:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::AShr:
        {
            TypedValue S0 = ShadowContext.getValue(instruction->getOperand(0));
            TypedValue S1 = ShadowContext.getValue(instruction->getOperand(1));

            if(S1 != ShadowContext::getCleanValue(instruction->getOperand(1)))
            {
                ShadowContext.setValue(instruction, ShadowContext::getPoisonedValue(instruction));
            }
            else
            {
                TypedValue newShadow = result.clone();
                TypedValue Shift = workItem->getOperand(instruction->getOperand(1));
                uint64_t shiftMask = (S0.num > 1 ? S0.size : max((size_t)S0.size, sizeof(uint32_t))) * 8 - 1;

                for (unsigned i = 0; i < S0.num; i++)
                {
                    newShadow.setUInt(S0.getSInt(i) >> (Shift.getUInt(i) & shiftMask), i);
                }

                ShadowContext.setValue(instruction, newShadow);
            }

            break;
        }
        case llvm::Instruction::BitCast:
        {
            TypedValue shadow = ShadowContext.getValue(instruction->getOperand(0));
            ShadowContext.setValue(instruction, shadow.clone());
            break;
        }
        case llvm::Instruction::Br:
        {
            checkAllOperandsDefined(instruction);
            break;
        }
        case llvm::Instruction::Call:
        {
            const llvm::CallInst *callInst = ((const llvm::CallInst*)instruction);
            const llvm::Function *function = callInst->getCalledFunction();

            // Check for indirect function calls
            if (!callInst->getCalledFunction())
            {
                // Resolve indirect function pointer
                const llvm::Value *func = callInst->getCalledValue();
                const llvm::Value *funcPtr = ((const llvm::User*)func)->getOperand(0);
                function = (const llvm::Function*)funcPtr;
            }

            assert(!function->isVarArg() && "Variadic functions are not supported!");

            // For inline asm, do the usual thing: check argument shadow and mark all
            // outputs as clean. Note that any side effects of the inline asm that are
            // not immediately visible in its constraints are not handled.
            if (callInst->isInlineAsm()) {
                checkAllOperandsDefined(instruction);
                ShadowContext.setValue(instruction, ShadowContext::getCleanValue(instruction));
                break;
            }

            if(const llvm::IntrinsicInst *II = llvm::dyn_cast<const llvm::IntrinsicInst>(instruction))
            {
                handleIntrinsicInstruction(workItem, II);
                break;
            }

            assert(!llvm::isa<const llvm::IntrinsicInst>(instruction) && "intrinsics are handled elsewhere");

            // Fresh memory for function
            ShadowFrame *frame = ShadowContext.createShadowFrame();

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
                    unsigned char *origShadowData = (unsigned char*)ShadowContext.getMemoryPointer(origShadowAddress);
                    size_t size = getTypeSize(argItr->getType()->getPointerElementType());

                    //// Set new shadow memory
                    frame->storeMemory(origShadowData, newShadowAddress, size);
                    frame->setValue(argItr, ShadowContext::getCleanValue(argItr));
                }
                else
                {
                    frame->setValue(argItr, ShadowContext.getValue(Val).clone());
                }
            }

            // Now, get the shadow for the RetVal.
            if(callInst->getType()->isSized())
            {
                frame->pushCall(callInst);
            }

            ShadowContext.push(frame);

            break;
        }
        case llvm::Instruction::ExtractElement:
        {
            const llvm::ExtractElementInst *extractInst = ((const llvm::ExtractElementInst*)instruction);
            
            TypedValue indexShadow = ShadowContext.getValue(extractInst->getIndexOperand());

            if(indexShadow != ShadowContext::getCleanValue(extractInst->getIndexOperand()))
            {
                logUninitializedIndex();
            }

            TypedValue vectorShadow = ShadowContext.getValue(extractInst->getVectorOperand());
            TypedValue newShadow = result.clone();

            unsigned index = workItem->getOperand(extractInst->getIndexOperand()).getUInt();
            memcpy(newShadow.data, vectorShadow.data + newShadow.size*index, newShadow.size);

            ShadowContext.setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::ExtractValue:
        {
            const llvm::ExtractValueInst *extractInst = ((const llvm::ExtractValueInst*)instruction);

            const llvm::Value *Agg = extractInst->getAggregateOperand();
            TypedValue ResShadow = result.clone();

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
            memcpy(ResShadow.data, ShadowContext.getValue(Agg).data + offset, getTypeSize(type));

            ShadowContext.setValue(instruction, ResShadow);
            break;
        }
        case llvm::Instruction::FAdd:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::FCmp:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::FDiv:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::FMul:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::FPExt:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::FPToSI:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::FPToUI:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::FPTrunc:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::FRem:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::FSub:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::GetElementPtr:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::ICmp:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::InsertElement:
        {
            TypedValue indexShadow = ShadowContext.getValue(instruction->getOperand(2));

            if(indexShadow != ShadowContext::getCleanValue(instruction->getOperand(2)))
            {
                logUninitializedIndex();
            }

            TypedValue vectorShadow = ShadowContext.getValue(instruction->getOperand(0));
            TypedValue elementShadow = ShadowContext.getValue(instruction->getOperand(1));
            TypedValue newShadow = result.clone();

            unsigned index = workItem->getOperand(instruction->getOperand(2)).getUInt();
            memcpy(newShadow.data, vectorShadow.data, newShadow.size*newShadow.num);
            memcpy(newShadow.data + index*newShadow.size, elementShadow.data, newShadow.size);

            ShadowContext.setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::InsertValue:
        {
            const llvm::InsertValueInst *insertInst = (const llvm::InsertValueInst*)instruction;

            TypedValue newShadow = result.clone();

            // Load original aggregate data
            const llvm::Value *agg = insertInst->getAggregateOperand();
            memcpy(newShadow.data, ShadowContext.getValue(agg).data, newShadow.size*newShadow.num);

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
            memcpy(newShadow.data + offset, ShadowContext.getValue(value).data, getTypeSize(value->getType()));

            ShadowContext.setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::IntToPtr:
        {
            TypedValue shadow = ShadowContext.getValue(instruction->getOperand(0));
            TypedValue newShadow = result.clone();

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                newShadow.setPointer(shadow.getUInt(i), i);
            }

            ShadowContext.setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::Load:
        {
            assert(instruction->getType()->isSized() && "Load type must have size");
            const llvm::LoadInst *loadInst = ((const llvm::LoadInst*)instruction);
            const llvm::Value *Addr = loadInst->getPointerOperand();

            size_t address = workItem->getOperand(Addr).getPointer();
            unsigned addrSpace = loadInst->getPointerAddressSpace();

            TypedValue v = result.clone();
            loadShadowMemory(addrSpace, address, v);
            ShadowContext.setValue(instruction, v);

//            if (ClCheckAccessAddress)
//                insertShadowCheck(I.getPointerOperand(), &I);
//
//            if (I.isAtomic())
//                I.setOrdering(addAcquireOrdering(I.getOrdering()));

            break;
        }
        case llvm::Instruction::LShr:
        {
            TypedValue S0 = ShadowContext.getValue(instruction->getOperand(0));
            TypedValue S1 = ShadowContext.getValue(instruction->getOperand(1));

            if(S1 != ShadowContext::getCleanValue(instruction->getOperand(1)))
            {
                ShadowContext.setValue(instruction, ShadowContext::getPoisonedValue(instruction));
            }
            else
            {
                TypedValue newShadow = result.clone();
                TypedValue Shift = workItem->getOperand(instruction->getOperand(1));
                uint64_t shiftMask = (S0.num > 1 ? S0.size : max((size_t)S0.size, sizeof(uint32_t))) * 8 - 1;

                for (unsigned i = 0; i < S0.num; i++)
                {
                    newShadow.setUInt(S0.getUInt(i) >> (Shift.getUInt(i) & shiftMask), i);
                }

                ShadowContext.setValue(instruction, newShadow);
            }

            break;
        }
        case llvm::Instruction::Mul:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::Or:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::PHI:
        {
            //FIXME: m_position is private
            //const llvm::PHINode *phiNode = (const llvm::PHINode*)instruction;
            //const llvm::Value *value = phiNode->getIncomingValueForBlock(
            //        (const llvm::BasicBlock*)m_position->prevBlock);

            //TypedValue newShadow = result.clone();

            //memcpy(newShadow.data, getValue(value).data, newShadow.size*newShadow.num);
            //setValue(instruction, newShadow);

            ShadowContext.setValue(instruction, ShadowContext::getCleanValue(instruction));
            break;
        }
        case llvm::Instruction::PtrToInt:
        {
            TypedValue shadow = ShadowContext.getValue(instruction->getOperand(0));
            TypedValue newShadow = result.clone();

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                newShadow.setUInt(shadow.getPointer(i), i);
            }

            ShadowContext.setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::Ret:
        {
            const llvm::ReturnInst *retInst = ((const llvm::ReturnInst*)instruction);
            const llvm::Value *RetVal = retInst->getReturnValue();

            if (!RetVal)
            {
                break;
            }
            //Value *ShadowPtr = getValuePtrForRetval(RetVal, IRB);
            //if (CheckReturnValue) {
            //    insertShadowCheck(RetVal, &I);
            //    Value *Shadow = getCleanValue(RetVal);
            //    IRB.CreateAlignedStore(Shadow, ShadowPtr, kShadowTLSAlignment);
            //} else {
            TypedValue retValShadow = ShadowContext.getValue(RetVal).clone();
            const llvm::CallInst *callInst = ShadowContext.popCall();
            ShadowContext.pop();
            ShadowContext.setValue(callInst, retValShadow);
            //}
            break;
        }
        case llvm::Instruction::SDiv:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::Select:
        {
            const llvm::SelectInst *selectInst = (const llvm::SelectInst*)instruction;

            TypedValue opCondition = workItem->getOperand(selectInst->getCondition());
            TypedValue conditionShadow = ShadowContext.getValue(selectInst->getCondition());
            TypedValue newShadow = result.clone();

            if(conditionShadow != ShadowContext::getCleanValue(selectInst->getCondition()))
            {
                newShadow = ShadowContext::getPoisonedValue(instruction);
            }
            else
            {
                for(unsigned i = 0; i < result.num; i++)
                {
                    const bool cond = selectInst->getCondition()->getType()->isVectorTy() ?
                        opCondition.getUInt(i) :
                        opCondition.getUInt();
                    const llvm::Value *op = cond ?
                        selectInst->getTrueValue() :
                        selectInst->getFalseValue();

                    memcpy(newShadow.data + i*newShadow.size,
                            ShadowContext.getValue(op).data + i*newShadow.size,
                            newShadow.size);
                }
            }

            ShadowContext.setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::SExt:
        {
            const llvm::Value *operand = instruction->getOperand(0);
            TypedValue shadow = ShadowContext.getValue(operand);
            TypedValue newShadow = result.clone();

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                int64_t val = shadow.getSInt(i);
                if (operand->getType()->getPrimitiveSizeInBits() == 1)
                {
                    val = val ? -1 : 0;
                }
                newShadow.setSInt(val, i);
            }

            ShadowContext.setValue(instruction, newShadow);

            break;
        }
        case llvm::Instruction::Shl:
        {
            TypedValue S0 = ShadowContext.getValue(instruction->getOperand(0));
            TypedValue S1 = ShadowContext.getValue(instruction->getOperand(1));

            if(S1 != ShadowContext::getCleanValue(instruction->getOperand(1)))
            {
                ShadowContext.setValue(instruction, ShadowContext::getPoisonedValue(instruction));
            }
            else
            {
                TypedValue newShadow = result.clone();
                TypedValue Shift = workItem->getOperand(instruction->getOperand(1));
                uint64_t shiftMask = (S0.num > 1 ? S0.size : max((size_t)S0.size, sizeof(uint32_t))) * 8 - 1;

                for (unsigned i = 0; i < S0.num; i++)
                {
                    newShadow.setUInt(S0.getUInt(i) << (Shift.getUInt(i) & shiftMask), i);
                }

                ShadowContext.setValue(instruction, newShadow);
            }

            break;
        }
        case llvm::Instruction::ShuffleVector:
        {
            const llvm::ShuffleVectorInst *shuffleInst = (const llvm::ShuffleVectorInst*)instruction;

            TypedValue maskShadow = ShadowContext.getValue(shuffleInst->getMask());

            if(maskShadow != ShadowContext::getCleanValue(shuffleInst->getMask()))
            {
                logUninitializedMask();
            }

            const llvm::Value *v1 = shuffleInst->getOperand(0);
            const llvm::Value *v2 = shuffleInst->getOperand(1);
            TypedValue mask = workItem->getOperand(shuffleInst->getMask());
            TypedValue newShadow = result.clone();

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

                memcpy(newShadow.data + i*newShadow.size, ShadowContext.getValue(src).data + index*newShadow.size, newShadow.size);
            }

            ShadowContext.setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::SIToFP:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::SRem:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::Store:
        {
            const llvm::StoreInst *storeInst = ((const llvm::StoreInst*)instruction);
            const llvm::Value *Val = storeInst->getValueOperand();
            const llvm::Value *Addr = storeInst->getPointerOperand();

            size_t address = workItem->getOperand(Addr).getPointer();
            unsigned addrSpace = storeInst->getPointerAddressSpace();

            TypedValue shadowVal = storeInst->isAtomic() ? ShadowContext::getCleanValue(Val) : ShadowContext.getValue(Val);

            if(addrSpace == AddrSpaceGlobal)
            {
                if(shadowVal != ShadowContext::getCleanValue(Val))
                {
                    logUninitializedWrite(addrSpace, address);
                }
            }

            storeShadowMemory(addrSpace, address, shadowVal);
            break;
        }
        case llvm::Instruction::Sub:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::Switch:
        {
            checkAllOperandsDefined(instruction);
            break;
        }
        case llvm::Instruction::Trunc:
        {
            TypedValue shadow = ShadowContext.getValue(instruction->getOperand(0));
            TypedValue newShadow = result.clone();

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                memcpy(newShadow.data+i*newShadow.size, shadow.data+i*shadow.size, newShadow.size);
            }

            ShadowContext.setValue(instruction, newShadow);
            break;
        }
        case llvm::Instruction::UDiv:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::UIToFP:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::URem:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::Unreachable:
            FATAL_ERROR("Encountered unreachable instruction");
        case llvm::Instruction::Xor:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::ZExt:
        {
            TypedValue shadow = ShadowContext.getValue(instruction->getOperand(0));
            TypedValue newShadow = result.clone();

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                newShadow.setUInt(shadow.getUInt(i), i);
            }

            ShadowContext.setValue(instruction, newShadow);
            break;
        }
        default:
            FATAL_ERROR("Unsupported instruction: %s", instruction->getOpcodeName());
    }

    ShadowContext.dump();
}

void MemCheckUninitialized::SimpleOr(const llvm::Instruction *I)
{
    for(llvm::Instruction::const_op_iterator OI = I->op_begin(); OI != I->op_end(); ++OI)
    {
        if(ShadowContext.getValue(OI->get()) != ShadowContext::getCleanValue(OI->get()))
        {
            ShadowContext.setValue(I, ShadowContext::getPoisonedValue(I));
            return;
        }
    }

    ShadowContext.setValue(I, ShadowContext::getCleanValue(I));
}

void MemCheckUninitialized::storeShadowMemory(unsigned addrSpace, size_t address, TypedValue SM)
{
    // Only write to private memory as these others are always clean
    if(addrSpace == AddrSpacePrivate)
    {
        ShadowContext.storeMemory(SM.data, address, SM.size*SM.num);
    }
}

void MemCheckUninitialized::loadShadowMemory(unsigned addrSpace, size_t address, TypedValue &SM)
{
    // Assume global memory is always clean!
    if(addrSpace != AddrSpacePrivate)
    {
        memset(SM.data, 0, SM.size*SM.num);
    }
    else
    {
        ShadowContext.loadMemory(SM.data, address, SM.size*SM.num);
    }
}

void MemCheckUninitialized::checkAllOperandsDefined(const llvm::Instruction *I)
{
    for(llvm::Instruction::const_op_iterator OI = I->op_begin(); OI != I->op_end(); ++OI)
    {
        if(ShadowContext.getValue(OI->get()) != ShadowContext::getCleanValue(OI->get()))
        {
            logUninitializedCF();
            return;
        }
    }
}

void MemCheckUninitialized::copyShadowMemory(unsigned dstAddrSpace, size_t dst, unsigned srcAddrSpace, size_t src, size_t size)
{
    if(dstAddrSpace == AddrSpacePrivate)
    {
        unsigned char *buffer = m_pool.alloc(size);

        if(srcAddrSpace != AddrSpacePrivate)
        {
            //Global memory is always clean
            memset(buffer, 0, size);
        }
        else
        {
            ShadowContext.loadMemory(buffer, src, size);
        }

        ShadowContext.storeMemory(buffer, dst, size);
    }
}

void MemCheckUninitialized::handleIntrinsicInstruction(const WorkItem *workItem, const llvm::IntrinsicInst *I)
{
    switch (I->getIntrinsicID())
    {
        case llvm::Intrinsic::memcpy:
        {
            const llvm::MemCpyInst *memcpyInst = (const llvm::MemCpyInst*)I;
            size_t dst = workItem->getOperand(memcpyInst->getDest()).getPointer();
            size_t src = workItem->getOperand(memcpyInst->getSource()).getPointer();
            size_t size = workItem->getOperand(memcpyInst->getLength()).getUInt();
            unsigned dstAddrSpace = memcpyInst->getDestAddressSpace();
            unsigned srcAddrSpace = memcpyInst->getSourceAddressSpace();

            copyShadowMemory(dstAddrSpace, dst, srcAddrSpace, src, size);
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

MemoryPool ShadowContext::m_pool;

ShadowContext::ShadowContext(unsigned bufferBits) :
    m_numBitsBuffer(bufferBits), m_stack()
{
    ShadowFrame *frame = createShadowFrame();
    push(frame);
}

ShadowContext::~ShadowContext()
{
    while(!m_stack.empty())
    {
        ShadowFrame *frame = m_stack.top();
        m_stack.pop();
        delete frame;
    }
}

ShadowFrame* ShadowContext::createShadowFrame()
{
    return new ShadowFrame(m_numBitsBuffer);
}

TypedValue ShadowContext::getCleanValue(const llvm::Value *V)
{
    pair<unsigned,unsigned> size = getValueSize(V);
    TypedValue v = {
        size.first,
        size.second,
        m_pool.alloc(size.first*size.second)
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
        m_pool.alloc(size)
    };

    memset(v.data, 0, v.size);

    return v;
}

TypedValue ShadowContext::getPoisonedValue(const llvm::Value *V)
{
    pair<unsigned,unsigned> size = getValueSize(V);
    TypedValue v = {
        size.first,
        size.second,
        m_pool.alloc(size.first*size.second)
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
        m_pool.alloc(size)
    };

    memset(v.data, -1, v.size);

    return v;
}

ShadowFrame::ShadowFrame(unsigned bufferBits) :
    m_numBitsAddress((sizeof(size_t)<<3) - bufferBits), m_numBitsBuffer(bufferBits)
{
}

ShadowFrame::~ShadowFrame()
{
    clearMemory();
}

void ShadowFrame::allocateMemory(size_t address, size_t size)
{
    size_t index = extractBuffer(address);

#ifdef ALLOW_DUMP
    m_memory[index] = std::pair<size_t, unsigned char*>(size, new unsigned char[size]);
#else
    m_memory[index] = new unsigned char[size];
#endif
}

void ShadowFrame::clearMemory()
{
    MemoryMap::iterator mItr;
    for(mItr = m_memory.begin(); mItr != m_memory.end(); ++mItr)
    {
#ifdef ALLOW_DUMP
        delete[] mItr->second.second;
#else
        delete[] mItr->second;
#endif
    }
}

void ShadowFrame::dump() const
{
    dumpValues();
    dumpMemory();
}

void ShadowFrame::dumpMemory() const
{
    cout << "====== ShadowMem ======";

#ifdef ALLOW_DUMP
    MemoryMap::const_iterator itr;
    for(itr = m_memory.begin(); itr != m_memory.end(); ++itr)
    {
        if(!itr->second.second)
        {
            continue;
        }

        for(unsigned i = 0; i < itr->second.first; i++)
        {
            if (i%4 == 0)
            {
                cout << endl << hex << uppercase
                    << setw(16) << setfill(' ') << right
                    << ((((size_t)itr->first)<<m_numBitsAddress) | i) << ":";
            }
            cout << " " << hex << uppercase << setw(2) << setfill('0')
                << (int)itr->second.second[i];
        }
    }
    cout << endl;
#else
    cout << endl << "Dump not activated!" << endl;
#endif

    cout << "=======================" << endl;
}

void ShadowFrame::dumpValues() const
{
    std::list<const llvm::Value*>::const_iterator itr;

    cout << "==== ShadowMap =======" << endl;

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

    cout << "=======================" << endl;
}

size_t ShadowFrame::extractBuffer(size_t address) const
{
    return (address >> m_numBitsAddress);
}

size_t ShadowFrame::extractOffset(size_t address) const
{
    return (address & (((size_t)-1) >> m_numBitsBuffer));
}

void* ShadowFrame::getMemoryPointer(size_t address) const
{
    size_t index = extractBuffer(address);
    size_t offset= extractOffset(address);

    assert(m_memory.count(index) && "No shadow memory found!");

#ifdef ALLOW_DUMP
    return m_memory.at(index).second + offset;
#else
    return m_memory.at(index) + offset;
#endif
}

TypedValue ShadowFrame::getValue(const llvm::Value *V) const
{
    if (llvm::isa<llvm::Instruction>(V)) {
        // For instructions the shadow is already stored in the map.
        assert(m_values.count(V) && "No shadow for a value");
        return m_values.at(V);
    }
    else if (llvm::isa<llvm::UndefValue>(V)) {
        return ShadowContext::getPoisonedValue(V);
    }
    else if (llvm::isa<llvm::Argument>(V)) {
        // For arguments the shadow is already stored in the map.
        assert(m_values.count(V) && "No shadow for a value");
        return m_values.at(V);
    }
    else
    {
        // For everything else the shadow is zero.
        return ShadowContext::getCleanValue(V);
    }
}

void ShadowFrame::loadMemory(unsigned char *dst, size_t address, size_t size) const
{
    size_t index = extractBuffer(address);
    size_t offset = extractOffset(address);

    assert(m_memory.count(index) && "No shadow memory found!");

#ifdef ALLOW_DUMP
    unsigned char *src = m_memory.at(index).second;
#else
    unsigned char *src = m_memory.at(index);
#endif

    memcpy(dst, src + offset, size);
}

void ShadowFrame::setValue(const llvm::Value *V, TypedValue SV)
{
  assert(!m_values.count(V) && "Values may only have one shadow");
  m_values[V] = SV;
  m_valuesList.push_back(V);
}

void ShadowFrame::storeMemory(const unsigned char *src, size_t address, size_t size)
{
    size_t index = extractBuffer(address);
    size_t offset = extractOffset(address);

    if(!m_memory.count(index))
    {
        allocateMemory(address, size);
    }

#ifdef ALLOW_DUMP
    unsigned char *dst = m_memory[index].second;
#else
    unsigned char *dst = m_memory[index];
#endif

    memcpy(dst + offset, src, size);
}
