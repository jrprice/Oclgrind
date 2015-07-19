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

#include "llvm/IR/LLVMContext.h"
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

void MemCheckUninitialized::dumpFunctionArgumentMap()
{
    std::map<const llvm::Function*, std::map<unsigned, TypedValue> >::iterator itr;

    cout << "==== Arguments ======" << endl;

    for(itr = FunctionArgumentMap.begin(); itr != FunctionArgumentMap.end(); ++itr)
    {
        cout << "F: " << itr->first->getName().str() << endl;

        std::map<unsigned, TypedValue>::iterator itr2;

        for(itr2 = itr->second.begin(); itr2 != itr->second.end(); ++itr2)
        {
            cout << "   " << itr2->first << ": " << itr2->second << endl;
        }
    }

    cout << "=======================" << endl;
}

void MemCheckUninitialized::dumpShadowMap()
{
    std::list<const llvm::Value*>::iterator itr;

    cout << "==== ShadowMap =======" << endl;

    unsigned num = 1;

    for(itr = ShadowList.begin(); itr != ShadowList.end(); ++itr)
    {
        if((*itr)->hasName())
        {
            cout << "%" << (*itr)->getName().str() << ": " << ShadowMap.at(*itr) << endl;
        }
        else
        {
            cout << "%" << dec << num++ << ": " << ShadowMap.at(*itr) << endl;
        }
    }

    cout << "=======================" << endl;
}

void MemCheckUninitialized::dumpShadowMem(unsigned addrSpace)
{
    cout << "====== ShadowMem(" << getAddressSpaceName(addrSpace) << ") ======";

    getMemory(addrSpace)->dump();

    cout << "=======================" << endl;
}

MemCheckUninitialized::MemCheckUninitialized(const Context *context)
 : Plugin(context), ShadowMap()
{
    ShadowMem[AddrSpacePrivate] = new Memory(AddrSpacePrivate, sizeof(size_t)==8 ? 32 : 16, context);
    ShadowMem[AddrSpaceGlobal] = new Memory(AddrSpaceGlobal, sizeof(size_t)==8 ? 16 : 8, context);
    llvmContext = new llvm::LLVMContext();
}

void MemCheckUninitialized::kernelBegin(const KernelInvocation *kernelInvocation)
{
    const Kernel *kernel = kernelInvocation->getKernel();
    unsigned ArgNum = 0;

    // Initialise kernel arguments and global variables
    for (auto value = kernel->values_begin(); value != kernel->values_end(); value++)
    {
        if(const llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(value->first))
        {
            llvm::Type *argType = A->getType();

            if(argType->isSized())
            {
                FunctionArgumentMap[A->getParent()][ArgNum] = getCleanShadow(A);

                if(argType->isPointerTy())
                {
                    llvm::Type *eltType = argType->getPointerElementType();
                    unsigned Size = A->hasByValAttr() ? getTypeSize(eltType) : getTypeSize(argType);
                    size_t address = getMemory(AddrSpaceGlobal)->allocateBuffer(Size);
                    //TODO: Clean or poisoned?
                    setShadowMem(AddrSpaceGlobal, address, getCleanShadow(eltType));
                }

                ++ArgNum;
            }
        }
        else
        {
            pair<unsigned,unsigned> size = getValueSize(value->first);
            //TypedValue v = {
            //    size.first,
            //    size.second,
            //    m_pool.alloc(size.first*size.second)
            //};

            const llvm::Type *type = value->first->getType();
            if (type->isPointerTy() && type->getPointerAddressSpace() == AddrSpacePrivate)
            {
                size_t sz = value->second.size*value->second.num;
                size_t address = getMemory(AddrSpacePrivate)->allocateBuffer(sz);
                //TODO: Clean or poisoned?
                setShadowMem(AddrSpacePrivate, address, getCleanShadow(value->first));
            }
            //else if (type->isPointerTy() &&
            //        type->getPointerAddressSpace() == AddrSpaceLocal)
            //{
            //    v.setPointer(m_workGroup->getLocalMemoryAddress(value->first));
            //}
            //else
            //{
            //    memcpy(v.data, value->second.data, v.size*v.num);
            //}
        }
    }
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

            setShadow(instruction, getCleanShadow(instruction));

            TypedValue v = getPoisonedShadow(allocaInst->getAllocatedType());
            getMemory(AddrSpacePrivate)->allocateBuffer(v.size*v.num);
            setShadowMem(AddrSpacePrivate, address, v);
            break;
        }
        case llvm::Instruction::And:
        {
            SimpleOr(instruction);
            break;
        }
        case llvm::Instruction::AShr:
        {
            TypedValue S0 = getShadow(instruction->getOperand(0));
            TypedValue S1 = getShadow(instruction->getOperand(1));

            if(S1 != getCleanShadow(instruction->getOperand(1)))
            {
                setShadow(instruction, getPoisonedShadow(instruction));
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

                setShadow(instruction, newShadow);
            }

            break;
        }
        case llvm::Instruction::BitCast:
        {
            TypedValue shadow = getShadow(instruction->getOperand(0));
            setShadow(instruction, shadow.clone());
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

            // For inline asm, do the usual thing: check argument shadow and mark all
            // outputs as clean. Note that any side effects of the inline asm that are
            // not immediately visible in its constraints are not handled.
            if (callInst->isInlineAsm()) {
                //TODO: Do something!
                //visitInstruction(I);
                break;
            }

            if(const llvm::IntrinsicInst *II = llvm::dyn_cast<const llvm::IntrinsicInst>(instruction))
            {
                handleIntrinsicInstruction(II);
                break;
            }

            assert(!llvm::isa<const llvm::IntrinsicInst>(instruction) && "intrinsics are handled elsewhere");

            unsigned ArgNum = 0;

            for (auto &U : callInst->arg_operands())
            {
                llvm::Value *Val = U.get();

                if (!Val->getType()->isSized())
                {
                    ++ArgNum;
                    continue;
                }
                unsigned Size = 0;
                llvm::Value *Store = nullptr;
                // Compute the Shadow for arg even if it is ByVal, because
                // in that case getShadow() will copy the actual arg shadow to
                // FunctionArgumentMap
                //FIXME: Clone shadow?
                TypedValue ArgShadow = getShadow(Val);
                //Value *ArgShadowBase = getShadowPtrForArgument(A, IRB, ArgOffset);
                
                if(callInst->paramHasAttr(ArgNum, llvm::Attribute::ByVal))
                {
                    assert(Val->getType()->isPointerTy() && "ByVal argument is not a pointer!");
                    Size = getTypeSize(Val->getType()->getPointerElementType());
                    
                    //size_t address = workItem->getOperand(A).getPointer();

                    //TODO: Copy memory
                    //TypedValue v;
                    //getShadowMem(AddrSpacePrivate, address, v);
                    //setShadowMem(AddrSpacePrivate, address, v);

                    //Store = IRB.CreateMemCpy(ArgShadowBase,
                    //        getShadowPtr(A, Type::getInt8Ty(*MS.C), IRB),
                    //        Size, Alignment);
                }
                else
                {
                    //FIXME:Recursive calls break
                    FunctionArgumentMap[callInst->getCalledFunction()][ArgNum] = ArgShadow;
                }

                ++ArgNum;
            }

            //FunctionType *FT =
            //    cast<FunctionType>(CS.getCalledValue()->getType()->getContainedType(0));
            //if (FT->isVarArg()) {
            //    VAHelper->visitCallSite(CS, IRB);
            //}

            // Now, get the shadow for the RetVal.
            if(!callInst->getType()->isSized())
            {
                return;
            }

            // Until we have full dynamic coverage, make sure the retval shadow is 0.
            //Value *Base = getShadowPtrForRetval(&I, IRBBefore);
            //IRBBefore.CreateAlignedStore(getCleanShadow(&I), Base, kShadowTLSAlignment);
            CallInstructions.push_back(callInst);
            //setShadow(callInst, getCleanShadow(callInst));

            //if (CS.isCall()) {
            //} else {
            //    BasicBlock *NormalDest = cast<InvokeInst>(&I)->getNormalDest();
            //    if (!NormalDest->getSinglePredecessor()) {
            //        // FIXME: this case is tricky, so we are just conservative here.
            //        // Perhaps we need to split the edge between this BB and NormalDest,
            //        // but a naive attempt to use SplitEdge leads to a crash.
            //        setShadow(&I, getCleanShadow(&I));
            //        setOrigin(&I, getCleanOrigin());
            //        return;
            //    }
            //    NextInsn = NormalDest->getFirstInsertionPt();
            //    assert(NextInsn &&
            //            "Could not find insertion point for retval shadow load");
            //}
            break;
        }
        case llvm::Instruction::ExtractElement:
        {
            const llvm::ExtractElementInst *extractInst = ((const llvm::ExtractElementInst*)instruction);
            
            TypedValue indexShadow = getShadow(extractInst->getIndexOperand());

            if(indexShadow != getCleanShadow(extractInst->getIndexOperand()))
            {
                logError(1, 0);
            }

            TypedValue vectorShadow = getShadow(extractInst->getVectorOperand());
            TypedValue newShadow = result.clone();

            unsigned index = workItem->getOperand(extractInst->getIndexOperand()).getUInt();
            memcpy(newShadow.data, vectorShadow.data + newShadow.size*index, newShadow.size);

            setShadow(instruction, newShadow);
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
            memcpy(ResShadow.data, getShadow(Agg).data + offset, getTypeSize(type));

            setShadow(instruction, ResShadow);
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
            TypedValue indexShadow = getShadow(instruction->getOperand(2));

            if(indexShadow != getCleanShadow(instruction->getOperand(2)))
            {
                logError(1, 0);
            }

            TypedValue vectorShadow = getShadow(instruction->getOperand(0));
            TypedValue elementShadow = getShadow(instruction->getOperand(1));
            TypedValue newShadow = result.clone();

            unsigned index = workItem->getOperand(instruction->getOperand(2)).getUInt();
            memcpy(newShadow.data, vectorShadow.data, newShadow.size*newShadow.num);
            memcpy(newShadow.data + index*newShadow.size, elementShadow.data, newShadow.size);

            setShadow(instruction, newShadow);
            break;
        }
//        case llvm::Instruction::InsertValue:
//          insertval(instruction, result);
//          break;
        case llvm::Instruction::IntToPtr:
        {
            TypedValue shadow = getShadow(instruction->getOperand(0));
            TypedValue newShadow = result.clone();

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                newShadow.setPointer(shadow.getUInt(i), i);
            }

            setShadow(instruction, newShadow);
            break;
        }
        case llvm::Instruction::Load:
        {
            assert(instruction->getType()->isSized() && "Load type must have size");
            const llvm::LoadInst *loadInst = ((const llvm::LoadInst*)instruction);
            const llvm::Value *Addr = loadInst->getPointerOperand();

            size_t address = workItem->getOperand(Addr).getPointer();
            unsigned addrSpace = loadInst->getPointerAddressSpace();

            TypedValue v = {
                result.size,
                result.num,
                m_pool.alloc(result.size*result.num)
            };

            //FIXME: Does not work for function arguments
            getShadowMem(addrSpace, address, v);
            setShadow(instruction, v);

//            if (ClCheckAccessAddress)
//                insertShadowCheck(I.getPointerOperand(), &I);
//
//            if (I.isAtomic())
//                I.setOrdering(addAcquireOrdering(I.getOrdering()));

            break;
        }
        case llvm::Instruction::LShr:
        {
            TypedValue S0 = getShadow(instruction->getOperand(0));
            TypedValue S1 = getShadow(instruction->getOperand(1));

            if(S1 != getCleanShadow(instruction->getOperand(1)))
            {
                setShadow(instruction, getPoisonedShadow(instruction));
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

                setShadow(instruction, newShadow);
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
//        case llvm::Instruction::PHI:
//          phi(instruction, result);
//          break;
        case llvm::Instruction::PtrToInt:
        {
            TypedValue shadow = getShadow(instruction->getOperand(0));
            TypedValue newShadow = result.clone();

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                newShadow.setUInt(shadow.getPointer(i), i);
            }

            setShadow(instruction, newShadow);
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
            //Value *ShadowPtr = getShadowPtrForRetval(RetVal, IRB);
            //if (CheckReturnValue) {
            //    insertShadowCheck(RetVal, &I);
            //    Value *Shadow = getCleanShadow(RetVal);
            //    IRB.CreateAlignedStore(Shadow, ShadowPtr, kShadowTLSAlignment);
            //} else {
            const llvm::CallInst *callInst = CallInstructions.back();
            CallInstructions.pop_back();
            setShadow(callInst, getShadow(RetVal).clone());
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
            TypedValue conditionShadow = getShadow(selectInst->getCondition());
            TypedValue newShadow = result.clone();

            if(conditionShadow != getCleanShadow(selectInst->getCondition()))
            {
                newShadow = getPoisonedShadow(instruction);
            }
            else
            {
                for (unsigned i = 0; i < result.num; i++)
                {
                    const bool cond = selectInst->getCondition()->getType()->isVectorTy() ?
                        opCondition.getUInt(i) :
                        opCondition.getUInt();
                    const llvm::Value *op = cond ?
                        selectInst->getTrueValue() :
                        selectInst->getFalseValue();

                    memcpy(newShadow.data + i*newShadow.size,
                            getShadow(op).data + i*newShadow.size,
                            newShadow.size);
                }
            }

            setShadow(instruction, newShadow);
            break;
        }
        case llvm::Instruction::SExt:
        {
            const llvm::Value *operand = instruction->getOperand(0);
            TypedValue shadow = getShadow(operand);
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

            setShadow(instruction, newShadow);

            break;
        }
        case llvm::Instruction::Shl:
        {
            TypedValue S0 = getShadow(instruction->getOperand(0));
            TypedValue S1 = getShadow(instruction->getOperand(1));

            if(S1 != getCleanShadow(instruction->getOperand(1)))
            {
                setShadow(instruction, getPoisonedShadow(instruction));
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

                setShadow(instruction, newShadow);
            }

            break;
        }
        case llvm::Instruction::ShuffleVector:
        {
            const llvm::ShuffleVectorInst *shuffleInst = (const llvm::ShuffleVectorInst*)instruction;

            TypedValue maskShadow = getShadow(shuffleInst->getMask());

            if(maskShadow != getCleanShadow(shuffleInst->getMask()))
            {
                logError(1, 0);
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

                memcpy(newShadow.data + i*newShadow.size, getShadow(src).data + index*newShadow.size, newShadow.size);
            }

            setShadow(instruction, newShadow);
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

            if(addrSpace == AddrSpaceGlobal)
            {
                if(getShadow(Val) != getCleanShadow(Val))
                {
                    //TODO:Better warning
                    logError(addrSpace, address);
                }

                //TODO: What about this?
                //setShadowMem(address, getCleanShadow(Val));
            }
            else
            {
                TypedValue shadowVal = storeInst->isAtomic() ? getCleanShadow(Val) : getShadow(Val);
                setShadowMem(addrSpace, address, shadowVal);
            }
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
            TypedValue shadow = getShadow(instruction->getOperand(0));
            TypedValue newShadow = result.clone();

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                memcpy(newShadow.data+i*newShadow.size, shadow.data+i*shadow.size, newShadow.size);
            }

            setShadow(instruction, newShadow);
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
            TypedValue shadow = getShadow(instruction->getOperand(0));
            TypedValue newShadow = result.clone();

            for (unsigned i = 0; i < newShadow.num; i++)
            {
                newShadow.setUInt(shadow.getUInt(i), i);
            }

            setShadow(instruction, newShadow);
            break;
        }
        default:
            FATAL_ERROR("Unsupported instruction: %s", instruction->getOpcodeName());
    }

    dumpFunctionArgumentMap();
    dumpShadowMap();
    dumpShadowMem(AddrSpacePrivate);
    dumpShadowMem(AddrSpaceGlobal);
}

Memory *MemCheckUninitialized::getMemory(unsigned addrSpace)
{
    if(!ShadowMem.count(addrSpace))
    {
        FATAL_ERROR("Unsupported address space: %d", addrSpace);
    }
    else
    {
        return ShadowMem[addrSpace];
    }
}

void MemCheckUninitialized::SimpleOr(const llvm::Instruction *I)
{
    for(llvm::Instruction::const_op_iterator OI = I->op_begin(); OI != I->op_end(); ++OI)
    {
        if(getShadow(OI->get()) != getCleanShadow(OI->get()))
        {
            setShadow(I, getPoisonedShadow(I));
            return;
        }
    }

    setShadow(I, getCleanShadow(I));
}

TypedValue MemCheckUninitialized::getCleanShadow(const llvm::Value *V) {
    pair<unsigned,unsigned> size = getValueSize(V);
    TypedValue v = {
        size.first,
        size.second,
        m_pool.alloc(size.first*size.second)
    };

    memset(v.data, 0, v.size*v.num);

    return v;
}

TypedValue MemCheckUninitialized::getCleanShadow(llvm::Type *Ty) {
    unsigned size = getTypeSize(Ty);
    TypedValue v = {
        size,
        1,
        m_pool.alloc(size)
    };

    memset(v.data, 0, v.size);

    return v;
}

TypedValue MemCheckUninitialized::getPoisonedShadow(const llvm::Value *V) {
    pair<unsigned,unsigned> size = getValueSize(V);
    TypedValue v = {
        size.first,
        size.second,
        m_pool.alloc(size.first*size.second)
    };

    memset(v.data, -1, v.size*v.num);

    return v;
}

TypedValue MemCheckUninitialized::getPoisonedShadow(llvm::Type *Ty) {
    unsigned size = getTypeSize(Ty);
    TypedValue v = {
        size,
        1,
        m_pool.alloc(size)
    };

    memset(v.data, -1, v.size);

    return v;
}

TypedValue MemCheckUninitialized::getShadow(const llvm::Value *V) {
    //FIXME: Do I have to clone the Shadows?
    if (const llvm::Instruction *I = llvm::dyn_cast<const llvm::Instruction>(V)) {
        // For instructions the shadow is already stored in the map.
        assert(ShadowMap.count(V) && "No shadow for a value");
        return ShadowMap[V];
    }

    if (const llvm::UndefValue *U = llvm::dyn_cast<const llvm::UndefValue>(V)) {
        return getPoisonedShadow(V);
    }

    if (const llvm::Argument *A = llvm::dyn_cast<const llvm::Argument>(V)) {
        // For arguments we compute the shadow on demand and store it in the map.
        if(ShadowMap.count(V))
        {
            return ShadowMap[V];
        }

        TypedValue *ShadowPtr = &ShadowMap[V];
        const llvm::Function *F = A->getParent();

        unsigned ArgNum = 0;

        for (auto &FArg : F->args())
        {
            if (!FArg.getType()->isSized())
            {
                ++ArgNum;
                continue;
            }

            if (A == &FArg)
            {
                if (FArg.hasByValAttr())
                {
                    // ByVal pointer itself has clean shadow. We copy the actual
                    // argument shadow to the underlying memory.
                    //FIXME: How to get address?
                    //setShadowMem(address, FunctionArgumentMap[F][A]);
                    *ShadowPtr = getCleanShadow(V);
                }
                else
                {
                    *ShadowPtr = FunctionArgumentMap[F][ArgNum];
                }
            }

            ++ArgNum;
        }

        return *ShadowPtr;
    }

    // For everything else the shadow is zero.
    return getCleanShadow(V);
}

//llvm::Type *MemCheckUninitialized::getShadowTy(const llvm::Value *V) {
//  return getShadowTy(V->getType());
//}
//
//llvm::Type *MemCheckUninitialized::getShadowTy(llvm::Type *OrigTy) {
//  if (!OrigTy->isSized()) {
//      cout << "NO SIZE!" << endl;
//    return nullptr;
//  }
//  // For integer type, shadow is the same as the original type.
//  // This may return weird-sized types like i1.
//  if (llvm::IntegerType *IT = llvm::dyn_cast<llvm::IntegerType>(OrigTy))
//  {
//      cout << "INTEGER!" << endl;
//    return IT;
//  }
//  if (llvm::VectorType *VT = llvm::dyn_cast<llvm::VectorType>(OrigTy)) {
//      cout << "VECTOR!" << endl;
//    uint32_t EltSize = VT->getElementType()->getScalarSizeInBits();
//    return llvm::VectorType::get(llvm::IntegerType::get(*llvmContext, EltSize),
//                           VT->getNumElements());
//  }
//  if (llvm::ArrayType *AT = llvm::dyn_cast<llvm::ArrayType>(OrigTy)) {
//      cout << "ARRAY!" << endl;
//    return llvm::ArrayType::get(getShadowTy(AT->getElementType()),
//                          AT->getNumElements());
//  }
//  if (llvm::StructType *ST = llvm::dyn_cast<llvm::StructType>(OrigTy)) {
//      cout << "STRUCT!" << endl;
//      llvm::SmallVector<llvm::Type*, 4> Elements;
//    for (unsigned i = 0, n = ST->getNumElements(); i < n; i++)
//      Elements.push_back(getShadowTy(ST->getElementType(i)));
//    llvm::StructType *Res = llvm::StructType::get(*llvmContext, Elements, ST->isPacked());
//    return Res;
//  }
//  if(llvm::PointerType *PT = llvm::dyn_cast<llvm::PointerType>(OrigTy)) {
//    return llvm::PointerType::get(PT->getPointerElementType(), PT->getAddressSpace());
//  }
//  cout << "OTHER!" << endl;
//  uint32_t TypeSize = OrigTy->getScalarSizeInBits();
//  OrigTy->dump();
//  cout << "TypeSize: " << TypeSize << endl;
//  cout << "TypeID: " << OrigTy->getTypeID() << endl;
//  return llvm::IntegerType::get(*llvmContext, TypeSize);
//}

void MemCheckUninitialized::setShadow(const llvm::Value *V, TypedValue SV) {
  assert(!ShadowMap.count(V) && "Values may only have one shadow");
  ShadowMap[V] = SV;
  ShadowList.push_back(V);
}

void MemCheckUninitialized::setShadowMem(unsigned addrSpace, size_t address, TypedValue SM)
{
    getMemory(addrSpace)->store(SM.data, address, SM.size*SM.num);
}

void MemCheckUninitialized::getShadowMem(unsigned addrSpace, size_t address, TypedValue &SM)
{
    getMemory(addrSpace)->load(SM.data, address, SM.size*SM.num);
}

void MemCheckUninitialized::checkAllOperandsDefined(const llvm::Instruction *I)
{
    for(llvm::Instruction::const_op_iterator OI = I->op_begin(); OI != I->op_end(); ++OI)
    {
        if(getShadow(OI->get()) != getCleanShadow(OI->get()))
        {
            logError(1, 2);
            return;
        }
    }
}

void MemCheckUninitialized::handleIntrinsicInstruction(const llvm::IntrinsicInst *I)
{
    cout << "Intrinsic" << endl;
}

void MemCheckUninitialized::logError(unsigned int addrSpace, size_t address) const
{
  Context::Message msg(WARNING, m_context);
  msg << "Uninitialized value read from "
      << getAddressSpaceName(addrSpace)
      << " memory address 0x" << hex << address << endl
      << msg.INDENT
      << "Kernel: " << msg.CURRENT_KERNEL << endl
      << "Entity: " << msg.CURRENT_ENTITY << endl
      << msg.CURRENT_LOCATION << endl;
  msg.send();
}
