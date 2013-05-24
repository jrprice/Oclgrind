#include "config.h"

namespace llvm
{
  class Value;
}

typedef std::map<const llvm::Value*,size_t> KernelArgs;

class Kernel
{
public:
  Kernel();

  KernelArgs::const_iterator args_begin() const {return m_arguments.begin();};
  KernelArgs::const_iterator args_end() const {return m_arguments.end();};
  void setArgument(const llvm::Value *arg, size_t value);

private:
  KernelArgs m_arguments;
};
