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
