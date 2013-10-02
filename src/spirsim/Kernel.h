// Kernel.h (oclgrind)
// Copyright (C) 2013 James Price
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

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
    Kernel(const Kernel& kernel);
    virtual ~Kernel();

    TypedValueMap::const_iterator args_begin() const;
    TypedValueMap::const_iterator args_end() const;
    void allocateConstants(Memory *memory);
    void deallocateConstants(Memory *memory);
    size_t getArgumentSize(unsigned int index) const;
    unsigned int getArgumentType(unsigned int index) const;
    const llvm::Function* getFunction() const;
    const Memory* getLocalMemory() const;
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
    Memory *m_localMemory;
    std::string m_name;
    size_t m_requiredWorkGroupSize[3];

    const llvm::Argument* getArgument(unsigned int index) const;
  };
}
