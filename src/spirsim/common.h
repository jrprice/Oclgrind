#ifndef __common_h_
#define __common_h_

#include "config.h"
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <vector>

#define BIG_SEPARATOR   "================================"
#define SMALL_SEPARATOR "--------------------------------"

namespace llvm
{
  class Instruction;
  class Value;
}

namespace spirsim
{
  // Enumeration for address spaces
  enum AddressSpace {
    AddrSpacePrivate = 0,
    AddrSpaceGlobal = 1,
    AddrSpaceConstant = 2,
    AddrSpaceLocal = 3,
  };

  // Structure for a value with a size/type
  typedef struct
  {
    size_t size;
    unsigned char *data;
  } TypedValue;

  // Private memory map type
  typedef std::map<const llvm::Value*,TypedValue> TypedValueMap;

  // Clone a TypedValue
  extern TypedValue clone(const TypedValue& source);

  // Output an instruction in human-readable format
  extern void dumpInstruction(const llvm::Instruction& instruction,
                              bool align=false);

  // Returns the size of an instruction's result
  extern size_t getInstructionResultSize(const llvm::Instruction& instruction);

  // Returns true if the operand is a constant value
  extern bool isConstantOperand(const llvm::Value *operand);
}

#endif // __common_h_
