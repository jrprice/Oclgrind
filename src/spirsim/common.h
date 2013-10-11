// common.h (oclgrind)
// Copyright (C) 2013 James Price
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

#ifndef __common_h_
#define __common_h_

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "config.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <stdint.h>
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
  extern void dumpInstruction(std::ostream& out,
                              const llvm::Instruction& instruction,
                              bool align=false);

  // Creates an instruction from a constant expression
  extern llvm::Instruction* getConstExprAsInstruction(
    const llvm::ConstantExpr *expr);

  // Returns the size of a value
  extern std::pair<size_t,size_t> getValueSize(const llvm::Value *value);

  // Returns the size of a type
  extern size_t getTypeSize(const llvm::Type *type);

  // Returns true if the operand is a constant value
  extern bool isConstantOperand(const llvm::Value *operand);

  // Returns true if the value is a 3-element vector
  extern bool isVector3(const llvm::Value *value);

  // Retrieve the raw data for a constant
  extern void getConstantData(unsigned char *data,
                              const llvm::Constant *constant);
}

#endif // __common_h_
