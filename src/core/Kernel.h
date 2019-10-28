// Kernel.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
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
  class Metadata;
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

    TypedValueMap::const_iterator values_begin() const;
    TypedValueMap::const_iterator values_end() const;
    bool allArgumentsSet() const;
    unsigned int getArgumentAccessQualifier(unsigned int index) const;
    unsigned int getArgumentAddressQualifier(unsigned int index) const;
    const llvm::StringRef getArgumentName(unsigned int index) const;
    size_t getArgumentSize(unsigned int index) const;
    const llvm::StringRef getArgumentTypeName(unsigned int index) const;
    unsigned int getArgumentTypeQualifier(unsigned int index) const;
    std::string getAttributes() const;
    const llvm::Function* getFunction() const;
    size_t getLocalMemorySize() const;
    const std::string& getName() const;
    unsigned int getNumArguments() const;
    const Program* getProgram() const;
    void getRequiredWorkGroupSize(size_t reqdWorkGroupSize[3]) const;
    bool requiresUniformWorkGroups() const;
    void setArgument(unsigned int index, TypedValue value);

  private:
    const Program *m_program;
    const llvm::Function *m_function;
    const llvm::MDNode *m_metadata;
    std::string m_name;

    TypedValueMap m_values;

    bool m_requiresUniformWorkGroups;

    const llvm::Argument* getArgument(unsigned int index) const;
    const llvm::Metadata* getArgumentMetadata(std::string name,
                                              unsigned int index) const;
  };
}
