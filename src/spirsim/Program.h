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
    Program(const char *source);
    virtual ~Program();

    static Program* createFromBitcode(const unsigned char *bitcode,
                                      size_t length);
    static Program* createFromBitcodeFile(const std::string filename);

    bool build(const char *options);
    Kernel* createKernel(const std::string name);

  private:
    Program(llvm::Module *module);

    llvm::Module *m_module;
    char *m_source;
  };
}
