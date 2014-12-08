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
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <stdint.h>
#include <vector>

#define BIG_SEPARATOR   "================================"
#define SMALL_SEPARATOR "--------------------------------"

#if defined(_WIN32) && !defined(__MINGW32__)
#define snprintf _snprintf
#undef ERROR
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
  enum AddressSpace
  {
    AddrSpacePrivate = 0,
    AddrSpaceGlobal = 1,
    AddrSpaceConstant = 2,
    AddrSpaceLocal = 3,
  };

  enum AtomicOp
  {
    AtomicAdd,
    AtomicAnd,
    AtomicCmpXchg,
    AtomicDec,
    AtomicInc,
    AtomicMax,
    AtomicMin,
    AtomicOr,
    AtomicSub,
    AtomicXchg,
    AtomicXor,
  };

  // Enumeration for different log message types
  enum MessageType
  {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
  };

  // 3-dimensional size
  typedef struct _Size3_
  {
    size_t x, y, z;
    _Size3_();
    _Size3_(size_t x, size_t y, size_t z);
    _Size3_(size_t linear, _Size3_ dimensions);
    size_t& operator[](size_t i);
    const size_t& operator[](size_t i) const;
    bool operator==(const _Size3_& rhs) const;
    friend std::ostream& operator<<(std::ostream& stream, const _Size3_& sz);
  } Size3;

  // Structure for a value with a size/type
  struct _TypedValue_
  {
    size_t size;
    size_t num;
    unsigned char *data;

    struct _TypedValue_ clone() const;

    double   getFloat(unsigned index = 0) const;
    size_t   getPointer(unsigned index = 0) const;
    int64_t  getSInt(unsigned index = 0) const;
    uint64_t getUInt(unsigned index = 0) const;
    void     setFloat(double value, unsigned index = 0);
    void     setPointer(size_t value, unsigned index = 0);
    void     setSInt(int64_t value, unsigned index = 0);
    void     setUInt(uint64_t value, unsigned index = 0);

  };
  typedef _TypedValue_ TypedValue;

  // Private memory map type
  typedef std::map<const llvm::Value*,TypedValue> TypedValueMap;

  // Image object
  typedef struct
  {
    size_t address;
    cl_image_format format;
    cl_image_desc desc;
  } Image;

  // Check if an environment variable is set to 1
  bool checkEnv(const char *var);

  // Output an instruction in human-readable format
  void dumpInstruction(std::ostream& out, const llvm::Instruction *instruction);

  // Retrieve the raw data for a constant
  void getConstantData(unsigned char *data, const llvm::Constant *constant);

  // Creates an instruction from a constant expression
  const llvm::Instruction* getConstExprAsInstruction(
    const llvm::ConstantExpr *expr);

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
