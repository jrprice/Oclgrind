#ifndef __common_h_
#define __common_h_

#include "config.h"
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <vector>

#define BIG_SEPARATOR   "================================"
#define SMALL_SEPARATOR "--------------------------------"

namespace llvm
{
  class Constant;
  class ConstantExpr;
  class Instruction;
  class Type;
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
    size_t num;
    unsigned char *data;
  } TypedValue;

  // Private memory map type
  typedef std::map<const llvm::Value*,TypedValue> TypedValueMap;

  // Clone a TypedValue
  extern TypedValue clone(const TypedValue& source);

  // Output an instruction in human-readable format
  extern void dumpInstruction(const llvm::Instruction& instruction,
                              bool align=false);

  // Creates an instruction from a constant expression
  extern llvm::Instruction* getConstExprAsInstruction(
    const llvm::ConstantExpr *expr);

  // Returns the size of an instruction's result
  extern std::pair<size_t,size_t> getInstructionResultSize(
    const llvm::Instruction& instruction);

  // Returns the size of a type
  extern size_t getTypeSize(const llvm::Type *type);

  // Returns true if the operand is a constant value
  extern bool isConstantOperand(const llvm::Value *operand);

  // Retrieve the raw data for a constant
  extern void getConstantData(unsigned char *data,
                              const llvm::Constant *constant);
}

#endif // __common_h_
