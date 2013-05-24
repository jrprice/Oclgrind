#include "common.h"

namespace llvm
{
  class Function;
  class LLVMContext;
  class Module;
}

class GlobalMemory;
class Kernel;

class Simulator
{
public:
  Simulator();
  virtual ~Simulator();

  bool init(std::istream& input);
  void run();

private:
  llvm::LLVMContext *m_context;
  const llvm::Module *m_module;
  const llvm::Function *m_function;

  GlobalMemory *m_globalMemory;
  Kernel *m_kernel;
  size_t m_ndrange[3];
  size_t m_wgsize[3];
};
