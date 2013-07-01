#include "common.h"

namespace llvm
{
  class Argument;
  class Constant;
  class Function;
  class GlobalVariable;
  class Module;
}

namespace spirsim
{
  class Memory;

  class Kernel
  {
  public:
    Kernel(const llvm::Function *function, const llvm::Module *module);
    virtual ~Kernel();

    TypedValueMap::const_iterator args_begin() const;
    TypedValueMap::const_iterator args_end() const;
    void allocateConstants(Memory *memory);
    void deallocateConstants(Memory *memory);
    size_t getArgumentSize(unsigned int index) const;
    unsigned int getArgumentType(unsigned int index) const;
    const llvm::Function* getFunction() const;
    size_t getLocalMemorySize() const;
    const std::string& getName() const;
    unsigned int getNumArguments() const;
    const size_t* getRequiredWorkGroupSize() const;
    void setArgument(unsigned int index, TypedValue value);

  private:
    const llvm::Function *m_function;
    TypedValueMap m_arguments;
    std::list<const llvm::GlobalVariable*> m_constants;
    std::list<size_t> m_constantBuffers;
    size_t m_localMemory;
    std::string m_name;
    size_t m_requiredWorkGroupSize[3];

    const llvm::Argument* getArgument(unsigned int index) const;
  };
}
