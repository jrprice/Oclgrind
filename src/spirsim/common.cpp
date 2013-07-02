#include "common.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/Constants.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Operator.h"
#include "llvm/Type.h"

using namespace spirsim;
using namespace std;

namespace spirsim
{
  TypedValue clone(const TypedValue& source)
  {
    TypedValue dest;
    dest.size = source.size;
    dest.num = source.num;
    dest.data = new unsigned char[dest.size*dest.num];
    memcpy(dest.data, source.data, dest.size*dest.num);
    return dest;
  }

  void dumpInstruction(const llvm::Instruction& instruction, bool align)
  {
    cout << setfill(' ');

    pair<size_t,size_t> resultSize = getInstructionResultSize(instruction);
    if (resultSize.first > 0)
    {
      cout << "%" << setw(12) <<  left
           << instruction.getName().str()
           << "(" << resultSize.second
           << "x" << resultSize.first
           << ") = ";
    }
    else if (align)
    {
      cout << setw(21) << " ";
    }

    cout << left << setw(align?14:0) << instruction.getOpcodeName();

    llvm::User::const_op_iterator opItr;
    for (opItr = instruction.op_begin(); opItr != instruction.op_end(); opItr++)
    {
      // TODO: Constant values
      cout << " %" << opItr->get()->getName().str();
    }

    cout << right << endl;
  }

  pair<size_t,size_t> getInstructionResultSize(
    const llvm::Instruction& instruction)
  {
    size_t bits, numElements;
    const llvm::Type *type = instruction.getType();

    if (type->isVectorTy())
    {
      bits = type->getVectorElementType()->getPrimitiveSizeInBits();
      numElements = type->getVectorNumElements();
    }
    else
    {
      bits = type->getPrimitiveSizeInBits();
      numElements = 1;
    }

    size_t elemSize = bits >> 3;

    // Special case for pointer types
    if (type->isPointerTy())
    {
      elemSize = sizeof(size_t);
    }

    // Special case for boolean results
    if (bits == 1)
    {
      elemSize = sizeof(bool);
    }

    return pair<size_t,size_t>(elemSize,numElements);
  }

  llvm::Instruction* getConstExprAsInstruction(const llvm::ConstantExpr *expr)
  {
    // Get operands
    int numOperands = expr->getNumOperands();
    llvm::Value **valueOperands = new llvm::Value*[numOperands];
    for (int i = 0; i < numOperands; i++)
    {
      valueOperands[i] = expr->getOperand(i);
    }
    llvm::ArrayRef<llvm::Value*> operands(valueOperands, numOperands);

    // Create instruction
    unsigned opcode = expr->getOpcode();
    switch (opcode)
    {
    case llvm::Instruction::Trunc:
    case llvm::Instruction::ZExt:
    case llvm::Instruction::SExt:
    case llvm::Instruction::FPTrunc:
    case llvm::Instruction::FPExt:
    case llvm::Instruction::UIToFP:
    case llvm::Instruction::SIToFP:
    case llvm::Instruction::FPToUI:
    case llvm::Instruction::FPToSI:
    case llvm::Instruction::PtrToInt:
    case llvm::Instruction::IntToPtr:
    case llvm::Instruction::BitCast:
      return llvm::CastInst::Create((llvm::Instruction::CastOps)opcode,
                                    operands[0], expr->getType());
    case llvm::Instruction::Select:
      return llvm::SelectInst::Create(operands[0], operands[1], operands[2]);
    case llvm::Instruction::InsertElement:
      return llvm::InsertElementInst::Create(operands[0], operands[1],
                                             operands[2]);
    case llvm::Instruction::ExtractElement:
      return llvm::ExtractElementInst::Create(operands[0], operands[1]);
    case llvm::Instruction::InsertValue:
      return llvm::InsertValueInst::Create(operands[0], operands[1],
                                           expr->getIndices());
    case llvm::Instruction::ExtractValue:
      return llvm::ExtractValueInst::Create(operands[0], expr->getIndices());
    case llvm::Instruction::ShuffleVector:
      return new llvm::ShuffleVectorInst(operands[0], operands[1],
                                         operands[2]);
    case llvm::Instruction::GetElementPtr:
      if (((const llvm::GEPOperator*)expr)->isInBounds())
      {
        return llvm::GetElementPtrInst::CreateInBounds(operands[0],
                                                       operands.slice(1));
      }
      else
      {
        cout << "Hello, world!" << endl;
        return llvm::GetElementPtrInst::Create(operands[0], operands.slice(1));
      }
    case llvm::Instruction::ICmp:
    case llvm::Instruction::FCmp:
      return llvm::CmpInst::Create((llvm::Instruction::OtherOps)opcode,
                                   expr->getPredicate(),
                                   operands[0], operands[1]);
    default:
      assert(expr->getNumOperands() == 2 && "Must be binary operator?");
      assert(false && "Unhandled binary operator in constant expression.");
      //llvm::BinaryOperator *BO =
      //  llvm::BinaryOperator::Create((llvm::Instruction::Binaryoperands)opcode,
      //                               operands[0], operands[1]);
      //if (isa<OverflowingBinaryOperator>(BO)) {
      //  BO->setHasNoUnsignedWrap(SubclassOptionalData &
      //                           OverflowingBinaryOperator::NoUnsignedWrap);
      //  BO->setHasNoSignedWrap(SubclassOptionalData &
      //                         OverflowingBinaryOperator::NoSignedWrap);
      //}
      //if (isa<PossiblyExactOperator>(BO))
      //  BO->setIsExact(SubclassOptionalData & PossiblyExactOperator::IsExact);
      //return BO;
    }
  }

  size_t getTypeSize(const llvm::Type *type)
  {
    if (type->isArrayTy())
    {
      size_t num = type->getArrayNumElements();
      size_t sz = getTypeSize(type->getArrayElementType());
      return num*sz;
    }
    else if (type->isStructTy())
    {
      size_t size = 0;
      for (int i = 0; i < type->getStructNumElements(); i++)
      {
        size += getTypeSize(type->getStructElementType(i));
      }
      return size;
    }
    else if (type->isVectorTy())
    {
      size_t num = type->getVectorNumElements();
      size_t sz = getTypeSize(type->getVectorElementType());
      return num*sz;
    }
    else if (type->isPointerTy())
    {
      return sizeof(size_t);
    }
    else
    {
      return ((llvm::Type*)type)->getScalarSizeInBits()>>3;
    }
  }

  bool isConstantOperand(const llvm::Value *operand)
  {
    unsigned id = operand->getValueID();
    return (id >= llvm::Value::ConstantFirstVal &&
            id <= llvm::Value::ConstantLastVal);
  }

  void getConstantData(unsigned char *data, const llvm::Constant *constant)
  {
    if (constant->getValueID() == llvm::Value::UndefValueVal)
    {
      memset(data, -1, getTypeSize(constant->getType()));
      return;
    }

    const llvm::Type *type = constant->getType();
    size_t size = getTypeSize(type);
    switch (type->getTypeID())
    {
    case llvm::Type::IntegerTyID:
      memcpy(data,
             ((llvm::ConstantInt*)constant)->getValue().getRawData(),
             size);
      break;
    case llvm::Type::FloatTyID:
    {
      *(float*)data =
        ((llvm::ConstantFP*)constant)->getValueAPF().convertToFloat();
      break;
    }
    case llvm::Type::VectorTyID:
    {
      int num = type->getVectorNumElements();
      const llvm::Type *elemType = type->getVectorElementType();
      size_t elemSize = getTypeSize(elemType);
      for (int i = 0; i < num; i++)
      {
        getConstantData(data + i*elemSize, constant->getAggregateElement(i));
      }
      break;
    }
    case llvm::Type::ArrayTyID:
    {
      int num = type->getArrayNumElements();
      const llvm::Type *elemType = type->getArrayElementType();
      size_t elemSize = getTypeSize(elemType);
      for (int i = 0; i < num; i++)
      {
        getConstantData(data + i*elemSize, constant->getAggregateElement(i));
      }
      break;
    }
    default:
      cerr << "Unhandled constant type " << type->getTypeID() << endl;
      break;
    }
  }
}
