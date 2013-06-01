#include "common.h"

namespace llvm
{
  class Function;
}

class Kernel
{
public:
  Kernel(const llvm::Function *function);

  TypedValueMap::const_iterator args_begin() const;
  TypedValueMap::const_iterator args_end() const;
  size_t allocateLocalMemory(size_t size);
  const llvm::Function* getFunction() const;
  size_t getLocalMemorySize() const;
  void setArgument(const llvm::Value *arg, TypedValue value);

private:
  const llvm::Function *m_function;
  TypedValueMap m_arguments;
  size_t m_localMemory;
};
