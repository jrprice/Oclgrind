#include "common.h"

namespace llvm
{
  class Function;
}

namespace spirsim
{
  class Kernel
  {
  public:
    Kernel(const llvm::Function *function);
    virtual ~Kernel();

    TypedValueMap::const_iterator args_begin() const;
    TypedValueMap::const_iterator args_end() const;
    size_t allocateLocalMemory(size_t size);
    const llvm::Function* getFunction() const;
    size_t getLocalMemorySize() const;
    const size_t* getGlobalSize() const;
    unsigned int getNumArguments() const;
    void setArgument(unsigned int index, TypedValue value);
    void setGlobalSize(const size_t globalSize[3]);

  private:
    const llvm::Function *m_function;
    TypedValueMap m_arguments;
    size_t m_localMemory;
    size_t m_globalSize[3];
  };
}
