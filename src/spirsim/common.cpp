#include "common.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/Instruction.h"
#include "llvm/Type.h"

using namespace spirsim;
using namespace std;

namespace spirsim
{
  TypedValue clone(const TypedValue& source)
  {
    TypedValue dest;
    dest.size = source.size;
    dest.data = new unsigned char[dest.size];
    memcpy(dest.data, source.data, dest.size);
    return dest;
  }

  void dumpInstruction(const llvm::Instruction& instruction, bool align)
  {
    cout << setfill(' ');

    size_t resultSize = getInstructionResultSize(instruction);
    if (resultSize > 0)
    {
      cout << "%" << setw(12) <<  left
           << instruction.getName().str()
           << "(" << resultSize << ") = ";
    }
    else if (align)
    {
      cout << setw(19) << " ";
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

  size_t getInstructionResultSize(const llvm::Instruction& instruction)
  {
    size_t bits = instruction.getType()->getPrimitiveSizeInBits();
    size_t resultSize = bits >> 3;

    // Special case for GEP
    if (instruction.getOpcode() == llvm::Instruction::GetElementPtr)
    {
      resultSize = sizeof(size_t);
    }

    // Special case for boolean results
    if (bits == 1)
    {
      resultSize = sizeof(bool);
    }

    return resultSize;
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
}
