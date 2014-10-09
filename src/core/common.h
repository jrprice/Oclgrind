// common.h (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#ifndef __common_h_
#define __common_h_

#include "config.h"
#include "CL/cl.h"
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
#include <stdexcept>
#include <stdint.h>
#include <vector>

#define BIG_SEPARATOR   "================================"
#define SMALL_SEPARATOR "--------------------------------"

#if defined(_WIN32) && !defined(__MINGW32__)
#define snprintf _snprintf
#endif

namespace llvm
{
  class Constant;
  class ConstantExpr;
  class Instruction;
  class StructType;
  class Type;
  class Value;
}

namespace oclgrind
{
  class Kernel;

  // Enumeration for address spaces
  enum AddressSpace {
    AddrSpacePrivate = 0,
    AddrSpaceGlobal = 1,
    AddrSpaceConstant = 2,
    AddrSpaceLocal = 3,
  };

  // Enumeration for types of data-race
  enum DataRaceType
  {
    ReadWriteRace,
    WriteWriteRace
  };

  // Kernel invocation
  typedef struct
  {
    const Kernel *kernel;
    size_t workDim;
    size_t globalSize[3];
    size_t globalOffset[3];
    size_t localSize[3];
    size_t numGroups[3];
  } KernelInvocation;

  // Structure for a value with a size/type
  typedef struct
  {
    size_t size;
    size_t num;
    unsigned char *data;

    double   getFloat(unsigned index = 0) const;
    size_t   getPointer(unsigned index = 0) const;
    int64_t  getSInt(unsigned index = 0) const;
    uint64_t getUInt(unsigned index = 0) const;
    void     setFloat(double value, unsigned index = 0);
    void     setPointer(size_t value, unsigned index = 0);
    void     setSInt(int64_t value, unsigned index = 0);
    void     setUInt(uint64_t value, unsigned index = 0);

  } TypedValue;

  // Private memory map type
  typedef std::map<const llvm::Value*,TypedValue> TypedValueMap;

  // Clone a TypedValue
  TypedValue clone(const TypedValue& source);

  // Output an instruction in human-readable format
  void dumpInstruction(std::ostream& out, const llvm::Instruction& instruction);

  // Retrieve the raw data for a constant
  void getConstantData(unsigned char *data, const llvm::Constant *constant);

  // Creates an instruction from a constant expression
  llvm::Instruction* getConstExprAsInstruction(const llvm::ConstantExpr *expr);

  // Get the byte offset of a struct member
  size_t getStructMemberOffset(const llvm::StructType *type, size_t index);

  // Returns the size of a type
  size_t getTypeSize(const llvm::Type *type);

  /// Returns the alignment requirements of this type
  size_t getTypeAlignment(const llvm::Type* type);

  // Returns the size of a value
  std::pair<size_t,size_t> getValueSize(const llvm::Value *value);

  // Returns true if the operand is a constant value
  bool isConstantOperand(const llvm::Value *operand);

  // Returns true if the value is a 3-element vector
  bool isVector3(const llvm::Value *value);

  // Return the current time in nanoseconds since the epoch
  double now();

  // Print data in a human readable format (according to its type)
  void printTypedData(const llvm::Type *type, const unsigned char *data);

  // Exception class for raising fatal errors
  class FatalError : std::runtime_error
  {
  public:
    FatalError(const std::string& msg, const std::string& file, size_t line);
    ~FatalError() throw();
    virtual const std::string& getFile() const;
    virtual size_t getLine() const;
    virtual const char* what() const throw();
  protected:
    std::string m_file;
    size_t m_line;
  };

  // Utility macro for raising an exception with a sprintf-based message
  #define FATAL_ERROR(format, ...)                       \
    {                                                    \
      int sz = snprintf(NULL, 0, format, ##__VA_ARGS__); \
      char *str = new char[sz+1];                        \
      sprintf(str, format, ##__VA_ARGS__);               \
      string msg = str;                                  \
      delete[] str;                                      \
      throw FatalError(msg, __FILE__, __LINE__);         \
    }
}

#endif // __common_h_
