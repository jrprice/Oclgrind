#include "common.h"

namespace llvm
{
  class Function;
  class LLVMContext;
  class Module;
}

class Memory;
class Kernel;

class Simulator
{
public:
  static const unsigned char OUTPUT_GLOBAL_MEM = 0x01;
  static const unsigned char OUTPUT_PRIVATE_MEM = 0x02;
  static const unsigned char OUTPUT_INSTRUCTIONS = 0x04;

public:
  Simulator();
  virtual ~Simulator();

  bool init(std::istream& input);
  void run();
  void setOutputMask(unsigned char mask);

private:
  llvm::LLVMContext *m_context;
  const llvm::Module *m_module;
  const llvm::Function *m_function;

  Memory *m_globalMemory;
  Kernel *m_kernel;
  size_t m_ndrange[3];
  size_t m_wgsize[3];

  unsigned char m_outputMask;
};
