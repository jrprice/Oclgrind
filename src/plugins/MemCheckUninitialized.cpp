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

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Type.h"

#include "MemCheckUninitialized.h"

using namespace oclgrind;
using namespace std;

void MemCheckUninitialized::dumpShadowMap()
{
    TypedValueMap::iterator itr;

    cout << "=======================" << endl;

    for(itr = ShadowMap.begin(); itr != ShadowMap.end(); ++itr)
    {
        cout << itr->first->getName().str() << ": " << hex << itr->second.getUInt() << endl;
    }

    cout << "=======================" << endl;
}

void MemCheckUninitialized::dumpShadowMem()
{
    cout << "=======================" << endl;

    ShadowMem->dump();

    cout << "=======================" << endl;
}

MemCheckUninitialized::MemCheckUninitialized(const Context *context)
 : Plugin(context), AddressMap(), ShadowMap()
{
    ShadowMem = new Memory(AddrSpacePrivate, sizeof(size_t)==8 ? 32 : 16, context);
    llvmContext = new llvm::LLVMContext();
}

void MemCheckUninitialized::instructionExecuted(const WorkItem *workItem,
                                        const llvm::Instruction *instruction,
                                        const TypedValue& result)
{
    cout << instruction->getOpcodeName() << endl;
    instruction->dump();

    switch(instruction->getOpcode())
    {
        case llvm::Instruction::Add:
        {
            if(llvm::Instruction *I = llvm::dyn_cast<llvm::Instruction>(instruction->getOperand(0)))
            {
                cout << "Instr1!" << endl;
            }
            if(llvm::Instruction *I = llvm::dyn_cast<llvm::Instruction>(instruction->getOperand(1)))
            {
                cout << "Instr2!" << endl;
            }
            if(llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(instruction->getOperand(0)))
            {
                cout << "Arg1!" << endl;
            }
            if(llvm::Argument *A = llvm::dyn_cast<llvm::Argument>(instruction->getOperand(1)))
            {
                cout << "Arg2!" << endl;
            }
            break;
        }
        case llvm::Instruction::Alloca:
        {
            const llvm::AllocaInst *allocInst = ((const llvm::AllocaInst*)instruction);
            size_t address = result.getPointer();

            setShadow(instruction, getCleanShadow(instruction));

            TypedValue v = getPoisonedShadow(instruction->getOperand(0));
            ShadowMem->allocateBuffer(v.size);
            setShadowMem(address, v);
            break;
        }
//        case llvm::Instruction::And:
//          bwand(instruction, result);
//          break;
//        case llvm::Instruction::AShr:
//          ashr(instruction, result);
//          break;
//        case llvm::Instruction::BitCast:
//          bitcast(instruction, result);
//          break;
//        case llvm::Instruction::Br:
//          br(instruction, result);
//          break;
//        case llvm::Instruction::Call:
//          call(instruction, result);
//          break;
//        case llvm::Instruction::ExtractElement:
//          extractelem(instruction, result);
//          break;
//        case llvm::Instruction::ExtractValue:
//          extractval(instruction, result);
//          break;
//        case llvm::Instruction::FAdd:
//          fadd(instruction, result);
//          break;
//        case llvm::Instruction::FCmp:
//          fcmp(instruction, result);
//          break;
//        case llvm::Instruction::FDiv:
//          fdiv(instruction, result);
//          break;
//        case llvm::Instruction::FMul:
//          fmul(instruction, result);
//          break;
//        case llvm::Instruction::FPExt:
//          fpext(instruction, result);
//          break;
//        case llvm::Instruction::FPToSI:
//          fptosi(instruction, result);
//          break;
//        case llvm::Instruction::FPToUI:
//          fptoui(instruction, result);
//          break;
//        case llvm::Instruction::FPTrunc:
//          fptrunc(instruction, result);
//          break;
//        case llvm::Instruction::FRem:
//          frem(instruction, result);
//          break;
//        case llvm::Instruction::FSub:
//          fsub(instruction, result);
//          break;
//        case llvm::Instruction::GetElementPtr:
//          gep(instruction, result);
//          break;
//        case llvm::Instruction::ICmp:
//          icmp(instruction, result);
//          break;
//        case llvm::Instruction::InsertElement:
//          insertelem(instruction, result);
//          break;
//        case llvm::Instruction::InsertValue:
//          insertval(instruction, result);
//          break;
//        case llvm::Instruction::IntToPtr:
//          inttoptr(instruction, result);
//          break;
//        case llvm::Instruction::Load:
//          load(instruction, result);
//          break;
//        case llvm::Instruction::LShr:
//          lshr(instruction, result);
//          break;
//        case llvm::Instruction::Mul:
//          mul(instruction, result);
//          break;
//        case llvm::Instruction::Or:
//          bwor(instruction, result);
//          break;
//        case llvm::Instruction::PHI:
//          phi(instruction, result);
//          break;
//        case llvm::Instruction::PtrToInt:
//          ptrtoint(instruction, result);
//          break;
//        case llvm::Instruction::Ret:
//          ret(instruction, result);
//          break;
//        case llvm::Instruction::SDiv:
//          sdiv(instruction, result);
//          break;
//        case llvm::Instruction::Select:
//          select(instruction, result);
//          break;
//        case llvm::Instruction::SExt:
//          sext(instruction, result);
//          break;
//        case llvm::Instruction::Shl:
//          shl(instruction, result);
//          break;
//        case llvm::Instruction::ShuffleVector:
//          shuffle(instruction, result);
//          break;
//        case llvm::Instruction::SIToFP:
//          sitofp(instruction, result);
//          break;
//        case llvm::Instruction::SRem:
//          srem(instruction, result);
//          break;
        case llvm::Instruction::Store:
        {
            const llvm::StoreInst *storeInst = ((const llvm::StoreInst*)instruction);
            const llvm::Value *Val = storeInst->getValueOperand();
            const llvm::Value *Addr = storeInst->getPointerOperand();

            size_t address = workItem->getOperand(Addr).getPointer();
            TypedValue shadowVal = storeInst->isAtomic() ? getCleanShadow(Val) : getShadow(Val);

            setShadowMem(address, shadowVal);
            break;
        }
//        case llvm::Instruction::Sub:
//          sub(instruction, result);
//          break;
//        case llvm::Instruction::Switch:
//          swtch(instruction, result);
//          break;
//        case llvm::Instruction::Trunc:
//          itrunc(instruction, result);
//          break;
//        case llvm::Instruction::UDiv:
//          udiv(instruction, result);
//          break;
//        case llvm::Instruction::UIToFP:
//          uitofp(instruction, result);
//          break;
//        case llvm::Instruction::URem:
//          urem(instruction, result);
//          break;
//        case llvm::Instruction::Unreachable:
//          FATAL_ERROR("Encountered unreachable instruction");
//        case llvm::Instruction::Xor:
//          bwxor(instruction, result);
//          break;
//        case llvm::Instruction::ZExt:
//          zext(instruction, result);
//          break;
        case llvm::Instruction::Unreachable:
            FATAL_ERROR("Encountered unreachable instruction");
        //default:
            //FATAL_ERROR("Unsupported instruction: %s", instruction->getOpcodeName());
    }

    dumpShadowMap();
    dumpShadowMem();
}

TypedValue MemCheckUninitialized::getCleanShadow(const llvm::Value *V) {
    pair<unsigned,unsigned> size = getValueSize(V);
    TypedValue v = {
        size.first,
        size.second,
        m_pool.alloc(size.first*size.second)
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

    memset(v.data, -1, v.size);

    return v;
}

//llvm::Constant *MemCheckUninitialized::getPoisonedShadow(llvm::Type *ShadowTy) {
//  assert(ShadowTy);
//  if (llvm::isa<llvm::IntegerType>(ShadowTy) || llvm::isa<llvm::VectorType>(ShadowTy))
//    return llvm::Constant::getAllOnesValue(ShadowTy);
//  if (llvm::ArrayType *AT = llvm::dyn_cast<llvm::ArrayType>(ShadowTy)) {
//    llvm::SmallVector<llvm::Constant *, 4> Vals(AT->getNumElements(),
//                                    getPoisonedShadow(AT->getElementType()));
//    return llvm::ConstantArray::get(AT, Vals);
//  }
//  if (llvm::StructType *ST = llvm::dyn_cast<llvm::StructType>(ShadowTy)) {
//    llvm::SmallVector<llvm::Constant *, 4> Vals;
//    for (unsigned i = 0, n = ST->getNumElements(); i < n; i++)
//      Vals.push_back(getPoisonedShadow(ST->getElementType(i)));
//    return llvm::ConstantStruct::get(ST, Vals);
//  }
//  llvm_unreachable("Unexpected shadow type");
//}
//
///// \brief Create a dirty shadow for a given value.
//llvm::Constant *MemCheckUninitialized::getPoisonedShadow(const llvm::Value *V) {
//  llvm::Type *ShadowTy = getShadowTy(V);
//  if (!ShadowTy)
//    return nullptr;
//  return getPoisonedShadow(ShadowTy);
//}
//
TypedValue MemCheckUninitialized::getShadow(const llvm::Value *V) {
  if (const llvm::Instruction *I = llvm::dyn_cast<const llvm::Instruction>(V)) {
    // For instructions the shadow is already stored in the map.
    TypedValue Shadow = ShadowMap[V];
    //FIXME: Add assertion
    //if (!Shadow) {
    //  (void)I;
    //  assert(Shadow && "No shadow for a value");
    //}
    return Shadow;
  }
  if (const llvm::UndefValue *U = llvm::dyn_cast<const llvm::UndefValue>(V)) {
    TypedValue AllOnes = getPoisonedShadow(V);
    (void)U;
    return AllOnes;
  }
  if (const llvm::Argument *A = llvm::dyn_cast<const llvm::Argument>(V)) {
//    // For arguments we compute the shadow on demand and store it in the map.
//    llvm::Value **ShadowPtr = &ShadowMap[V];
//    if (*ShadowPtr)
//      return *ShadowPtr;
//    Function *F = A->getParent();
//    IRBuilder<> EntryIRB(F->getEntryBlock().getFirstNonPHI());
//    unsigned ArgOffset = 0;
//    for (auto &FArg : F->args()) {
//      if (!FArg.getType()->isSized()) {
//        DEBUG(dbgs() << "Arg is not sized\n");
//        continue;
//      }
//      unsigned Size = FArg.hasByValAttr()
//        ? MS.DL->getTypeAllocSize(FArg.getType()->getPointerElementType())
//        : MS.DL->getTypeAllocSize(FArg.getType());
//      if (A == &FArg) {
//        bool Overflow = ArgOffset + Size > kParamTLSSize;
//        Value *Base = getShadowPtrForArgument(&FArg, EntryIRB, ArgOffset);
//        if (FArg.hasByValAttr()) {
//          // ByVal pointer itself has clean shadow. We copy the actual
//          // argument shadow to the underlying memory.
//          // Figure out maximal valid memcpy alignment.
//          unsigned ArgAlign = FArg.getParamAlignment();
//          if (ArgAlign == 0) {
//            Type *EltType = A->getType()->getPointerElementType();
//            ArgAlign = MS.DL->getABITypeAlignment(EltType);
//          }
//          if (Overflow) {
//            // ParamTLS overflow.
//            EntryIRB.CreateMemSet(
//                getShadowPtr(V, EntryIRB.getInt8Ty(), EntryIRB),
//                Constant::getNullValue(EntryIRB.getInt8Ty()), Size, ArgAlign);
//          } else {
//            unsigned CopyAlign = std::min(ArgAlign, kShadowTLSAlignment);
//            Value *Cpy = EntryIRB.CreateMemCpy(
//                getShadowPtr(V, EntryIRB.getInt8Ty(), EntryIRB), Base, Size,
//                CopyAlign);
//            DEBUG(dbgs() << "  ByValCpy: " << *Cpy << "\n");
//            (void)Cpy;
//          }
//          *ShadowPtr = getCleanShadow(V);
//        } else {
//          if (Overflow) {
//            // ParamTLS overflow.
//            *ShadowPtr = getCleanShadow(V);
//          } else {
//            *ShadowPtr =
//                EntryIRB.CreateAlignedLoad(Base, kShadowTLSAlignment);
//          }
//        }
//        DEBUG(dbgs() << "  ARG:    "  << FArg << " ==> " <<
//              **ShadowPtr << "\n");
//        if (MS.TrackOrigins && !Overflow) {
//          Value *OriginPtr =
//              getOriginPtrForArgument(&FArg, EntryIRB, ArgOffset);
//          setOrigin(A, EntryIRB.CreateLoad(OriginPtr));
//        } else {
//          setOrigin(A, getCleanOrigin());
//        }
//      }
//      ArgOffset += RoundUpToAlignment(Size, kShadowTLSAlignment);
//    }
//    assert(*ShadowPtr && "Could not find shadow for an argument");
//    return *ShadowPtr;
    assert(false);
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
}

void MemCheckUninitialized::setShadowMem(size_t address, TypedValue SM)
{
    ShadowMem->store(SM.data, address, SM.size);
}

//bool MemCheckUninitialized::isCleanShadowMem(size_t address, unsigned size)
//{
//    size_t index = address >> 6;
//    unsigned int offset = address & 0x3f;
//    unsigned long mask;
//
//    if(offset + size < 63)
//    {
//        mask = ((1 << size) - 1) << offset;
//
//        return ((ShadowMem[index] | mask) == mask);
//    }
//    else
//    {
//        mask = ((unsigned long)-1) >> offset;
//        
//        if((ShadowMem[index] | mask) != mask)
//        {
//            return false;
//        }
//
//        size -= 64 - offset;
//        ++index;
//
//        while(size >= 64)
//        {
//            if(ShadowMem[index] != 0)
//            {
//                return false;
//            }
//
//            ++index;
//            size -= 64;
//        }
//
//        if(size > 0)
//        {
//            mask = (1 << size) - 1;
//
//            if((ShadowMem[index] | mask) != mask)
//            {
//                return false;
//            }
//        }
//
//        return true;
//    }
//}

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
