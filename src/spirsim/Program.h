#include "common.h"

namespace llvm
{
  class Module;
}

namespace spirsim
{
  class Kernel;

  class Program
  {
  public:
    virtual ~Program();

    static Program* createFromBitcode(const unsigned char *bitcode,
                                      size_t length);
    static Program* createFromBitcodeFile(const std::string filename);

    Kernel* createKernel(const std::string name);

  private:
    Program(llvm::Module *module);

    llvm::Module *m_module;
  };
}
