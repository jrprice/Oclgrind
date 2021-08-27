// common.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#ifndef __common_h_
#define __common_h_

#define CL_TARGET_OPENCL_VERSION 300
#include "CL/cl.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <stdint.h>
#include <unordered_map>
#include <vector>

#if defined(_WIN32) && !defined(__MINGW32__)
#define snprintf _snprintf
#undef ERROR
#endif

#ifdef __APPLE__
// TODO: Remove this when thread_local fixed on OS X
#define THREAD_LOCAL __thread
#elif defined(_WIN32) && !defined(__MINGW32__)
// TODO: Remove this when thread_local fixed on Windows
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL thread_local
#endif

#define CLK_NORMALIZED_COORDS_TRUE 0x0001

#define CLK_ADDRESS_NONE 0x0000
#define CLK_ADDRESS_CLAMP_TO_EDGE 0x0002
#define CLK_ADDRESS_CLAMP 0x0004
#define CLK_ADDRESS_REPEAT 0x0006
#define CLK_ADDRESS_MIRRORED_REPEAT 0x0008
#define CLK_ADDRESS_MASK 0x000E

#define CLK_FILTER_NEAREST 0x0010
#define CLK_FILTER_LINEAR 0x0020

namespace llvm
{
class Constant;
class ConstantExpr;
class ConstantInt;
class Instruction;
class Metadata;
class StructType;
class Type;
class Value;
} // namespace llvm

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
  AddrSpaceGeneric = 4,
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
struct Size3
{
  size_t x, y, z;
  Size3();
  Size3(size_t x, size_t y, size_t z);
  Size3(size_t linear, Size3 dimensions);
  size_t& operator[](unsigned i);
  const size_t& operator[](unsigned i) const;
  bool operator==(const Size3& rhs) const;
  bool operator!=(const Size3& rhs) const;
  friend std::ostream& operator<<(std::ostream& stream, const Size3& sz);
};

// Structure for a value with a size/type
struct TypedValue
{
  unsigned size;
  unsigned num;
  unsigned char* data;

  bool operator==(const TypedValue& rhs) const;
  bool operator!=(const TypedValue& rhs) const;

  friend std::ostream& operator<<(std::ostream& stream, const TypedValue& tv);

  struct TypedValue clone() const;

  double getFloat(unsigned index = 0) const;
  size_t getPointer(unsigned index = 0) const;
  int64_t getSInt(unsigned index = 0) const;
  uint64_t getUInt(unsigned index = 0) const;
  void setFloat(double value, unsigned index = 0);
  void setPointer(size_t value, unsigned index = 0);
  void setSInt(int64_t value, unsigned index = 0);
  void setUInt(uint64_t value, unsigned index = 0);
};

// Private memory map type
typedef std::map<const llvm::Value*, TypedValue> TypedValueMap;

// Image object
struct Image
{
  size_t address;
  cl_image_format format;
  cl_image_desc desc;
};

// Check if an environment variable is set to 1
bool checkEnv(const char* var);

// Get an environment variable as an integer
unsigned getEnvInt(const char* var, int def = 0, bool allowZero = true);

// Output an instruction in human-readable format
void dumpInstruction(std::ostream& out, const llvm::Instruction* instruction);

// Get the human readable name of an address space
const char* getAddressSpaceName(unsigned addrSpace);

// Retrieve the raw data for a constant
void getConstantData(unsigned char* data, const llvm::Constant* constant);

// Creates an instruction from a constant expression
llvm::Instruction* getConstExprAsInstruction(const llvm::ConstantExpr* expr);

// Get the ConstantInt object for a Metadata node
const llvm::ConstantInt* getMDAsConstInt(const llvm::Metadata* md);

// Get the byte offset of a struct member
unsigned getStructMemberOffset(const llvm::StructType* type, unsigned index);

// Returns the size of a type
unsigned getTypeSize(const llvm::Type* type);

/// Returns the alignment requirements of this type
unsigned getTypeAlignment(const llvm::Type* type);

// Returns the size of a value
std::pair<unsigned, unsigned> getValueSize(const llvm::Value* value);

// Returns true if the operand is a constant value
bool isConstantOperand(const llvm::Value* operand);

// Returns true if the value is a 3-element vector
bool isVector3(const llvm::Value* value);

// Return the current time in nanoseconds since the epoch
double now();

// Print data in a human readable format (according to its type)
void printTypedData(const llvm::Type* type, const unsigned char* data);

// Resolve a constant pointer, using a set of known constant values
size_t resolveConstantPointer(const llvm::Value* ptr, TypedValueMap& values);

// Resolve a GEP from a base address and list of offsets
size_t resolveGEP(size_t base, const llvm::Type* ptrType,
                  std::vector<int64_t>& offsets);

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
#define FATAL_ERROR(format, ...)                                               \
  {                                                                            \
    int sz = snprintf(NULL, 0, format, ##__VA_ARGS__);                         \
    char* str = new char[sz + 1];                                              \
    sprintf(str, format, ##__VA_ARGS__);                                       \
    string msg = str;                                                          \
    delete[] str;                                                              \
    throw FatalError(msg, __FILE__, __LINE__);                                 \
  }

class MemoryPool
{
public:
  MemoryPool(size_t blockSize = 1024);
  ~MemoryPool();
  uint8_t* alloc(size_t size);
  TypedValue clone(const TypedValue& source);

private:
  size_t m_blockSize;
  size_t m_offset;
  std::list<uint8_t*> m_blocks;
};

// Pool allocator class for STL containers
template <class T, size_t BLOCKSIZE> class PoolAllocator
{
  template <typename U, size_t BS> friend class PoolAllocator;

public:
  typedef T value_type;
  typedef T* pointer;
  typedef T& reference;
  typedef const T* const_pointer;
  typedef const T& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  template <typename U> struct rebind
  {
    typedef PoolAllocator<U, BLOCKSIZE> other;
  };

  PoolAllocator()
  {
    pool.reset(new MemoryPool(BLOCKSIZE));
  }

  PoolAllocator(const PoolAllocator& p)
  {
    this->pool = p.pool;
  }

  template <typename U> PoolAllocator(const PoolAllocator<U, BLOCKSIZE>& p)
  {
    this->pool = p.pool;
  }

  pointer allocate(size_type n, const_pointer hint = 0)
  {
    return (pointer)(pool->alloc(n * sizeof(value_type)));
  }

  void deallocate(pointer p, size_type n) {}

  template <class U, class... Args> void construct(U* p, Args&&... args)
  {
    new ((void*)p) U(std::forward<Args>(args)...);
  }

  template <class U> void destroy(U* p)
  {
    p->~U();
  }

  bool operator==(const PoolAllocator& p) const
  {
    return this->pool == p.pool;
  }

  bool operator!=(const PoolAllocator& p) const
  {
    return this->pool != p.pool;
  }

private:
  std::shared_ptr<MemoryPool> pool;
};

} // namespace oclgrind

#endif // __common_h_
