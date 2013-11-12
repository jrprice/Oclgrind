// Kernel.cpp (oclgrind)
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
#include <sstream>

#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/Support/raw_os_ostream.h"

#include "Kernel.h"
#include "Memory.h"

using namespace spirsim;
using namespace std;

Kernel::Kernel(const Program& program,
               const llvm::Function *function, const llvm::Module *module)
 : m_program(program)
{
  m_function = function;
  m_localMemory = new Memory();
  m_privateMemory = new Memory();

  // Get name
  m_name = function->getName().str();

  // Set-up global variables
  llvm::Module::const_global_iterator itr;
  for (itr = module->global_begin(); itr != module->global_end(); itr++)
  {
    llvm::PointerType *type = itr->getType();
    if (type->getPointerAddressSpace() == AddrSpaceLocal)
    {
      // Allocate buffer
      size_t size = getTypeSize(itr->getInitializer()->getType());
      TypedValue v = {
        sizeof(size_t),
        1,
        new unsigned char[sizeof(size_t)]
      };
      *((size_t*)v.data) = m_localMemory->allocateBuffer(size);
      m_arguments[itr] = v;
    }
    else if (itr->hasUnnamedAddr() && itr->getName().str()[0] != '.')
    {
      // Allocate buffer
      size_t size = getTypeSize(itr->getInitializer()->getType());
      TypedValue v = {
        sizeof(size_t),
        1,
        new unsigned char[sizeof(size_t)]
      };
      size_t address = m_privateMemory->allocateBuffer(size);
      *((size_t*)v.data) = address;
      m_arguments[itr] = v;

      // Initialise buffer contents
      unsigned char *data = new unsigned char[size];
      getConstantData(data, (const llvm::Constant*)itr->getInitializer());
      m_privateMemory->store(data, address, size);
      delete[] data;
    }
    else if (itr->isConstant())
    {
      m_constants.push_back(itr);
    }
  }

  // Get metadata node containing kernel arg info
  m_metadata = NULL;
  llvm::NamedMDNode *md = module->getNamedMetadata("opencl.kernels");
  if (md)
  {
    for (int i = 0; i < md->getNumOperands(); i++)
    {
      llvm::MDNode *node = md->getOperand(i);
      if (node->getOperand(0)->getName().str() == m_name)
      {
        m_metadata = node;
        break;
      }
    }
  }
}

Kernel::Kernel(const Kernel& kernel)
 : m_program(kernel.m_program)
{
  m_function = kernel.m_function;
  m_arguments = kernel.m_arguments;
  m_constants = kernel.m_constants;
  m_constantBuffers = kernel.m_constantBuffers;
  m_localMemory = kernel.m_localMemory->clone();
  m_privateMemory = kernel.m_privateMemory->clone();
  m_name = kernel.m_name;
  m_metadata = kernel.m_metadata;
}

Kernel::~Kernel()
{
  delete m_localMemory;
  delete m_privateMemory;
}

void Kernel::allocateConstants(Memory *memory)
{
  list<const llvm::GlobalVariable*>::const_iterator itr;
  for (itr = m_constants.begin(); itr != m_constants.end(); itr++)
  {
    const llvm::Constant *initializer = (*itr)->getInitializer();
    const llvm::Type *type = initializer->getType();

    // Allocate buffer
    size_t size = getTypeSize(type);
    TypedValue v = {
      sizeof(size_t),
      1,
      new unsigned char[sizeof(size_t)]
    };
    size_t address = memory->allocateBuffer(size);
    *((size_t*)v.data) = address;
    m_constantBuffers.push_back(address);
    m_arguments[*itr] = v;

    // Initialise buffer contents
    unsigned char *data = new unsigned char[size];
    getConstantData(data, (const llvm::Constant*)initializer);
    memory->store(data, address, size);
    delete[] data;
  }
}

void Kernel::deallocateConstants(Memory *memory)
{
  list<size_t>::const_iterator itr;
  for (itr = m_constantBuffers.begin(); itr != m_constantBuffers.end(); itr++)
  {
    memory->deallocateBuffer(*itr);
  }
  m_constantBuffers.clear();
}

const llvm::Argument* Kernel::getArgument(unsigned int index) const
{
  assert(index < getNumArguments());

  llvm::Function::const_arg_iterator argItr = m_function->arg_begin();
  for (int i = 0; i < index; i++)
  {
    argItr++;
  }
  return argItr;
}

unsigned int Kernel::getArgumentAccessQualifier(unsigned int index) const
{
  assert(index < getNumArguments());

  // Get metadata node
  const llvm::MDNode *node = getArgumentMetadata("kernel_arg_access_qual");
  if (!node)
  {
    return -1;
  }

  // Get qualifier string
  string str = node->getOperand(index+1)->getName();
  if (str == "read_only")
  {
    return CL_KERNEL_ARG_ACCESS_READ_ONLY;
  }
  else if (str == "write_only")
  {
    return CL_KERNEL_ARG_ACCESS_WRITE_ONLY;
  }
  else if (str == "read_write")
  {
    return CL_KERNEL_ARG_ACCESS_READ_WRITE;
  }
  return CL_KERNEL_ARG_ACCESS_NONE;
}

unsigned int Kernel::getArgumentAddressQualifier(unsigned int index) const
{
  assert(index < getNumArguments());

  // Get metadata node
  const llvm::MDNode *node = getArgumentMetadata("kernel_arg_addr_space");
  if (!node)
  {
    return -1;
  }

  // TODO: Remove this when kernel arg info metadata fixed in compiler
  if (strncmp(getArgumentTypeName(index), "image", 5) == 0)
  {
    return CL_KERNEL_ARG_ADDRESS_GLOBAL;
  }

  // Get address space
  switch(((llvm::ConstantInt*)node->getOperand(index+1))->getZExtValue())
  {
    case AddrSpacePrivate:
      return CL_KERNEL_ARG_ADDRESS_PRIVATE;
    case AddrSpaceGlobal:
      return CL_KERNEL_ARG_ADDRESS_GLOBAL;
    case AddrSpaceConstant:
      return CL_KERNEL_ARG_ADDRESS_CONSTANT;
    case AddrSpaceLocal:
      return CL_KERNEL_ARG_ADDRESS_LOCAL;
    default:
      return -1;
  }
}

const llvm::MDNode* Kernel::getArgumentMetadata(string name) const
{
  if (!m_metadata)
  {
    return NULL;
  }

  // Loop over all metadata nodes for this kernel
  for (int i = 0; i < m_metadata->getNumOperands(); i++)
  {
    const llvm::Value *value = m_metadata->getOperand(i);
    if (value->getType()->getTypeID() == llvm::Type::MetadataTyID)
    {
      // Check if node matches target name
      const llvm::MDNode *node = (llvm::MDNode*)value;
      if (node->getNumOperands() > 0 && node->getOperand(0)->getName() == name)
      {
        return node;
      }
    }
  }
  return NULL;
}

char* Kernel::getArgumentName(unsigned int index) const
{
  return strdup(getArgument(index)->getName().str().c_str());
}

char* Kernel::getArgumentTypeName(unsigned int index) const
{
  assert(index < getNumArguments());

  // Get metadata node
  const llvm::MDNode *node = getArgumentMetadata("kernel_arg_type");
  if (!node)
  {
    return NULL;
  }

  // Return copy of string
  return strdup(node->getOperand(index+1)->getName().str().c_str());
}

unsigned int Kernel::getArgumentTypeQualifier(unsigned int index) const
{
  assert(index < getNumArguments());

  // Get metadata node
  const llvm::MDNode *node = getArgumentMetadata("kernel_arg_type_qual");
  if (!node)
  {
    return -1;
  }

  // Get qualifiers
  istringstream iss(node->getOperand(index+1)->getName().str());

  unsigned int result = CL_KERNEL_ARG_TYPE_NONE;
  while (!iss.eof())
  {
    string tok;
    iss >> tok;
    if (tok == "const")
    {
      result |= CL_KERNEL_ARG_TYPE_CONST;
    }
    else if (tok == "restrict")
    {
      result |= CL_KERNEL_ARG_TYPE_RESTRICT;
    }
    else if (tok == "volatile")
    {
      result |= CL_KERNEL_ARG_TYPE_VOLATILE;
    }
  }

  return result;
}

size_t Kernel::getArgumentSize(unsigned int index) const
{
  const llvm::Type *type = getArgument(index)->getType();

  // Check if pointer argument
  if (type->isPointerTy())
  {
    return sizeof(size_t);
  }

  return getTypeSize(type);
}

string Kernel::getAttributes() const
{
  ostringstream attributes("");
  for (int i = 0; i < m_metadata->getNumOperands(); i++)
  {
    llvm::Value *op = m_metadata->getOperand(i);
    if (op->getValueID() == llvm::Value::MDNodeVal)
    {
      llvm::MDNode *val = ((llvm::MDNode*)op);
      string name = val->getOperand(0)->getName().str();

      if (name == "reqd_work_group_size" ||
          name == "work_group_size_hint")
      {
        attributes << name << "("
                   <<
          ((const llvm::ConstantInt*)val->getOperand(1))->getZExtValue()
                   << "," <<
          ((const llvm::ConstantInt*)val->getOperand(2))->getZExtValue()
                   << "," <<
          ((const llvm::ConstantInt*)val->getOperand(3))->getZExtValue()
                   << ") ";
      }
      else if (name == "vec_type_hint")
      {
        // Get type hint
        size_t n = 1;
        const llvm::Type *type = val->getOperand(1)->getType();
        if (type->isVectorTy())
        {
          n = type->getVectorNumElements();
          type = type->getVectorElementType();
        }

        // Generate attribute string
        attributes << name << "(" << flush;
        llvm::raw_os_ostream out(attributes);
        type->print(out);
        out.flush();
        attributes << n << ") ";
      }
    }
  }
  return attributes.str();
}

const llvm::Function* Kernel::getFunction() const
{
  return m_function;
}

const Memory* Kernel::getLocalMemory() const
{
  return m_localMemory;
}

size_t Kernel::getLocalMemorySize() const
{
  return m_localMemory->getTotalAllocated();
}

const std::string& Kernel::getName() const
{
  return m_name;
}

unsigned int Kernel::getNumArguments() const
{
  return m_function->arg_size();
}

const Memory* Kernel::getPrivateMemory() const
{
  return m_privateMemory;
}

const Program& Kernel::getProgram() const
{
  return m_program;
}

void Kernel::getRequiredWorkGroupSize(size_t reqdWorkGroupSize[3]) const
{
  memset(reqdWorkGroupSize, 0, 3*sizeof(size_t));
  for (int i = 0; i < m_metadata->getNumOperands(); i++)
  {
    llvm::Value *op = m_metadata->getOperand(i);
    if (op->getValueID() == llvm::Value::MDNodeVal)
    {
      llvm::MDNode *val = ((llvm::MDNode*)op);
      string name = val->getOperand(0)->getName().str();
      if (name == "reqd_work_group_size")
      {
        for (int j = 0; j < 3; j++)
        {
          reqdWorkGroupSize[j] =
            ((const llvm::ConstantInt*)val->getOperand(j+1))->getZExtValue();
        }
      }
    }
  }
}

void Kernel::setArgument(unsigned int index, TypedValue value)
{
  assert(index < m_function->arg_size());

  unsigned int type = getArgumentAddressQualifier(index);
  if (type == CL_KERNEL_ARG_ADDRESS_LOCAL)
  {
    const llvm::Value *arg = getArgument(index);

    // Deallocate existing argument
    if (m_arguments.find(arg) != m_arguments.end())
    {
      m_localMemory->deallocateBuffer(*(size_t*)m_arguments[arg].data);
    }

    // Allocate local memory buffer
    TypedValue v = {
      sizeof(size_t),
      1,
      new unsigned char[sizeof(size_t)]
    };
    *((size_t*)v.data) = m_localMemory->allocateBuffer(value.size);
    m_arguments[arg] = v;
  }
  else
  {
    const llvm::Type *type = getArgument(index)->getType();
    if (type->isVectorTy())
    {
      value.num = type->getVectorNumElements();
      value.size = getTypeSize(type->getVectorElementType());
    }
    m_arguments[getArgument(index)] = clone(value);
  }
}

TypedValueMap::const_iterator Kernel::args_begin() const
{
  return m_arguments.begin();
}

TypedValueMap::const_iterator Kernel::args_end() const
{
  return m_arguments.end();
}
