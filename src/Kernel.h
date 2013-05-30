#include "common.h"

class Kernel
{
public:
  Kernel();

  TypedValueMap::const_iterator args_begin() const;
  TypedValueMap::const_iterator args_end() const;
  size_t allocateLocalMemory(size_t size);
  size_t getLocalMemorySize() const;
  void setArgument(const llvm::Value *arg, TypedValue value);

private:
  TypedValueMap m_arguments;
  size_t m_localMemory;
};
