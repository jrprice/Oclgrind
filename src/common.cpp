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
    cout << "%" << setw(10) <<  left
         << instruction.getName().str()
         << "(" << resultSize << ") = ";
  }
  else if (align)
  {
    cout << setw(17) << " ";
  }

  cout << left << setw(align?14:0) << instruction.getOpcodeName();
  llvm::User::const_op_iterator opitr;
  for (opitr = instruction.op_begin(); opitr != instruction.op_end(); opitr++)
  {
    cout << " %" << opitr->get()->getName().str();
  }

  cout << right << endl;
}

size_t getInstructionResultSize(const llvm::Instruction& instruction)
{
  size_t resultSize = instruction.getType()->getPrimitiveSizeInBits() >> 3;

  // TODO: Is this necessary for GEP?
  if (instruction.getOpcode() == llvm::Instruction::GetElementPtr)
  {
    // TODO: Configurable pointer size
    resultSize = 4;
  }

  // TODO: Is this necessary for ?Cmp instructions?
  if (instruction.getOpcode() == llvm::Instruction::ICmp ||
      instruction.getOpcode() == llvm::Instruction::FCmp)
  {
    resultSize = 1;
  }

  return resultSize;
}
