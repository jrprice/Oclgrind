#include "common.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"

using namespace std;

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
  size_t resultSize = instruction.getType()->getPrimitiveSizeInBits() >> 3;

  // TODO: Is this necessary for GEP?
  if (instruction.getOpcode() == llvm::Instruction::GetElementPtr)
  {
    resultSize = sizeof(size_t);
  }

  // TODO: Is this necessary for boolean instructions?
  // TODO: If so, there may be an isPredicate() or similar
  if (instruction.getOpcode() == llvm::Instruction::ICmp ||
      instruction.getOpcode() == llvm::Instruction::FCmp ||
      instruction.getOpcode() == llvm::Instruction::And)
  {
    resultSize = sizeof(bool);
  }

  return resultSize;
}

bool isConstantOperand(const llvm::Value *operand)
{
  unsigned id = operand->getValueID();
  return (id >= llvm::Value::ConstantFirstVal &&
          id <= llvm::Value::ConstantLastVal);
}
