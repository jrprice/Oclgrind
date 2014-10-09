// Kernel.h (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

#include "llvm/ADT/StringRef.h"

namespace llvm
{
  class Argument;
  class Constant;
  class Function;
  class GlobalVariable;
  class MDNode;
  class Module;
}

namespace oclgrind
{
  class Memory;
  class Program;

  class Kernel
  {
  public:
    Kernel(const Program *program,
           const llvm::Function *function, const llvm::Module *module);
    Kernel(const Kernel& kernel);
    virtual ~Kernel();

    TypedValueMap::const_iterator args_begin() const;
    TypedValueMap::const_iterator args_end() const;
    std::list<const llvm::GlobalVariable*>::const_iterator vars_begin() const;
    std::list<const llvm::GlobalVariable*>::const_iterator vars_end() const;
    bool allArgumentsSet() const;
    void allocateConstants(Memory *memory);
    void deallocateConstants(Memory *memory);
    size_t getArgumentSize(unsigned int index) const;
    unsigned int getArgumentAccessQualifier(unsigned int index) const;
    unsigned int getArgumentAddressQualifier(unsigned int index) const;
    const llvm::StringRef getArgumentName(unsigned int index) const;
    const llvm::StringRef getArgumentTypeName(unsigned int index) const;
    unsigned int getArgumentTypeQualifier(unsigned int index) const;
    std::string getAttributes() const;
    const llvm::Function* getFunction() const;
    const Memory* getLocalMemory() const;
    size_t getLocalMemorySize() const;
    const std::string& getName() const;
    unsigned int getNumArguments() const;
    const Program* getProgram() const;
    void getRequiredWorkGroupSize(size_t reqdWorkGroupSize[3]) const;
    void setArgument(unsigned int index, TypedValue value);

  private:
    const Program *m_program;
    const llvm::Function *m_function;
    TypedValueMap m_arguments;
    std::list<const llvm::GlobalVariable*> m_constants;
    std::list<const llvm::GlobalVariable*> m_globalVariables;
    std::list<size_t> m_constantBuffers;
    Memory *m_localMemory;
    std::string m_name;
    const llvm::MDNode *m_metadata;

    const llvm::Argument* getArgument(unsigned int index) const;
    const llvm::MDNode* getArgumentMetadata(std::string name) const;
  };
}
